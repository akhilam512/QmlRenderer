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

#ifndef QMLCORERENDERER_H
#define QMLCORERENDERER_H

#include<qmlanimationdriver.h>
#include <QObject>
#include <QSize>
#include <QImage>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLFramebufferObject>
#include <QQuickRenderControl>
#include <QOffscreenSurface>
#include <QEvent>
#include <QtConcurrent/QtConcurrent>
#include <QQmlError>
#include <QFuture>
#include <QQuickWindow>
#include <QThread>
#include <QEventLoop>
#include <QMutex>
#include <QWaitCondition>
#include <QtCore/QAnimationDriver>

static const QEvent::Type INIT = QEvent::Type(QEvent::User + 1);
static const QEvent::Type RENDER = QEvent::Type(QEvent::User + 2);
static const QEvent::Type RESIZE = QEvent::Type(QEvent::User + 3);
static const QEvent::Type STOP = QEvent::Type(QEvent::User + 4);
static const QEvent::Type UPDATE = QEvent::Type(QEvent::User + 5);

class QmlCoreRenderer : public QObject
{
public:
    explicit QmlCoreRenderer(QObject *parent = nullptr);

    ~QmlCoreRenderer() override;

    void requestInit() { QCoreApplication::postEvent(this, new QEvent(INIT)); }
    void requestRender() { QCoreApplication::postEvent(this, new QEvent(RENDER)); }
    void requestResize() { QCoreApplication::postEvent(this, new QEvent(RESIZE)); }
    void requestStop() { QCoreApplication::postEvent(this, new QEvent(STOP)); }
    QWaitCondition *cond() { return &m_cond; }
    QMutex *mutex() { return &m_mutex; }

    void setContext(QOpenGLContext *context) { m_context = context; }
    void setSurface(QOffscreenSurface *surface) { m_offscreenSurface = surface; }
    void setQuickWindow(QQuickWindow *window) { m_quickWindow = window; }
    void setRenderControl(QQuickRenderControl* control) { m_renderControl = control; }
    void setAnimationDriver(QmlAnimationDriver* driver) { m_animationDriver = driver; }

    void setSize(QSize s) { m_size = s; }
    void setDPR(qreal value) { m_dpr = value; }
    void setFPS(int value) { m_fps = value;}
    void setFormat( QImage::Format f) { m_format = f; }

    QImage getRenderedQImage() { return m_image; }

    QImage asyncRender();
    void init();

private:
    bool event(QEvent *e) override;

    void aboutToQuit();


    void cleanup();

    void ensureFbo();

    void render(QMutexLocker *lock);

    QWaitCondition m_cond;
    QMutex m_mutex;

    QOpenGLContext* m_context;
    QOffscreenSurface* m_offscreenSurface;
    QQuickRenderControl* m_renderControl;
    QQuickWindow* m_quickWindow;
    QOpenGLFramebufferObject* m_fbo;
    QmlAnimationDriver* m_animationDriver;

    QImage::Format m_format;
    QSize m_size;
    qreal m_dpr;
    QMutex m_quitMutex;
    int m_fps;
    bool m_quit;
    QImage m_image;
};

#endif // CORERENDERER_H
