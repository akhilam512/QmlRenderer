QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

SOURCES +=  tst_render.cpp

HEADERS += \
    tst_render.h

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../lib/release/ -lQmlRenderer
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../lib/debug/ -lQmlRenderer
else:unix: LIBS += -L$$PWD/../../../lib/ -lQmlRenderer

INCLUDEPATH += $$PWD/../../../src
DEPENDPATH += $$PWD/../../../src
