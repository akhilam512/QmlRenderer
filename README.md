
# QmlRenderer

A Qt library which renders QML to frames

# Introduction

QmlRenderer uses QQuickRenderControl to render QML files to frames of a given format for a given duration, FPS and DPI. 

# Getting Started

## An overview -

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

**Note** - This is a GitHub mirror (which I try to update then and now). You can always find the latest QmlRenderer [here](https://invent.kde.org/akhilkgangadharan/QmlRenderer)
