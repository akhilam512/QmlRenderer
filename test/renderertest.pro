TEMPLATE = app
INCLUDEPATH += ../src
DEPENDSPATH += ../src
DESTDIR = ../bin
win32: LIBS += -L../bin
else: LIBS += -L../lib
LIBS += -lQmlRenderer

QT = core testlib qml quick
CONFIG += depend_includepath testcase
CONFIG -= app_bundle
SOURCES +=  tst_render.cpp
HEADERS += tst_render.h
