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
#include <QQmlEngine>
#include <QQmlError>
#include <QFuture>
#include <QQuickWindow>

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
    explicit QmlRenderer(QObject *parent = nullptr);
    explicit QmlRenderer(QString qmlFileUrlString, qint64 frameTime=0, qreal devicePixelRatio = 1.0, int durationMs = 1000*5, int fps = 24, QObject *parent = nullptr);
    ~QmlRenderer() override;
    enum Status {
            NotRunning,
            Initialised,
            Running,
        };

    /* @brief Initialises the render parameters - overloaded: loads a QML file template passed as a QString, used for QML MLT Producer
    */
    void initialiseRenderParams(int width = 1280, int height = 720, const QImage::Format imageFormat = QImage::Format_ARGB32_Premultiplied);

    /* @brief Initialises the render parameters - overloaded: loads a QML file template passed as a QUrl
    */
    void initialiseRenderParams(const QUrl &qmlFileUrl, bool isSingleFrame=false, qint64 frameTime=0, const QString &outputDirectory = "",  const QString &filename = "output_frame", const QString &outputFormat = "jpg", const QSize &size = QSize(1280, 720), qreal devicePixelRatio = 1.0, int durationMs = 1000*5, int fps = 24);

    /* @brief Loads QML components - overloaded: called by overloaded initialiseRenderParams which uses QUrl for loading QML
    */
    bool loadComponent(const QUrl &qmlFileUrl);

    /* @brief Loads QML components - overloaded: called by overloaded initialiseRenderParams which uses QString QML file
    */
    bool loadComponent(const QString &qmlFileText); // over loaded - used for MLT QML producer

    /* @brief Sets m_status to 'Initialised', creates FBO, installs animation driver
    */
    void prepareRenderer();

    /* @brief Uninstalls the animation driver, destroys FBO
     * @description cleanup() must be called by the program using QmlRenderer library seperately
    */
    void cleanup();

    /* @brief Begins rendering and sets m_status to 'Running;
     * @description Depending on value of m_isSingleFrame, renderSingleFrame() or renderEntireQml() is called from here
    */
    void renderQml();

    /* @brief Getter method for class members
    */
    void getAllParams();

    /* @brief Returns true if m_status is 'Initialised' and m_quickWindow's scene graph is initialised
    */
    bool getSceneGraphStatus();

    /* @brief Returns true if m_status is 'Initialised' and m_animationDriver is initialised
    */
    bool getAnimationDriverStatus();

    /* @brief Returns true if m_status is 'Initialised' and m_Fbo is bound
    */
    bool getfboStatus();

    /* @brief Returns state of m_status
    */
    int getStatus();

    /* @brief Returns m_currentFrame which is the number of frames actually rendered or atleast iterated
    */
    int getActualFramesCount(); // returns the number of frames actually rendered

    /* @brief Returns m_framesCount which is the number of frames that are supposed to be rendered (fps x duration)
    */
    int getCalculatedFramesCount();

    /* @brief Returns m_selectFrame which is the frame being rendered when m_isSingleFrame is set true
    */
    int getSelectFrame();

    /* @brief Returns the size of the QVector of futures stored in m_future
    */
    int getFutureCount();

    /* @brief Called by renderQml() when m_isSingleFrame is true, to render only a specific frame
    */
    void renderSingleFrame();

    /* @brief Called by renderQml() when m_isSingleFrame is false, to render all the frames
    */
    void renderAllFrames();

    QImage renderToQImage();

    /* @brief Called after the future finishes for a frame after rendering to save the frame in the given output directory and format
    */
    static void saveImage(const QImage &image, const QString &outputFile)
    {
        image.save(outputFile); // no need to save frames if it is not for the producer
    }

    /* @brief Returns true if m_status is 'Running'
    */
    bool isRunning();

    void render(QImage &img);

    void prepareWindow();

    void renderNext();

    void initialiseContext();

    void loadInput();

private:
    /* @brief Creates the Frame Buffer Object and sets render target to the FBO
    */
    void createFbo();

    /* @brief Resets the created Frame Buffer Object
    */
    void destroyFbo();

    /* @brief Returns true if QML components are loaded properly
    */
    bool checkIfComponentOK();

    /* @brief Called by loadComponent() to prepare m_quickWindow with the given size
    */
    bool loadQML();

    /* Overriden event() handles the Update requests sent by renderAllFrames() / renderSingleFrame() required for rendering
    */
    bool event(QEvent *event) override;

    bool loadRootObject();

    std::unique_ptr<QOpenGLContext> m_context;
    std::unique_ptr<QOffscreenSurface> m_offscreenSurface;
    std::unique_ptr<QQuickRenderControl> m_renderControl;
    std::unique_ptr<QQuickWindow> m_quickWindow;
    std::unique_ptr<QQmlEngine> m_qmlEngine;
    std::unique_ptr<QQmlComponent> m_qmlComponent;
    std::unique_ptr<QQuickItem> m_rootItem;
    std::unique_ptr<QOpenGLFramebufferObject> m_fbo;
    std::unique_ptr<QmlAnimationDriver> m_animationDriver;
    std::unique_ptr<QObject> m_rootObject;
    std::unique_ptr<QFutureWatcher<void>> watcher;
    QScopedPointer<QEvent> updateRequest;
    QVector<std::shared_ptr<QFutureWatcher<void>>> m_futures;

    qreal m_dpr;
    QSize m_size;
    Status m_status;
    int m_selectFrame;
    int m_duration;  // by default = 5 seconds
    int m_fps; // by default = 24 fps
    int m_framesCount;
    int m_currentFrame;
    int m_futureFinishedCounter;
    QString m_outputName;
    QString m_outputFormat;
    QString m_outputDirectory;
    QString m_outputFile;
    QString m_qmlFileText;
    QUrl m_qmlFileUrl;
    QImage m_frame;
    bool m_ifProducer;  //set true when render() for producer is called
    qint64 m_frameTime;
    bool m_isSingleFrame;
    QImage::Format m_ImageFormat;
    bool renderFlag;

signals:
    void finished(); 
    void terminate(); // can be used by an implementing program to connect with a slot to close the execution
                      // Currently used in QmlRender (CLI implementation of the lib) and connected with slot to quit the program
                      // Currently used in test module

private slots:
    /* @brief Slot activated when a future finishes, m_futureFinisheCounter is incremented and checks if rendering needs to be stopped, sets m_status to 'NotRunning' and emits terminate()
    */
    void futureFinished();

    /* @brief Slot activated when handling the QML file returns with warning
    */
    void displayQmlError(QList<QQmlError> warnings);

    /* @brief Slot activated when the associated Scene graph returns errors
    */
    void displaySceneGraphError(QQuickWindow::SceneGraphError error, const QString &message);
};

#endif // QMLRENDERER_H
