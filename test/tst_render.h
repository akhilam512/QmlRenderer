#ifndef TST_RENDER_H
#define TST_RENDER_H

#include <memory>
#include <QObject>
#include <QtTest>
#include "qmlrenderer.h"

// add necessary includes here

class Render : public QObject
{
    Q_OBJECT

public:
    Render();
    ~Render();

private slots:
    void test_case1();
    void test_case2();
    void test_case3();
    void test_case4();
    void test_case5();
    void test_case6();
};
#endif // TST_RENDER_H
