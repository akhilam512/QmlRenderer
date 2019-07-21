
An overview -

src/ - contains source code of rendering part of the library
test/ - contains unit tests and reference output frames for testing
cli/ - contains the source code for the CLI executable of the library

To build - 

mkdir build
cd build
qmake -r ..
make 
cd bin
./QmlRender -o /path/to/output/directory -i /path/to/input/QML/file