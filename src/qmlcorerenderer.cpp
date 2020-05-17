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

#include "qmlcorerenderer.h"
#include "qmlanimationdriver.h"
#include <memory>

#include <QCoreApplication>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QQuickRenderControl>
#include <QOpenGLFramebufferObject>
#include <QThread>
#include <QOpenGLFunctions>

QmlCoreRenderer::QmlCoreRenderer(QObject *parent)
    : QObject(parent),
    m_quit(false),
    m_fbo(nullptr),
    m_animationDriver(nullptr),
    m_offscreenSurface(nullptr),
    m_context(nullptr),
    m_quickWindow(nullptr),
    m_renderControl(nullptr)
    {}

QmlCoreRenderer::~QmlCoreRenderer()
{
}

bool QmlCoreRenderer::event(QEvent *e)
{
  //  qDebug() << " -------- CoreRenderer event handler -------- ";
        QMutexLocker lock(&m_mutex);
 //       qDebug() << " Not locked :(";
        switch (int(e->type())) {
            case INIT:
                init();
                return true;
            case RENDER:
   //         qDebug() << " RENDER event";
                render(&lock);
                return true;
            case RESIZE:
                // TODO
                return true;
            case STOP:
                cleanup();
                return true;
            default:
                return QObject::event(e);
        }
}

QImage QmlCoreRenderer::asyncRender()
{
        Q_ASSERT(QThread::currentThread() != this->thread());

//    qDebug() << " ----------- asynceRender() -------------";
    bool ok = m_mutex.tryLock();
//    qDebug() << "lock try == " << ok;
    QMutexLocker lock(&m_mutex);
 //   qDebug() << " Locked!!!! ";
    render(&lock);
    return m_image;
}

void QmlCoreRenderer::aboutToQuit()
{
    QMutexLocker lock(&m_quitMutex);
    m_quit = true;
}

void QmlCoreRenderer::init()
    {
   //     qDebug() << " COULD BE FROM INIT!!!!";
        m_context->makeCurrent(m_offscreenSurface);
        m_renderControl->initialize(m_context);
        m_animationDriver->install();
        qDebug() << " CORE RENDERER ELAPSED = " << m_animationDriver->elapsed();
    }

void QmlCoreRenderer::cleanup()
    {
        m_context->makeCurrent(m_offscreenSurface);
        m_renderControl->invalidate();
        delete m_fbo;
        m_fbo = nullptr;

        m_context->doneCurrent();

        m_animationDriver->uninstall();

        m_context->moveToThread(QCoreApplication::instance()->thread());
        m_cond.wakeOne();
    }

void QmlCoreRenderer::ensureFbo()
{
    Q_ASSERT(!m_size.isEmpty());
    Q_ASSERT(m_dpr != 0.0);

    if (m_fbo && m_fbo->size() != m_size * m_dpr) {
        delete m_fbo;
        m_fbo = nullptr;
    }

    if (!m_fbo) {
        m_fbo = new QOpenGLFramebufferObject(m_size * m_dpr, QOpenGLFramebufferObject::CombinedDepthStencil);
        m_quickWindow->setRenderTarget(m_fbo);
        Q_ASSERT(m_quickWindow->isSceneGraphInitialized());
    }
}

void QmlCoreRenderer::render(QMutexLocker *lock)
    {
      //  qDebug() << " ----------- render(lock) -------- ";
    //	Q_ASSERT(QThread::currentThread() != this->thread());
        if (!m_context->makeCurrent(m_offscreenSurface)) {
            qWarning("!!!!! ERROR : Failed to make context current on render thread");
            return;
        }

        ensureFbo();

        // Synchronization and rendering happens here on the render thread
        m_renderControl->sync();
        // Meanwhile on this thread continue with the actual rendering (into the FBO first).
        m_renderControl->render();
        m_context->functions()->glFlush();

        m_image = m_fbo->toImage();
        m_image.convertTo(m_format);
        qDebug() << " CORE RENDERER ELAPSED = " << m_animationDriver->elapsed();

        m_cond.wakeOne();
        lock->unlock();
        // The main thread can now continue
        // NOTE: the main thread should ideally be not blocked while rendering but since we are not doing anything intensive on the main thread, we can afford to do it

}
