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

#include "tst_render.h"
#include "qmlrenderer.h"
#include <QObject>
#include <QTest>
#include <memory>

Render::Render()
{          
    libDir = QDir::currentPath() + "/../lib_output";
    refDir = QDir::currentPath() + "/../reference_output";
}

Render::~Render()
{

}

void Render::test_case1()  // base initialisation test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    qDebug() << " CURRENTPATH() == " << libDir;

    m_renderer->initialiseRenderParams(QUrl(refDir + "/test.qml"), false, 0, "test_output", libDir, "jpg", QSize(1280,720), 1, 1000, 25);
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getCalculatedFramesCount()!=0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus()!=false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus()==false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus()==true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
}

void Render::test_case2() // QmlRenderer::renderEntireQml() test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    m_renderer->initialiseRenderParams(QUrl(refDir + "/test.qml"), false, 0, "test_output", libDir, "jpg", QSize(1280,720), 1, 1000, 25);

    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getCalculatedFramesCount() != 0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus() != false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus() == false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus() == true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->renderQml();

    // We wait till the signal QmlRenderer::terminate is emitted from the renderer

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( m_renderer, &QmlRenderer::terminate, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(2000);
    loop.exec();

    qDebug() << "calc frame == " <<  m_renderer->getActualFramesCount() << " " <<  m_renderer->getCalculatedFramesCount();
    QVERIFY2(m_renderer->getCalculatedFramesCount() == m_renderer->getActualFramesCount(), "RENDERING ERROR: Missing frames");
    QVERIFY2(m_renderer->getFutureCount() == m_renderer->getCalculatedFramesCount(), "FUTURE HANDLING ERROR: Enough futures were not found");

        qDebug() << " NOT ENOUGH TIME!!!";
}

void Render::test_case3() // QmlRenderer::renderSingleFrame() test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    m_renderer->initialiseRenderParams(QUrl(refDir + "/test.qml"), true, 80, "test_output", libDir, "jpg", QSize(1280,720), 1, 1000, 25);
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getCalculatedFramesCount()!=0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus()!=false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getfboStatus()==true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->renderQml();

    // We wait till the signal QmlRenderer::terminate is emitted from the renderer

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( m_renderer, &QmlRenderer::terminate, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(2000);
    loop.exec();

    QVERIFY2(m_renderer->getActualFramesCount() == m_renderer->getSelectFrame(), "FRAME NOT RENDERED: The required frame was not rendered properly");
    //QVERIFY2(m_renderer->getFutureCount() == 1, "FUTURE HANDLING ERROR: Futures not handled properly");
}

void Render::test_case4()  // QmlRenderer::cleanup() test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    m_renderer->initialiseRenderParams(QUrl(refDir + "/test.qml"), false, 0, "test_output", libDir, "jpg", QSize(1280,720), 1, 1000, 25);
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getCalculatedFramesCount() != 0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus() != false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getfboStatus() == true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->renderQml();

    // We wait till the signal QmlRenderer::terminate is emitted from the renderer

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( m_renderer, &QmlRenderer::terminate, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(2000);
    loop.exec();

    QVERIFY2(m_renderer->getCalculatedFramesCount() == m_renderer->getActualFramesCount(), "RENDERING ERROR: Missing frames");
    QVERIFY2(m_renderer->getFutureCount() == m_renderer->getActualFramesCount(), "FUTURE ERROR: Future handling went wrong");
    QVERIFY2(m_renderer->getAnimationDriverStatus() == false, "ANIMATION DRIVER ERROR: Animation driver still running");
    QVERIFY2(m_renderer->getfboStatus() == false, "FBO ERROR: Animation driver still running");
}

