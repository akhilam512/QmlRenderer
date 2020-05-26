/*
Copyright (C) 2019  Akhil K Gangadharan <helloimakhil@gmail.com>
This file is part of Kdenlive. See www.kdenlive.org.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of
the License or (at your option) version 3 or any later version
accepted by the membership of KDE e.V. (or its successor approved
by the membership of KDE e.V.), which shall act as a proxy
defined in Section 14 of version 3 of the license.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "qmlrenderer.h"
#include <QEvent>

/*
 * The QmlRenderer class renders a given QML file using QQuickRenderControl
 * onto a QOpenGlFrameBufferObject which can then be saved to create an
 * QImage.
 *
 * The rendering is done by the QmlCoreRenderer class, which does all of
 * its functions (syncing, rendering) on a separate thread dedicated for rendering.
 * The QOpenGLContext and QQuickRenderControl live on the render thread. Syncing,
 * rendering and related gl functions are called from the render thread, while
 * polishing is done on the main thread.
 *
 * Note that the aim of a separate thread for rendering is not to improve
 * performance but to ensure that there is no clash between OpenGL contexts
 * of this renderer with the OpenGL context of the application that is using
 * the renderer, which would be living on the same thread otherwise. This can
 * lead to crashes when one context tries to call makeCurrent() while living being on
 * the same thread another context lives in.
 *
 * The renderer uses it's own custom QAnimationDriver class to advance QML animations
 * at a given frame rate.
*/

QmlRenderer::QmlRenderer(QString qmlFileUrlString, int fps, int duration, QObject *parent)
    : QObject(parent)
    , m_fbo(nullptr)
    , m_animationDriver(nullptr)
    , m_offscreenSurface(nullptr)
    , m_context(nullptr)
    , m_quickWindow(nullptr)
    , m_renderControl(nullptr)
    , m_rootItem(nullptr)
    , m_corerenderer(nullptr)
    , m_qmlComponent(nullptr)
    , m_qmlEngine(nullptr)
    , m_status(NotRunning)
    , m_qmlFileUrl(qmlFileUrlString)
    , m_dpr(1.0)
    , m_duration(duration)
    , m_fps(fps)
    , m_currentFrame(0)
    , m_framesCount(fps*duration)
{
    //    QCoreApplication::setAttribute(Qt::AA_DontCheckOpenGLContextThreadAffinity);
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    m_context = new QOpenGLContext();
    m_context->setFormat(format);
    Q_ASSERT(format.depthBufferSize() == (m_context->format()).depthBufferSize());
    Q_ASSERT(format.stencilBufferSize() == (m_context->format()).stencilBufferSize());
    m_context->create();
    Q_ASSERT(m_context->isValid());

    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();
    Q_ASSERT(m_offscreenSurface->isValid());

    m_renderControl = new QQuickRenderControl(this);
    Q_ASSERT(m_renderControl != nullptr);
    QQmlEngine::setObjectOwnership(m_renderControl, QQmlEngine::CppOwnership);

    m_quickWindow = new QQuickWindow(m_renderControl);
    Q_ASSERT(m_quickWindow != nullptr);

    m_qmlEngine = new QQmlEngine();
    if (!m_qmlEngine->incubationController()) {
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());
    }

    m_corerenderer = new QmlCoreRenderer();
    m_corerenderer->setContext(m_context);
    m_corerenderer->setSurface(m_offscreenSurface);
    m_corerenderer->setQuickWindow(m_quickWindow);
    m_corerenderer->setRenderControl(m_renderControl);
    m_corerenderer->setDPR(m_dpr);
    m_corerenderer->setFPS(m_fps);

    m_rendererThread = new QThread;
    m_renderControl->prepareThread(m_rendererThread);

    m_context->moveToThread(m_rendererThread);
    m_corerenderer->moveToThread(m_rendererThread);
    m_rendererThread->start();

    connect(
        m_quickWindow, &QQuickWindow::sceneGraphError,
        [=]( QQuickWindow::SceneGraphError error, const QString &message) {
            qDebug() << "!!!!!!!! ERROR - QML Scene Graph: " << error << message;
            }
    );
    connect(
        m_qmlEngine, &QQmlEngine::warnings,
        [=]( QList<QQmlError> warnings) {
            foreach(const QQmlError& warning, warnings) {
                qDebug() << "!!!! QML WARNING : "  << warning << "  " ;
            }
        }
    );
}

QmlRenderer::~QmlRenderer()
{
    m_corerenderer->mutex()->lock();
    m_corerenderer->requestStop();
    m_corerenderer->cond()->wait(m_corerenderer->mutex());
    m_corerenderer->mutex()->unlock();

    m_rendererThread->quit();
    m_rendererThread->wait();

    m_context->makeCurrent(m_offscreenSurface);
    m_renderControl->invalidate();
    delete m_fbo;
    m_fbo = nullptr;

    m_context->doneCurrent();

    delete m_context;
    delete m_offscreenSurface;
}

