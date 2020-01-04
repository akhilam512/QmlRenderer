TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src/QmlRenderer.pro cli/QmlRender.pro test/renderertest.pro
cli.depends = src
test.depends = src

# Copies directory containing reference_output frames and lib_output directory (used in unit test) to the build directory
copydata.commands = $(COPY_DIR) $$PWD/test/reference_output $$OUT_PWD;  $(COPY_DIR) $$PWD/test/lib_output $$OUT_PWD
first.depends = $(first) copydata
export(first.depends)
export(copydata1.commands)
QMAKE_EXTRA_TARGETS += first copydata
