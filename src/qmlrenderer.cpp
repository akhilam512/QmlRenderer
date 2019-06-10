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
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QCoreApplication>
#include <QEvent>
#include <QtConcurrent/QtConcurrent>
#include <memory>
#include <QDebug>

#include "qmlrenderer.h"
#include "qmlanimationdriver.h"

QmlRenderer::QmlRenderer(QObject *parent)
    : QObject(parent)
    , m_status(NotRunning)
{
    QSurfaceFormat format;
    // Qt Quick may need a depth and stencil buffer. Always make sure these are available.
    format.setDepthBufferSize(16);
    format.setStencilBufferSize(8);

    m_context = std::make_unique<QOpenGLContext>();
    m_context->setFormat(format);
    m_context->create();

    m_offscreenSurface = std::make_unique<QOffscreenSurface>();
    m_offscreenSurface->setFormat(m_context->format());
    m_offscreenSurface->create();

    m_renderControl = std::make_unique<QQuickRenderControl>(this);
    m_quickWindow = std::make_unique<QQuickWindow>(m_renderControl.get());
    m_qmlEngine = std::make_unique<QQmlEngine>(new QQmlEngine);

    if (!m_qmlEngine->incubationController())
        m_qmlEngine->setIncubationController(m_quickWindow->incubationController());

    m_context->makeCurrent(m_offscreenSurface.get());
    m_renderControl->initialize(m_context.get());
 }


void QmlRenderer::initialiseRenderParams(const QString &qmlFile, const QString &filename, const QString &outputDirectory, const QString &outputFormat, const QSize &size, qreal devicePixelRatio, int durationMs, int fps, bool isSingleFrame, qint64 frameTime)
{
    m_qmlFile = qmlFile;
    m_size = size;
    m_dpr = devicePixelRatio;
    m_duration = durationMs;
    m_fps = fps;
    m_outputName = filename;
    m_outputDirectory = outputDirectory;
    m_outputFormat = outputFormat;
    m_isSingleFrame = isSingleFrame;
    m_frameTime = frameTime;
}

void QmlRenderer::renderQml()
{
    if (m_status != NotRunning)
        return;

    if (!loadQML(m_qmlFile, m_size)) {
       return;
    }

   m_status = Running;
   createFbo();

   if (!m_context->makeCurrent(m_offscreenSurface.get()))
       return;

   m_frames = m_duration / 1000 * m_fps;
   m_animationDriver = std::make_unique<QmlAnimationDriver>(1000/m_fps);
   m_animationDriver->install();
   m_currentFrame = 0;
   m_futureCounter = 0;

   if (m_isSingleFrame== false){
       renderEntireQml();
   }
   else {
       m_selectFrame  =  static_cast<int>(m_frameTime / ((1000/m_fps)));
       renderSingleFrame();
   }
}

QmlRenderer::~QmlRenderer()
{
    m_context->makeCurrent(m_offscreenSurface.get());
    m_context->doneCurrent();
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
    m_quickWindow->setRenderTarget(m_fbo.get());
}

void QmlRenderer::destroyFbo()
{
    m_fbo.reset();
}

void QmlRenderer::checkComponent()
{
     if (m_qmlComponent->isError()) {
        const QList<QQmlError> errorList = m_qmlComponent->errors();
        for (const QQmlError &error : errorList)
            qDebug() << error.url() << error.line() << error;
    }
}

bool QmlRenderer::loadQML(const QString &qmlFile, const QSize &size)
{
    if (m_qmlComponent != nullptr) {
        m_qmlComponent.reset();
    }
    m_qmlComponent = std::make_unique<QQmlComponent>(m_qmlEngine.get(), QUrl(qmlFile), QQmlComponent::PreferSynchronous);

    checkComponent();

    QQmlEngine::setObjectOwnership(rootObject.get(), QQmlEngine::CppOwnership);
    rootObject.reset(m_qmlComponent->create());

    checkComponent();

    m_rootItem.reset(qobject_cast<QQuickItem*>(rootObject.get()));

    if (!m_rootItem) {
        qDebug()<< "run: Not a QQuickItem";
        rootObject.reset();
        return false;
    }

    // The root item is ready. Associate it with the window.
    m_rootItem->setParentItem(m_quickWindow->contentItem());

    m_rootItem->setWidth(size.width());
    m_rootItem->setHeight(size.height());

    m_quickWindow->setGeometry(0, 0, size.width(), size.height());

    return true;
}

void static saveImage(const QImage &image, const QString &outputFile)
{
    image.save(outputFile);
}

void QmlRenderer::futureFinished()
{
    m_futureCounter++;
    if (m_futureCounter == (m_frames - 1)) {
        m_futures.clear();
        m_status = NotRunning;
    }
}

void QmlRenderer::renderEntireQml()
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
    //Advance animation
    m_animationDriver->advance();

    if (m_currentFrame < m_frames) {
        //Schedule the next update
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    } else {
        //Finished
        cleanup();
    }
}

bool QmlRenderer::event(QEvent *event)
{
    if (event->type() == QEvent::UpdateRequest) {
        if(m_isSingleFrame==false) {
            renderEntireQml();
            return true;
        }
        else {
            renderSingleFrame();   //do not forget to change this when you test other  render functions (renderSelectFrame(), renderOneFrame())
            return true;

        }
    }
    return QObject::event(event);
}

bool QmlRenderer::isRunning()
{
    return m_status == Running;
}

void QmlRenderer::renderOneFrame() // advance animation using advance() repeatedly
{
    while(m_animationDriver->elapsed() < m_frameTime) {
        m_animationDriver->advance();
        QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    }

    m_renderControl->polishItems();
    m_renderControl->sync();
    m_renderControl->render();

    m_context->functions()->glFlush();

    m_currentFrame =static_cast<int>(m_frameTime / ((1000/m_fps)));
    m_outputFile =  QString(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);

    saveImage(m_fbo->toImage(), m_outputFile);

    cleanup();
    m_status = NotRunning;
}
void QmlRenderer::renderSelectFrame() // advance animation till required frame
{
    m_animationDriver->advance(m_frameTime);
    QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    // Render frame
    m_renderControl->polishItems();
    m_renderControl->sync();
    m_renderControl->render();

    m_context->functions()->glFlush();

    m_currentFrame =static_cast<int>(m_frameTime / ((1000/m_fps)));
    m_outputFile =  QString(m_outputDirectory + QDir::separator() + m_outputName + "_" + QString::number(m_currentFrame) + "." + m_outputFormat);

    saveImage(m_fbo->toImage(), m_outputFile);

    cleanup();
    m_status = NotRunning;
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

    if (m_currentFrame < m_frames) {
        //Schedule the next update
       QCoreApplication::postEvent(this, new QEvent(QEvent::UpdateRequest));
    } else {
        //Finished
        cleanup();
    }

}
