TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src/QmlRenderer.pro cli/QmlRender.pro test/renderertest.pro
cli.depends = src
test.depends = src
