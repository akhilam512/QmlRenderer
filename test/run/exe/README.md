For testing QmlRenderer, first run : 

$ ./QmlRender ---help

Example:

$ ./QmlRender -i "/home/akhilam512/render_examples/test.qml" -o "/home/akhilam512/output" 

-i and -o are absolutely necessary, missing other arguments is okay as default values are fed. 

Default values : 
-F : fps : 25
-f : output format : jpg
-d : duration (in ms) : 1000
-S : whether to render single frame or no : false
-t : if rendering single frame at which time (in ms): 0 