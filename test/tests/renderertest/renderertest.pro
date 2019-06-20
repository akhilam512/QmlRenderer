QT       += core widgets qml quick opengl testlib concurrent
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle
CONFIG += c++14

TEMPLATE = app

SOURCES +=  tst_render.cpp

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../lib/release/ -lQmlRenderer
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../lib/debug/ -lQmlRenderer
else:unix: LIBS += -L$$PWD/../../../lib/ -lQmlRenderer

INCLUDEPATH += $$PWD/../../../src
DEPENDPATH += $$PWD/../../../src

HEADERS += \
    tst_render.h