void Render::test_case5() // Integration test - QmlRenderer::renderEntireQml()
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    m_renderer->initialiseRenderParams(QUrl(refDir + "/test.qml"), false, 0,  "test_output", libDir,  "jpg", QSize(1280,720), 1, 1000, 25);
    m_renderer->renderQml();

    // We wait till the signal QmlRenderer::terminate is emitted from the renderer

    QTimer timer;
    timer.setSingleShot(true);
    QEventLoop loop;
    connect( m_renderer, &QmlRenderer::terminate, &loop, &QEventLoop::quit );
    connect( &timer, &QTimer::timeout, &loop, &QEventLoop::quit );
    timer.start(3000);
    loop.exec();
    qDebug() << m_renderer->getCalculatedFramesCount() << "  " <<  m_renderer->getActualFramesCount();
    QVERIFY2(m_renderer->getCalculatedFramesCount() == m_renderer->getActualFramesCount(), "RENDERING ERROR: Missing frames");

    QImage orig_frame;
    QImage lib_frame;
    orig_frame = QImage(refDir + "/output_2.jpg");
    lib_frame = QImage(libDir + "/test_output_2.jpg");
    QVERIFY2(orig_frame == lib_frame, "Rendering error");

    orig_frame = QImage(refDir + "/output_13.jpg");
    lib_frame = QImage(libDir + "/test_output_13.jpg");
    QVERIFY2(orig_frame == lib_frame, "Rendering error");

    orig_frame = QImage(refDir + "/output_25.jpg");
    lib_frame = QImage(libDir + "/test_output_25.jpg");
    QVERIFY2(orig_frame == lib_frame, "Rendering error");
}

void Render::test_case6() // integration test : QmlRenderer::renderSingleFrame()
{

    QmlRenderer *m_renderer = new QmlRenderer(this);
    m_renderer->initialiseRenderParams(QUrl(refDir + "/test.qml"), true, 800, "test_output", libDir, "jpg", QSize(1280,720), 1, 1000, 25);
    // at frameTime = 800, 20th frame is rendered

    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getCalculatedFramesCount()!=0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus()!=false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getfboStatus()==true, "FRAME BUFFER OBJECT ERROR: FBO not bound");

    m_renderer->renderQml();

    QImage orig_frame = QImage(refDir + "/output_frame_20.jpg");
    QImage lib_frame = QImage(libDir + "/test_output_20.jpg");
    QVERIFY2(orig_frame == lib_frame, "Rendering error");

}

void Render::test_case7()
{

    QmlRenderer *m_renderer = new QmlRenderer(refDir + "/test.qml");

    //TODO

    int width = 720;
    int height = 596;
    QImage::Format image_format = QImage::Format_RGBA8888;
    QImage img(width, height, image_format);
    m_renderer->render(img);

    QVERIFY2(img.save(refDir + "/lib_output_frame1.jpg") == true, "Saving Failed");
    //QVERIFY2(renderedFrame.save(libDir + "/lib_output/test_output_1.jpg") == true, "Rendered Frame Not Saved");

    QImage orig_frame = QImage(refDir + "/output_frame_1.jpg");
    // QImage orig_frame = QImage(libDir + "/reference_output/output_frame_1.jpg", "jpg");
    //QVERIFY2(orig_frame == renderedFrame, "Rendering error");

}

void Render::test_case8()
{

    QmlRenderer *m_renderer = new QmlRenderer( refDir + "\test0.qml");

    int width = 720;
    int height = 596;
    int n = 5;
    while(--n) {
        QImage img;
        img = QImage(width, height, QImage::Format_RGBA8888);
        m_renderer->render(img);
        QVERIFY2(img.save(refDir + "/lib_output_frame1.jpg") == true, "Saving Failed");
    }

}

void Render::test_case9()
{

    QmlRenderer *m_renderer = new QmlRenderer( refDir + "\test0.qml");

    int width = 720;
    int height = 596;
    int n = 5;
    while (--n) {
        QImage img;
        img = QImage(width, height, QImage::Format_RGBA8888);
        m_renderer->render(img);
        QVERIFY2(img.save(refDir + "/lib_output_frame1.jpg") == true, "Saving Failed");
        width = width + 2;
        height = height - 5;
    }

}

QTEST_MAIN(Render)