bool QmlRenderer::eventFilter(QObject *obj, QEvent *event)
{
    if(event->type() == QEvent::UpdateRequest)
    {
        renderAnimated();
        return true;
    }
    return QObject::event(event);
}

void QmlRenderer::init(int width, int height, QImage::Format imageFormat)
{
    if (m_status == NotRunning || m_duration > 0) {
        initDriver();
        m_size = QSize(width, height);
        m_ImageFormat = imageFormat;
        m_corerenderer->setSize(m_size);
        m_corerenderer->setFormat(m_ImageFormat);
        loadInput();
        m_corerenderer->requestInit();
        m_status = Initialised;
    }
    //TODO : W/H/Format update case?
}

void QmlRenderer::initDriver()
{
    /*
    We use a corrected FPS because animations take more frames
    than expected due to a weird behaviour of the custom QAnimation
    Driver. For different timesteps the animations lag by a certain
    amount of frames, values for which were investigated for different
    values of fps's. The cause of this bug remains unknown for the time
    being but it is probably an upstream one.
    */
    int correctedFps = m_fps;
    if (m_fps < 40) {
        correctedFps -= 1;
    }
    else if (m_fps <= 60) {
        correctedFps -= 2;
    }
    else if (m_fps <= 69) {
        correctedFps -= 3;
    }
    else if (m_fps <= 80) {
        correctedFps -= 4;
    }
    else {
        correctedFps -= 5;
    }

    m_animationDriver = new QmlAnimationDriver(1000 / correctedFps);
    m_animationDriver->install();
    m_corerenderer->setAnimationDriver(m_animationDriver);
}

void QmlRenderer::resetDriver()
{
    m_animationDriver->uninstall();
    delete m_animationDriver;
    m_animationDriver = nullptr;
}

void QmlRenderer::loadInput()
{
    m_qmlComponent = new QQmlComponent(m_qmlEngine, QUrl(m_qmlFileUrl), QQmlComponent::PreferSynchronous);
    Q_ASSERT(!m_qmlComponent->isNull() || m_qmlComponent->isReady());
    bool assert = loadRootObject();
    Q_ASSERT(assert);
    Q_ASSERT(!m_size.isEmpty());
    m_rootItem->setWidth(m_size.width());
    m_rootItem->setHeight(m_size.height());
    m_quickWindow->setGeometry(0, 0, m_size.width(), m_size.height());
}

bool QmlRenderer::loadRootObject()
{
    if(!checkQmlComponent()) {
        return false;
    }
    Q_ASSERT(m_qmlComponent->create() != nullptr);
    QObject *rootObject = m_qmlComponent->create();
    QQmlEngine::setObjectOwnership(rootObject, QQmlEngine::CppOwnership);
    Q_ASSERT(rootObject);
    if(!checkQmlComponent()) {
        return false;
    }
    m_rootItem = qobject_cast<QQuickItem*>(rootObject);
    if (!m_rootItem) {
        qDebug()<< "ERROR - run: Not a QQuickItem - QML file INVALID ";
        delete rootObject;
        return false;
    }
    m_rootItem->setParentItem(m_quickWindow->contentItem());
    return true;
}

bool QmlRenderer::checkQmlComponent()
{
    if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList) {
            qDebug() <<"QML Component Error: " << error.url() << error.line() << error;
        }
        return false;
    }
    return true;
}

QImage QmlRenderer::render(int width, int height, QImage::Format format)
{
    init(width, height, format);
    renderStatic();
    return m_img;
}

void QmlRenderer::renderStatic()
{
    polishSyncRender();
    m_img = m_corerenderer->getRenderedQImage();
}

QImage QmlRenderer::render(int width, int height, QImage::Format format, int frame)
{
    m_requestedFrame = frame;
    m_currentFrame = 0;

    init(width, height, format);
    installEventFilter(this);

    QEventLoop loop;
    connect(this, &QmlRenderer::imageReady, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    renderAnimated();
    loop.exec();

    resetDriver();
    delete m_qmlComponent;
    delete m_rootItem;

    return m_img;
}

void QmlRenderer::renderAnimated()
{
    polishSyncRender();
    m_animationDriver->advance();

    if(m_currentFrame == m_requestedFrame) {
        m_img = m_corerenderer->getRenderedQImage();
        removeEventFilter(this);
        emit imageReady();
    }

    m_currentFrame++;

    if (m_currentFrame < m_framesCount) {
        QEvent *updateRequest = new QEvent(QEvent::UpdateRequest);
        QCoreApplication::postEvent(this, updateRequest);
    }

    else {
        removeEventFilter(this);
        emit imageReady();
        return;
    }
}

void QmlRenderer::polishSyncRender()
{
    // Polishing happens on the main thread
    m_renderControl->polishItems();
    // Sync and render happens on the render thread with the main thread (this one) blocked
    QMutexLocker lock(m_corerenderer->mutex());
    m_corerenderer->requestRender();
    // Wait until sync and render is complete
    m_corerenderer->cond()->wait(m_corerenderer->mutex());
}
