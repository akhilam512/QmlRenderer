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
#include "corerenderer.h"
#include <memory>

#include <QObject>
#include <QSize>
#include <QQmlEngine>
#include <QQmlError>
#include <QFuture>
#include <QQuickWindow>
#include <QThread>

class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QQuickRenderControl;
class QQmlComponent;
class QQuickItem;
class QmlAnimationDriver;

class QMLRENDERERSHARED_EXPORT QmlRenderer: public QObject
{

    Q_OBJECT

public:
    explicit QmlRenderer(QString qmlFileUrlString, qint64 frameTime=0, qreal devicePixelRatio = 1.0, int durationMs = 1000*5, int fps = 24, QObject *parent = nullptr);
    ~QmlRenderer() override;
    enum renderStatus {
            NotRunning,
            Initialised,
            Running,
        };

    QImage render(int width, int height, QImage::Format format);


private:

    void init(int width = 1280, int height = 720, QImage::Format image_format = QImage::Format_ARGB32_Premultiplied);
    void loadInput();
    // bool event(QEvent *event) override;
    void createFbo();
    bool loadRootObject();
    bool checkQmlComponent();
    QImage renderToQImage();

    void renderNext();

    void initialiseContext();

    std::shared_ptr<QOpenGLContext> m_context;
    std::shared_ptr<QOffscreenSurface> m_offscreenSurface;
    std::shared_ptr<QQuickRenderControl> m_renderControl;
    std::shared_ptr<QQuickWindow> m_quickWindow;
    std::unique_ptr<QQmlEngine> m_qmlEngine;
    std::unique_ptr<QQmlComponent> m_qmlComponent;
    std::unique_ptr<QQuickItem> m_rootItem;
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo;
    std::unique_ptr<QmlAnimationDriver> m_animationDriver;
    std::unique_ptr<QObject> m_rootObject;
    std::unique_ptr<CoreRenderer> m_corerenderer;
    QThread *m_rendererThread;
    qreal m_dpr;
    QSize m_size;
    renderStatus m_status;
    int m_selectFrame;
    int m_duration;  // by default = 5 seconds
    int m_fps; // by default = 24 fps
    int m_framesCount;
    int m_currentFrame;
    QString m_outputName;
    QString m_outputFormat;
    QString m_outputDirectory;
    QString m_outputFile;
    QString m_qmlFileText;
    QUrl m_qmlFileUrl;
    QImage m_frame;
    qint64 m_frameTime;
    QImage::Format m_ImageFormat;
    bool renderFlag;

signals:
    // TODO - implement terminate()

private slots:
    /* @brief Slot activated when handling the QML file returns with warning
    */
    void displayQmlError(QList<QQmlError> warnings);

    /* @brief Slot activated when the associated Scene graph returns errors
    */
    void displaySceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);
};

#endif // QMLRENDERER_H
