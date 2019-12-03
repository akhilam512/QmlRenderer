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

/*
 * Note:
 * 1 - It is the responsbility of the implementing program to call cleanup(). The program can
 *     do this by catching the terminate() signal and connecting it to a slot.
 *
*/

QmlRenderer::QmlRenderer(QObject *parent)
    : QObject(parent)
    , m_status(NotRunning)
    , m_framesCount(0)
    , m_currentFrame(0)
    , m_futureFinishedCounter(0)
{
    QSurfaceFormat format;
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);
    m_context = std::make_unique<QOpenGLContext>();
    m_context->setFormat(format);
    Q_ASSERT(format.depthBufferSize() == (m_context->format()).depthBufferSize());
    Q_ASSERT(format.stencilBufferSize() == (m_context->format()).stencilBufferSize());
    m_context->create();
    Q_ASSERT(m_context->isValid());

    m_offscreenSurface = std::make_unique<QOffscreenSurface>();
    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();
    Q_ASSERT(m_offscreenSurface->isValid());

    m_renderControl = std::make_unique<QQuickRenderControl>(this);
    Q_ASSERT(m_renderControl != nullptr);
    QQmlEngine::setObjectOwnership(m_renderControl.get(), QQmlEngine::CppOwnership);
    m_quickWindow = std::make_unique<QQuickWindow>(m_renderControl.get());
    Q_ASSERT(m_quickWindow != nullptr);

    m_qmlEngine = std::make_unique<QQmlEngine>();
    if (!m_qmlEngine->incubationController()) {
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());
    }

    m_context->makeCurrent(m_offscreenSurface.get());
    Q_ASSERT(m_context->currentContext()!= nullptr);
    m_renderControl->initialize(m_context.get());

    connect(m_quickWindow.get(), SIGNAL(sceneGraphError(QQuickWindow::SceneGraphError, const QString)), this, SLOT(displaySceneGraphError(QQuickWindow::SceneGraphError, const QString)));
    connect(m_qmlEngine.get(), SIGNAL(warnings(QList<QQmlError>)), this, SLOT(displayQmlError(QList<QQmlError>)));
}

QmlRenderer::~QmlRenderer()
{
    m_context->makeCurrent(m_offscreenSurface.get());
    m_context->doneCurrent();
}

QImage QmlRenderer::m_frame = QImage();
bool QmlRenderer::m_ifProducer = false;

void QmlRenderer::displaySceneGraphError(QQuickWindow::SceneGraphError error, const QString &message)
{
    qDebug() << "ERROR : QML Scene Graph " << error << message;
}

void QmlRenderer::displayQmlError(QList<QQmlError> warnings)
{
    foreach(const QQmlError& warning, warnings) {
        qDebug() << "QML ERROR: "  << warning;
    }
}

int QmlRenderer::getStatus()
{
    return m_status;
}

int QmlRenderer::getCalculatedFramesCount()
{
    return m_framesCount;
}

int QmlRenderer::getActualFramesCount()
{
    return m_currentFrame;
}

int QmlRenderer::getSelectFrame()
{
    return m_selectFrame;
}

bool QmlRenderer::getSceneGraphStatus()
{
    if(m_status == Initialised) {
        return m_quickWindow->isSceneGraphInitialized();
    }
    return false;
}

bool QmlRenderer::getAnimationDriverStatus()
{
    if(m_status == Initialised && m_animationDriver) {
        return m_animationDriver->isRunning();
    }
    return false;
}

bool QmlRenderer::getfboStatus()
{
    if(m_status == Initialised && m_fbo) {
        return m_fbo->isBound();
    }

    return false;
}

int QmlRenderer::getFutureCount()
{
    return m_futures.count();
}

void QmlRenderer::getAllParams()
{
    qDebug() << "frames" << m_fps;
    qDebug() << "file" << m_qmlFileText <<" : " << m_qmlFileUrl;
    qDebug() << "odir" << m_outputDirectory;
    qDebug() << "outputfile name" << m_outputName;
    qDebug() << "format" << m_outputFormat;
    qDebug() << "durration" << m_duration;
    qDebug() << "single frame" << m_isSingleFrame;
    qDebug() << "actual frames " << m_currentFrame;
    qDebug() << "calculated frames " << m_framesCount;
    qDebug() << "frame time " << m_frameTime;
}


