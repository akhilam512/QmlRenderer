TEMPLATE = lib
TARGET = QmlRenderer
QT = core qml opengl quick 
DEFINES += QMLRENDERER_LIBRARY
SOURCES += qmlrenderer.cpp qmlanimationdriver.cpp qmlcorerenderer.cpp
HEADERS += qmlrenderer.h qmlrenderer_global.h qmlanimationdriver.h \
    qmlcorerenderer.h
win32: DESTDIR = ../bin
else: DESTDIR = ../lib
