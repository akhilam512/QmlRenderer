TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = src/QmlRenderer.pro cli/QmlRender.pro test/renderertest.pro
cli.depends = src
test.depends = src

# Copies directory containing reference_output frames (used in unit test) to the build directory
copydata1.commands = $(COPY_DIR) $$PWD/test/reference_output $$OUT_PWD
first.depends = $(first) copydata1
export(first.depends)
export(copydata1.commands)
QMAKE_EXTRA_TARGETS += first copydata1


# Copies target directory for library output frames which will be compared with reference_output frames (used in unit test) to the build directory
# Note that this will be an empty directory in the source.
copydata2.commands = $(COPY_DIR) $$PWD/test/lib_output $$OUT_PWD
first.depends = $(first) copydata2
export(first.depends)
export(copydata2.commands)
QMAKE_EXTRA_TARGETS += first copydata2

