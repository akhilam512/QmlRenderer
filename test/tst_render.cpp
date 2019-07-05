#include "tst_render.h"
#include "qmlrenderer.h"
#include <QObject>
#include <QTest>
#include <memory>

Render::Render()
{      

}

Render::~Render()
{

}

void Render::test_case1()  // base initialisation test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    QDir rootPath = QDir::currentPath();
    m_renderer->initialiseRenderParams(QDir::cleanPath(rootPath.currentPath() + "/../sampledata/test.qml"), "test_output", QDir::cleanPath(rootPath.currentPath() + "/../sampledata/output_lib/"), "jpg", QSize(1280,720), 1, 1000, 25);
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getActualFrames()!=0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus()!=false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus()==false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus()==true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->cleanup();
}

void Render::test_case2() // QmlRenderer::renderEntireQml() test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    QDir rootPath = QDir::currentPath();
    m_renderer->initialiseRenderParams(QDir::cleanPath(rootPath.currentPath() + "/../sampledata/test.qml"), "test_output", QDir::cleanPath(rootPath.currentPath() + "/../sampledata/output_lib/"), "jpg", QSize(1280,720), 1, 1000, 25);
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getActualFrames() != 0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus() != false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus() == false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus() == true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->renderQml();
    QTest::qWait(2000); // quick fix (since futures are still running in the rendering)
    QVERIFY2(m_renderer->getActualFrames() == m_renderer->getCurrentFrame(), "RENDERING ERROR: Missing frames");


    QVERIFY2(m_renderer->getFutureCount() == m_renderer->getActualFrames(), "FUTURE ERROR: Future handling went wrong");
}

void Render::test_case3() // QmlRenderer::renderSingleFrame() test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);

    QDir rootPath = QDir::currentPath();
    m_renderer->initialiseRenderParams(QDir::cleanPath(rootPath.currentPath() + "/../sampledata/test.qml"), "test_output", QDir::cleanPath(rootPath.currentPath() + "/../sampledata/output_lib/"), "jpg", QSize(1280,720), 1, 1000, 25, true, 80);
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getActualFrames()!=0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus()!=false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus()==false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus()==true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->renderQml();
    QTest::qWait(2000); // quick fix (futures still running in rendering)
    QVERIFY2(m_renderer->getActualFrames()==1, "RENDERING ERROR: Missing frame");
    m_renderer->cleanup();
}

void Render::test_case4()  // QmlRenderer::cleanup() test
{
    QmlRenderer *m_renderer = new QmlRenderer(this);

    QDir rootPath = QDir::currentPath();
    m_renderer->initialiseRenderParams(QDir::cleanPath(rootPath.currentPath() + "/../sampledata/test.qml"), "test_output", QDir::cleanPath(rootPath.currentPath() + "/../sampledata/output_lib/"), "jpg", QSize(1280,720), 1, 1000, 25);

    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getActualFrames() != 0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus() != false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus() == false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus() == true, "FRAME BUFFER OBJECT ERROR: FBO not bound");
    m_renderer->renderQml();
    QTest::qWait(2000); // quick fix (futures still running in rendering)
    QVERIFY2(m_renderer->getActualFrames() == m_renderer->getCurrentFrame(), "RENDERING ERROR: Missing frames");
    QVERIFY2(m_renderer->getFutureCount() == m_renderer->getActualFrames(), "FUTURE ERROR: Future handling went wrong");
    m_renderer->cleanup();
    QVERIFY2(m_renderer->getAnimationDriverStatus() == false, "ANIMATION DRIVER ERROR: Animation driver still running");
    // more?
}

void Render::test_case5() // Integration test - QmlRenderer::renderEntireQml()
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    QDir rootPath = QDir::currentPath();
    m_renderer->initialiseRenderParams(QDir::cleanPath(rootPath.currentPath() + "/../sampledata/test.qml"), "output", QDir::cleanPath(rootPath.currentPath() + "/../sampledata/output_lib/"), "jpg", QSize(1280,720), 1, 1000, 25);

    m_renderer->renderQml();
    QTest::qWait(2000); // quick fix (futures still running in rendering)
    QImage orig_frame;
    QImage lib_frame;
    orig_frame = QImage(QDir::cleanPath(QDir::currentPath() + "/../sampledata/output_orig/output_1.jpg"));
    lib_frame = QImage(QDir::cleanPath(QDir::currentPath() + "/../sampledata/output_lib/output_1.jpg")); // test first frame
    QVERIFY2(orig_frame == lib_frame, "Rendering error");

    orig_frame = QImage(QDir::cleanPath(QDir::currentPath() + "/../sampledata/output_orig/output_13.jpg"));
    lib_frame = QImage(QDir::cleanPath(QDir::currentPath() + "/../sampledata/output_lib/output_13.jpg")); // test a middle frame
    QVERIFY2(orig_frame == lib_frame, "Rendering error");

    orig_frame = QImage(QDir::cleanPath(QDir::currentPath() + "/../sampledata/output_orig/output_25.jpg"));
    lib_frame = QImage(QDir::cleanPath(QDir::currentPath() + "/../sampledata/output_lib/output_25.jpg")); // test last frame
    QVERIFY2(orig_frame == lib_frame, "Rendering error");
    m_renderer->cleanup();
}

void Render::test_case6() // integration test : QmlRenderer::renderSingleFrame()
{
    QmlRenderer *m_renderer = new QmlRenderer(this);
    QDir rootPath = QDir::currentPath();
    m_renderer->initialiseRenderParams(QDir::cleanPath(rootPath.currentPath() + "/../sampledata/test.qml"), "output", QDir::cleanPath(rootPath.currentPath() + "/../sampledata/output_lib/"), "jpg", QSize(1280,720), 1, 1000, 25, true, 800); // 800ms means 20th frame for 25 fps and 1000ms duration
    QVERIFY2(m_renderer->getStatus() != m_renderer->Status::NotRunning, "STATUS ERROR : Not supposed to be running");
    QVERIFY2(m_renderer->getActualFrames()!=0, "VALUE ERROR: Frames not supposed to be zero");
    QVERIFY2(m_renderer->getSceneGraphStatus()!=false, "SCENE GRAPH ERROR: Scene graph not initialised");
    QVERIFY2(m_renderer->getAnimationDriverStatus()==false, "ANIMATION DRIVER ERROR: Driver not supposed to be running");
    QVERIFY2(m_renderer->getfboStatus()==true, "FRAME BUFFER OBJECT ERROR: FBO not bound");

    m_renderer->renderQml();

    QImage orig_frame = QImage(QDir::currentPath() + "/output_orig/output_20.jpg"); // test
    QImage lib_frame = QImage(QDir::currentPath() + "/output_lib/output_20.jpg");
    QVERIFY2(orig_frame == lib_frame, "Rendering error");

    m_renderer->cleanup();

}

QTEST_MAIN(Render)
