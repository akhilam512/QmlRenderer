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

#include "corerenderer.h"
#include "qmlanimationdriver.h"
#include <memory>

#include <QCoreApplication>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QQuickRenderControl>
#include <QOpenGLFramebufferObject>
#include <QThread>
#include <QOpenGLFunctions>

CoreRenderer::CoreRenderer(QObject *parent)
    : QObject(parent),
      m_quit(false)
{

}

CoreRenderer::~CoreRenderer() = default;

void CoreRenderer::requestInit()
{
    QCoreApplication::postEvent(this, new QEvent(INIT));
}

void CoreRenderer::requestRender()
{
    QCoreApplication::postEvent(this, new QEvent(RENDER));
}

void CoreRenderer::requestResize()
{
    QCoreApplication::postEvent(this, new QEvent(RESIZE));
}

void CoreRenderer::requestStop()
{
    QCoreApplication::postEvent(this, new QEvent(STOP));
}


bool CoreRenderer::event(QEvent *e)
{
    QMutexLocker lock(&m_mutex);

    switch (int(e->type())) {
    case INIT:
        init();
        return true;
    case RENDER:
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

void CoreRenderer::init()
{
    m_context->makeCurrent(m_offscreenSurface.get());

    m_renderControl->initialize(m_context.get());

    Q_ASSERT(m_fps>0);
    m_animationDriver = std::make_unique<QmlAnimationDriver>(1000/m_fps);
    m_animationDriver->install();
}

void CoreRenderer::cleanup()
{
    m_context->makeCurrent(m_offscreenSurface.get());

    m_renderControl->invalidate();

    m_context->doneCurrent();
    m_context->moveToThread(QCoreApplication::instance()->thread());

    m_cond.wakeOne();
}

void CoreRenderer::ensureFbo()
{
    Q_ASSERT(!m_size.isEmpty());
    Q_ASSERT(m_dpr != 0.0);

    if (m_fbo && m_fbo->size() != m_size * m_dpr) {
        m_fbo.reset();
    }

    if (!m_fbo) {
        m_fbo = std::make_unique<QOpenGLFramebufferObject>(m_size * m_dpr, QOpenGLFramebufferObject::CombinedDepthStencil);
        m_quickWindow->setRenderTarget(m_fbo.get());
        Q_ASSERT(m_quickWindow->isSceneGraphInitialized());
    }
}

void CoreRenderer::render(QMutexLocker *lock)
{
   //  Q_ASSERT(QThread::currentThread() != m_window->thread());

    if (!m_context->makeCurrent(m_offscreenSurface.get())) {
        qWarning("Failed to make context current on render thread");
        return;
    }

    ensureFbo();

    // Synchronization and rendering happens here on the render thread
    m_renderControl->sync();

    // Meanwhile on this thread continue with the actual rendering (into the FBO first).
    m_renderControl->render();
    m_context->functions()->glFlush();

    QMutexLocker quitLock(&m_quitMutex);

    image = m_fbo->toImage();
    image.convertTo(m_format);

    // The main thread can now continue
    m_cond.wakeOne();
    lock->unlock();
    m_animationDriver->advance();
}


void CoreRenderer::aboutToQuit()
{
    QMutexLocker lock(&m_quitMutex);
    m_quit = true;
}
