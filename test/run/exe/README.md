For testing QmlRenderer, first run : 

$ ./QmlRender ---help

Example usage: 

$ ./QmlRender -i "/home/akhilam512/render_examples/test.qml" -o "/home/akhilam512/output" 

Please note that -i and -o are absolutely necessary, missing other arguments is okay as default values are fed. 

Default values : 
-F : fps : 25
-f : output format : jpg
-d : duration (in ms) : 1000
-S : whether to render single frame or no : false
-t : if rendering single frame at which time (in ms): 0 


Troubleshooting common errors - 

Error : error while loading shared libraries: libQmlRenderer.so: cannot open shared object file: No such file or directory

Solution : 
1 echo 'export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/path/to/QmlRenderer/lib/' >> ~/.bashrc
2 open new terminal window and try again