void QmlRenderer::initRenderParams(const QUrl &qmlFileUrl, bool isSingleFrame, int width, int height,  const QImage::Format imageFormat, qint64 frameTime, const QString &outputFormat, qreal devicePixelRatio, int durationMs, int fps)
{
    //TODO: remember to remove DURATION Once you confirm that the number of frames is not actually needed by the producer
    //TODO: write legible tests

    m_ifProducer = true;
    m_ImageFormat = imageFormat;
    m_qmlFileUrl = qmlFileUrl;
    m_size = QSize(width, height);
    m_dpr = devicePixelRatio;
    m_duration = durationMs;
    m_outputFormat = outputFormat;
    m_isSingleFrame = isSingleFrame;

    m_frameTime = 1000; //hard coded for the time being to test QML files with no animation
    m_fps = 1;
    m_duration = 1000;

    Q_ASSERT(m_fps!=0);

    if(isSingleFrame) {
        m_selectFrame  =  static_cast<int>(m_frameTime / ((1000/m_fps)));
    }

    m_framesCount = (m_duration / 1000 )* m_fps;

    if (!loadComponent(m_qmlFileUrl)) {
       return;
    }
}


void QmlRenderer::initialiseRenderParams(const QUrl &qmlFileUrl, bool isSingleFrame, qint64 frameTime, const QString &filename, const QString &outputDirectory, const QString &outputFormat, const QSize &size, qreal devicePixelRatio, int durationMs, int fps)
{
    m_qmlFileUrl = qmlFileUrl;
    m_size = size;
    m_dpr = devicePixelRatio;
    m_duration = durationMs;
    m_fps = fps;
    m_outputName = filename;
    m_outputDirectory = outputDirectory;
    m_outputFormat = outputFormat;
    m_isSingleFrame = isSingleFrame;
    m_frameTime = frameTime;
    Q_ASSERT(m_fps!=0);

    if(isSingleFrame) {
        m_selectFrame  =  static_cast<int>(m_frameTime / ((1000/m_fps)));
    }

    m_framesCount = (m_duration / 1000 )* m_fps;

    if (!loadComponent(m_qmlFileUrl)) {
       return;
    }
}

void QmlRenderer::prepareRenderer()
{
    //TODO: Consider merging with init()

    if (m_status == Running) {
        return;
    }

    createFbo();

    m_animationDriver = std::make_unique<QmlAnimationDriver>(1000/m_fps);
    m_animationDriver->install();
    //Q_ASSERT(!m_animationDriver->isRunning());
    m_status = Initialised;
}

void QmlRenderer::renderQml()
{
   m_status = Running;
   if (m_ifProducer) {
       renderToQImage();
   }
   else if (m_isSingleFrame){
        renderSingleFrame();
   }
   else {
        renderAllFrames();
   }
}

