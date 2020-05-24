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

#ifndef QMLRENDERER_H
#define QMLRENDERER_H

#include "qmlrenderer_global.h"
#include "qmlanimationdriver.h"

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
#include <QtCore/QAnimationDriver>

#include "qmlcorerenderer.h"

typedef int32_t mlt_position;


class QmlRenderer: public QObject
{
    Q_OBJECT

public:
    explicit QmlRenderer(QString qmlFileUrlString, int fps, int duration, QObject *parent = nullptr);

    ~QmlRenderer() override;

    enum renderStatus {
        NotRunning,
        Initialised
    };

    QImage render(int width, int height, QImage::Format format);
    QImage render(int width, int height, QImage::Format format, int frame);
    void checkCurrentContex() {    m_context->currentContext() == nullptr? qDebug() << "1 Context is Null ": qDebug() << "2 A context was made current!"; }
    void checkifAnimDriverRunning() { m_animationDriver->isRunning()? qDebug() << " 1 driver running": qDebug() << "2  driver is NOT running :)"; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void initDriver();
    void resetDriver();
    void init(int width, int height, QImage::Format imageFormat);
    void loadInput();
    void polishSyncRender();
    bool loadRootObject();
    bool checkQmlComponent();
    void renderStatic();
    void renderAnimated();

    QOpenGLContext *m_context;
    QOffscreenSurface *m_offscreenSurface;
    QQuickRenderControl* m_renderControl;
    QQuickWindow *m_quickWindow;
    QQmlEngine *m_qmlEngine;
    QQmlComponent *m_qmlComponent;
    QQuickItem *m_rootItem;
    QOpenGLFramebufferObject *m_fbo;
    QmlAnimationDriver *m_animationDriver;
    QmlCoreRenderer *m_corerenderer;
    QThread *m_rendererThread;

    qreal m_dpr;
    QSize m_size;
    renderStatus m_status;
    int m_duration;
    int m_fps;
    int m_framesCount;
    mlt_position m_currentFrame;
    QUrl m_qmlFileUrl;
    QImage m_frame;
    mlt_position m_requestedFrame;
    QImage::Format m_ImageFormat;
    QImage m_img;
    mlt_position m_totalFrames;
    QWaitCondition m_cond;
    QMutex m_mutex;

signals:
    void imageReady();
};

#endif // QMLRENDERER_H
