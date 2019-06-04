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
#include <memory>

#include <QObject>
#include <QSize>
#include <QString>
#include <QDir>
#include <QFuture>

class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOffscreenSurface;
class QQuickRenderControl;
class QQuickWindow;
class QQmlEngine;
class QQmlComponent;
class QQuickItem;
class QmlAnimationDriver;

class QMLRENDERERSHARED_EXPORT QmlRenderer: public QObject
{

    Q_OBJECT

public:
    QmlRenderer(QObject *parent = 0);
    ~QmlRenderer();

    void renderQml(const QString &qmlFile, const QString &filename, const QString &outputDirectory, const QString &outputFormat, const QSize &size, qreal devicePixelRatio = 1.0, int durationMs = 1000*5, int fps = 24, bool checkIfEntire=true, qint64 frameTime=0);
    void renderSelectFrame(); //renders single frame
    enum Status {
            NotRunning,
            Running
        };

private:
    void cleanup();

    void createFbo();
    void destroyFbo();
    bool loadQML(const QString &qmlFile, const QSize &size);
    void renderEntireQml();
    bool isRunning();
    bool event(QEvent *event) override;
    int progress() const;
    void checkComponent();
    void renderFrames();
    void renderSingleFrame();
    void renderOneFrame();
    std::unique_ptr<QOpenGLContext> m_context;
    std::unique_ptr<QOffscreenSurface> m_offscreenSurface;
    std::unique_ptr<QQuickRenderControl> m_renderControl;
    std::unique_ptr<QQuickWindow> m_quickWindow;
    std::unique_ptr<QQmlEngine> m_qmlEngine;
    std::unique_ptr<QQmlComponent> m_qmlComponent;
    std::unique_ptr<QQuickItem> m_rootItem;
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo;
    std::unique_ptr<QmlAnimationDriver> m_animationDriver;
    std::unique_ptr<QObject> rootObject;
    std::unique_ptr<QFutureWatcher<void>> watcher;
    QScopedPointer<QEvent> updateRequest;

    qreal m_dpr;
    QSize m_size;
    Status m_status;
    qint64 m_selectFrame;

    int m_progress;
    int m_duration;  // by default = 5 seconds
    int m_fps; // by default = 24 fps
    int m_frames;
    int m_currentFrame;
    QString m_outputName;
    QString m_outputFormat;
    QString m_outputDirectory;
    QString m_outputFile;
    qint64 m_frameTime;
    bool m_checkIfEntire;


    QVector<std::shared_ptr<QFutureWatcher<void>>> m_futures;
    int m_futureCounter;

private slots:
    void futureFinished();


};

#endif // QMLRENDERER_H