void QmlRenderer::renderToQImage()
{
    // Render frame
    m_renderControl->polishItems();
    m_renderControl->sync();
    m_renderControl->render();

    m_context->functions()->glFlush();
    m_currentFrame++;

    watcher = std::make_unique<QFutureWatcher<void>>();
    connect(watcher.get(), SIGNAL(finished()), this, SLOT(futureFinished()));
    if(m_currentFrame ==  m_selectFrame) {
        QImage img = m_fbo->toImage();
        watcher->setFuture(QtConcurrent::run(saveImage, img, img.width(), img.height(), m_ImageFormat));
        return;
    }
    m_futures.append(std::move(watcher));

    //advance animation
    m_animationDriver->advance();

    if (m_currentFrame < m_framesCount) {
        //Schedule the next update
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

void QmlRenderer::cleanup()
{
    m_animationDriver->uninstall();
    m_animationDriver.reset();
    destroyFbo();
}

void QmlRenderer::createFbo()
{
    m_fbo = std::make_unique<QOpenGLFramebufferObject>(m_size * m_dpr, QOpenGLFramebufferObject::CombinedDepthStencil);
    Q_ASSERT(m_fbo != nullptr);
    m_quickWindow->setRenderTarget(m_fbo.get());
    Q_ASSERT(m_quickWindow->renderTarget() != nullptr && m_quickWindow != nullptr);
}

void QmlRenderer::destroyFbo()
{
    m_fbo.reset();
}

bool QmlRenderer::checkIfComponentOK()
{
     if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qDebug() <<"QML Component Error: " << error.url() << error.line() << error;
        return false;
     }
    return true;
}

bool QmlRenderer::loadComponent(const QUrl &qmlFileUrl)
{
    if (m_qmlComponent != nullptr) {
        m_qmlComponent.reset();
    }

    m_qmlComponent = std::make_unique<QQmlComponent>(m_qmlEngine.get(), QUrl(qmlFileUrl), QQmlComponent::PreferSynchronous);
    Q_ASSERT(!m_qmlComponent->isNull() || m_qmlComponent->isReady());

    if(!loadQML()) {
        return false;
    }

    return true;
}

bool QmlRenderer::loadComponent(const QString &qmlFileText)
{
    if (m_qmlComponent != nullptr) {
        m_qmlComponent.reset();
    }

    m_qmlComponent = std::make_unique<QQmlComponent>(m_qmlEngine.get(), qmlFileText, QQmlComponent::PreferSynchronous);
    Q_ASSERT(!m_qmlComponent->isNull() || m_qmlComponent->isReady());


    if(!loadQML()) {
        return false;
    }
    return true;
}

bool QmlRenderer::loadQML()
{
    if(!checkIfComponentOK()) {
        return false;
    }

    QQmlEngine::setObjectOwnership(m_rootObject.get(), QQmlEngine::CppOwnership);
    m_rootObject.reset(m_qmlComponent->create());
    Q_ASSERT(m_rootObject);

    if(!checkIfComponentOK()) {
        return false;
    }

    m_rootItem.reset(qobject_cast<QQuickItem*>(m_rootObject.get()));

    if (!m_rootItem) {
        qDebug()<< "ERROR - run: Not a QQuickItem - QML file INVALID ";
        m_rootObject.reset();
        return false;
    }

    // The root item is ready. Associate it with the window.
    m_rootItem->setParentItem(m_quickWindow->contentItem());
    m_rootItem->setWidth(m_size.width());
    m_rootItem->setHeight(m_size.height());

    m_quickWindow->setGeometry(0, 0, m_size.width(), m_size.height());
    return true;
}

void QmlRenderer::futureFinished()
{
    m_futureFinishedCounter++;
    if (getFutureCount() == (m_framesCount - 1) || m_isSingleFrame) {
        m_status = NotRunning;
        getAllParams();
        emit terminate();
    }
}

void QmlRenderer::renderAllFrames()
{
    // Render frame
    m_renderControl->polishItems();
    m_renderControl->sync();
    m_renderControl->render();

    m_context->functions()->glFlush();

    m_currentFrame++;
    m_outputFile =  QString(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);

    watcher = std::make_unique<QFutureWatcher<void>>();
    connect(watcher.get(), SIGNAL(finished()), this, SLOT(futureFinished()));
    watcher->setFuture(QtConcurrent::run(saveImage, m_fbo->toImage(), m_outputFile));
    m_futures.append((std::move(watcher)));
    //Q_ASSERT(m_futures.back()->isRunning()); // make sure the last future is running

    //Advance animation
    m_animationDriver->advance();
    Q_ASSERT(m_animationDriver->isRunning());

    if (m_currentFrame < m_framesCount) {
        //Schedule the next update
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}

bool QmlRenderer::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        if(m_isSingleFrame == true) {
            renderSingleFrame();
            return true;
        }
        else {
            renderAllFrames();
            return true;
        }
    }
    return QObject::event(event);
}

bool QmlRenderer::isRunning()
{
    return m_status == Running;
}

void QmlRenderer::renderSingleFrame()  //  CURRENT APPROACH : render frames without saving frames till the required frame is reached
{
    // Render frame
    m_renderControl->polishItems();
    m_renderControl->sync();
    m_renderControl->render();

    m_context->functions()->glFlush();

    m_currentFrame++;

    m_outputFile =  QString(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);
    watcher = std::make_unique<QFutureWatcher<void>>();
    connect(watcher.get(), SIGNAL(finished()), this, SLOT(futureFinished()));
    if(m_currentFrame ==  m_selectFrame) {
        watcher->setFuture(QtConcurrent::run(saveImage, m_fbo->toImage(), m_outputFile));
        return;
    }
    m_futures.append(std::move(watcher));

    //advance animation
    m_animationDriver->advance();

    if (m_currentFrame < m_framesCount) {
        //Schedule the next update
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }
}
