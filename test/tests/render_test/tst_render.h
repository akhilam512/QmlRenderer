#ifndef TST_RENDER_H
#define TST_RENDER_H

#include <QObject>
#include "qmlrenderer.h"

class render : public QObject
{
    Q_OBJECT

public:
    render();
    ~render();

private:
    std::unique_ptr<QmlRenderer> m_renderer;

private slots:
    void test_case1();


};

#endif // TST_RENDER_H
