#include "tst_render.h"
#include <QtTest>

// add necessary includes here


render::render()
{
    m_renderer = std::make_unique<QmlRenderer>(new QmlRenderer);
}

render::~render()
{

}

void render::test_case1()
{

}

QTEST_APPLESS_MAIN(render)

