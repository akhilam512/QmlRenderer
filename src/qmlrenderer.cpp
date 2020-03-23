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

#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QOffscreenSurface>
#include <QScreen>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QCoreApplication>
#include <QEvent>
#include <QtConcurrent/QtConcurrent>
#include <memory>
#include <QDebug>
#include <QQmlEngine>

#include "qmlrenderer.h"
#include "qmlanimationdriver.h"
#include "corerenderer.h"

QmlRenderer::QmlRenderer(QString qmlFileUrlString, qint64 frameTime, qreal devicePixelRatio, int durationMs, int fps, QObject *parent)
    : QObject(parent)
    , m_status(NotRunning)
    , m_qmlFileUrl(qmlFileUrlString)
    , m_frameTime(frameTime)
    , m_dpr(devicePixelRatio)
    , m_duration(durationMs)
    , m_fps(fps)
    , m_framesCount(0)
    , m_currentFrame(0)
{
    m_selectFrame =  static_cast<int>(m_frameTime / ((1000/m_fps)));
    m_framesCount = (m_duration / 1000 )* m_fps;

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
}

void QmlRenderer::initialiseContext()
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    m_context = std::make_shared<QOpenGLContext>();
    m_context->setFormat(format);
    Q_ASSERT(format.depthBufferSize() == (m_context->format()).depthBufferSize());
    Q_ASSERT(format.stencilBufferSize() == (m_context->format()).stencilBufferSize());
    m_context->create();
    Q_ASSERT(m_context->isValid());

    m_offscreenSurface = std::make_shared<QOffscreenSurface>();
    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();
    Q_ASSERT(m_offscreenSurface->isValid());

    m_renderControl = std::make_shared<QQuickRenderControl>(this);
    Q_ASSERT(m_renderControl != nullptr);
    QQmlEngine::setObjectOwnership(m_renderControl.get(), QQmlEngine::CppOwnership);

    m_quickWindow = std::make_shared<QQuickWindow>(m_renderControl.get());
    Q_ASSERT(m_quickWindow != nullptr);

    m_qmlEngine = std::make_unique<QQmlEngine>();
    if (!m_qmlEngine->incubationController()) {
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());
    }

    m_corerenderer = std::make_unique<CoreRenderer>();
    m_corerenderer->setContext(m_context);

    m_corerenderer->setSurface(m_offscreenSurface);
    m_corerenderer->setQuickWindow(m_quickWindow);
    m_corerenderer->setRenderControl(m_renderControl);

    m_corerenderer->setFPS(m_fps);
    m_corerenderer->setDPR(m_dpr);

    m_rendererThread = new QThread;

    m_renderControl->prepareThread(m_rendererThread);

    m_context->moveToThread(m_rendererThread);
    m_corerenderer->moveToThread(m_rendererThread);

    m_rendererThread->start();

    connect(m_quickWindow.get(), SIGNAL(sceneGraphError(QQuickWindow::SceneGraphError, const QString)), this, SLOT(displaySceneGraphError(QQuickWindow::SceneGraphError, const QString)));
    connect(m_qmlEngine.get(), SIGNAL(warnings(QList<QQmlError>)), this, SLOT(displayQmlError(QList<QQmlError>)));
}

bool QmlRenderer::checkQmlComponent()
{
     if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qDebug() <<"QML Component Error: " << error.url() << error.line() << error;
        return false;
     }
    return true;
}

bool QmlRenderer::loadRootObject()
{
    if(!checkQmlComponent()) {
        return false;
    }
    Q_ASSERT(m_qmlComponent->create() != nullptr);
    m_rootObject.reset(m_qmlComponent->create());
    QQmlEngine::setObjectOwnership(m_rootObject.get(), QQmlEngine::CppOwnership);
    Q_ASSERT(m_rootObject);
    if(!checkQmlComponent()) {
        return false;
    }

    m_rootItem.reset(qobject_cast<QQuickItem*>(m_rootObject.get()));

    if (!m_rootItem) {
        qDebug()<< "ERROR - run: Not a QQuickItem - QML file INVALID ";
        m_rootObject.reset();
        return false;
    }
    m_rootItem->setParentItem(m_quickWindow->contentItem());
    return true;
}

void QmlRenderer::createFbo()
{
    Q_ASSERT(!m_size.isEmpty());
    Q_ASSERT(m_dpr != 0.0);
    m_fbo = std::make_unique<QOpenGLFramebufferObject>(m_size * m_dpr, QOpenGLFramebufferObject::CombinedDepthStencil);
    Q_ASSERT(m_fbo != nullptr);
    m_quickWindow->setRenderTarget(m_fbo.get());
    Q_ASSERT(m_quickWindow->isSceneGraphInitialized());
    Q_ASSERT(m_quickWindow->renderTarget() != nullptr && m_quickWindow != nullptr);
}

void QmlRenderer::loadInput()
{
    m_qmlComponent = std::make_unique<QQmlComponent>(m_qmlEngine.get(), QUrl(m_qmlFileUrl), QQmlComponent::PreferSynchronous);
    Q_ASSERT(!m_qmlComponent->isNull() || m_qmlComponent->isReady());
    bool assert = loadRootObject();
    Q_ASSERT(assert);

    Q_ASSERT(!m_size.isEmpty());
    m_rootItem->setWidth(m_size.width());
    m_rootItem->setHeight(m_size.height());
    m_quickWindow->setGeometry(0, 0, m_size.width(), m_size.height());
}

void QmlRenderer::init(int width, int height, const QImage::Format imageFormat)
{
    if (m_status == NotRunning) {
        m_size = QSize(width, height);
        m_ImageFormat = imageFormat;
        m_corerenderer->setSize(m_size);
        m_corerenderer->setFormat(m_ImageFormat);
        loadInput();
        m_corerenderer->requestInit();
        m_status = Initialised;
    }
    //TODO: Fix crash caused from code below
    /*
    else if ( width != m_size.width() || height != m_size.height() || imageFormat != m_ImageFormat ) {
        qDebug() << " HEIGHT AND WIDTH CHANGED111 !!!! !! \;n \n ";
        m_size = QSize(width, height);
        m_ImageFormat = imageFormat;
        m_corerenderer->requestStop();
        loadInput();
        m_corerenderer->requestInit();
        m_status = Initialised;
    }
    */
}

QImage QmlRenderer::renderToQImage()
{
    // Polishing happens on the main thread
    m_renderControl->polishItems();
    // Sync happens on the render thread with the main thread (this one) blocked
    QMutexLocker lock(m_corerenderer->mutex());
    m_corerenderer->requestRender();
    // Wait until sync is complete
    m_corerenderer->cond()->wait(m_corerenderer->mutex());

    return m_corerenderer->image;
}


QImage QmlRenderer::render(int width, int height, QImage::Format format)
{
    init(width, height, format);
    return renderToQImage();
}

void QmlRenderer::displaySceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
{
    qDebug() << "!!!!!!! ERROR : QML Scene Graph " << error << message;
}

void QmlRenderer::displayQmlError(QList<QQmlError> errors)
{
    foreach(const QQmlError& error, errors) {
        qDebug() << "!!!!!!!! QML ERROR : "  << error << "  " ;
    }
}
