#ifndef TST_H
#define TST_H

#include <QObject>
#include <memory>
#include <QVector>

class tst : public QObject
{
    Q_OBJECT
public:
    explicit tst(QObject *parent = nullptr);
    void test();
    QVector<std::unique_ptr<int>> vec ;
signals:

public slots:
};

#endif // TST_H
