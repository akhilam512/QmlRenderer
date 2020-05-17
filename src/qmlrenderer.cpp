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


QmlRenderer::QmlRenderer(QString qmlFileUrlString, int fps, int duration, qint64 frameTime, qreal devicePixelRatio, QObject *parent)
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
        , m_dpr(devicePixelRatio)
        , m_duration(duration)
        , m_fps(fps)
        , m_framesCount(0)
        , m_currentFrame(0)
        , m_quickInitialized(false)
        , m_psrRequested(false)
        , m_imageReady(false)
    {
        m_framesCount = m_duration* m_fps;

        initialiseContext();
    }

QmlRenderer::~QmlRenderer()
{

        m_corerenderer->mutex()->lock();
        m_corerenderer->requestStop();
        m_corerenderer->cond()->wait(m_corerenderer->mutex());
        m_corerenderer->mutex()->unlock();

        m_rendererThread->quit();
        m_rendererThread->wait();
    /*
        bool ok = m_context->makeCurrent(m_offscreenSurface);
        qDebug() << " debug == " << ok;
        delete m_renderControl;
        delete m_qmlComponent;
        delete m_quickWindow;
        delete m_qmlEngine;
        delete m_fbo;

        m_context->doneCurrent();

        delete m_offscreenSurface;
        delete m_context;
*/
//qDebug() << " HERE??? in destructor!!!!!!!!!";
        m_context->makeCurrent(m_offscreenSurface);
        m_renderControl->invalidate();
        delete m_fbo;
        m_fbo = nullptr;

        m_context->doneCurrent();

        m_animationDriver->uninstall();

}

bool QmlRenderer::event(QEvent *event)
{
  //  qDebug() << " ------------- QmlRenderer event hanlder --------- ";
    if(event->type() == UPDATE) {
        renderStatic();
        m_psrRequested = false;
        return true;
    }
    else if(event->type() == QEvent::UpdateRequest) {
  //      qDebug() << "Event recived ===== Update Request! ";
        renderAnimated();
        return true;
    }
    return QObject::event(event);
}

void QmlRenderer::init(int width, int height, QImage::Format imageFormat)
    {
        if (m_status == NotRunning) {

            m_size = QSize(width, height);
            m_ImageFormat = imageFormat;
            m_corerenderer->setSize(m_size);
            m_corerenderer->setFormat(m_ImageFormat);
            loadInput();
            m_corerenderer->requestInit();
            m_quickInitialized = true;
            m_status = Initialised;
        }
        //TODO : W/H/Format update case?
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

void QmlRenderer::createFbo()
    {
        Q_ASSERT(!m_size.isEmpty());
        Q_ASSERT(m_dpr != 0.0);
        m_fbo = new QOpenGLFramebufferObject(m_size * m_dpr, QOpenGLFramebufferObject::CombinedDepthStencil);
        Q_ASSERT(m_fbo != nullptr);
        m_quickWindow->setRenderTarget(m_fbo);
        Q_ASSERT(m_quickWindow->isSceneGraphInitialized());
        Q_ASSERT(m_quickWindow->renderTarget() != nullptr && m_quickWindow != nullptr);
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
    return renderStatic();
}

QImage QmlRenderer::renderStatic()
    {
        // Polishing happens on the main thread
        m_renderControl->polishItems();
        // Sync happens on the render thread with the main thread (this one) blocked
        QMutexLocker lock(m_corerenderer->mutex());
        m_corerenderer->requestRender();
        // Wait until sync is complete
        m_corerenderer->cond()->wait(m_corerenderer->mutex());
        return m_corerenderer->getRenderedQImage();
    }

QImage QmlRenderer::render(int width, int height, QImage::Format format, int frame)
{
    m_requestedFrame = frame;
    m_currentFrame = 0;
    m_imageReady = false;
    init(width, height, format);

    QEventLoop loop;
    connect(this, &QmlRenderer::imageReady, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    renderAnimated();
    loop.exec();

    return m_img;
}

void QmlRenderer::renderAnimated()
{
//    qDebug() <<" ----------- renderAnimated() called ----------- \n";
    // Polishing happens on the main thread
    m_renderControl->polishItems();
    // Sync happens on the render thread with the main thread (this one) blocked
    QMutexLocker lock(m_corerenderer->mutex());
 //   qDebug() << " Mutex Locked!!!";
    //m_img = m_corerenderer->asyncRender();
    m_corerenderer->requestRender();
    // Wait until sync & render is complete
    m_corerenderer->cond()->wait(m_corerenderer->mutex());
//	lock.unlock();
    if (m_currentFrame == m_requestedFrame) {
        m_imageReady = true;
        m_img =  m_corerenderer->getRenderedQImage();
        emit imageReady();
        return;
    }
    if (m_currentFrame < m_framesCount) {
     //   qDebug() << " OK SENDING EVENT!\n";
     //   qDebug() << " ANIM DRIVER : ELAPSED : " << m_animationDriver->elapsed();
        m_currentFrame++;
        m_animationDriver->advance();
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
    else {

        return;
    }
    //	QCoreApplication::processEvents();

}

void QmlRenderer::initialiseContext()
{
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

    m_rendererThread = new QThread;
    m_renderControl->prepareThread(m_rendererThread);
    m_animationDriver = new QmlAnimationDriver(1000/m_fps);
    m_animationDriver->install();
    m_corerenderer->setFPS(m_fps);
    m_corerenderer->setAnimationDriver(m_animationDriver);

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
    connect(m_renderControl, &QQuickRenderControl::renderRequested, this, &QmlRenderer::requestUpdate);
    connect(m_renderControl, &QQuickRenderControl::sceneChanged, this, &QmlRenderer::requestUpdate);
}

void QmlRenderer::requestUpdate()
{

    if (m_quickInitialized && !m_psrRequested) {
        m_psrRequested = true;
        QCoreApplication::postEvent(this, new QEvent(UPDATE));
    }
}

