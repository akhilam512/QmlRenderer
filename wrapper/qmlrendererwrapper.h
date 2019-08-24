#ifndef QMLRENDERERWRAPPER_H
#define QMLRENDERERWRAPPER_H

#include <QObject>

class QmlRendererWrapper : public QObject
{
    Q_OBJECT
public:
    explicit QmlRendererWrapper(QObject *parent = nullptr);

signals:

public slots:
};

#endif // QMLRENDERERWRAPPER_H
