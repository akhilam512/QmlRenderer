
# QmlRenderer

QmlRenderer renders .qml files to video frames

## How it works

Using QQuickRenderControl to render Qt Quick Content, QmlRenderer renders static and animated QML files to video frames of a specified format, duration, FPS and DPI.

QmlRenderer uses a custom QAnimationDriver to overcome the limits of QML render rate which is limited to the screen's refresh rate to allow rendering of Qt Quick Content at a custom frame rate.

The render loop in a nutshell works like this:  Using QQuickRenderControl, we render the contents of the Qt Quick Scenegraph onto a frame buffer object, save it and then advance the animation driver to move to the next frame at the prescribed frame rate and continue.


The rendered frames can then be picked up by multimedia frameworks like [MLT](https://www.mltframework.org/) or FFmpeg to 'play' QML videos. Currently, the code is optimised to work in use with a [MLT module](https://github.com/akhilam512/mlt) The latest version on the branch "multithreaded_mlt" renders QML on a separate render thread to avoid OpenGL context conflicts while running within other QML components/applications.

## Getting Started

### An overview -

src/ - contains source code of rendering part of the library

test/ - contains unit tests and reference output frames for testing

cli/ - contains the source code for the CLI executable of the library

## To build - 

mkdir build

cd build

qmake -r ..

make 

cd bin

./QmlRender -o /path/to/output/directory -i /path/to/input/QML/file

## For options - 

./QmlRender --help

