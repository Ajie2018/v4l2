1. using /dev/fb0  && using struct fb_var_vscreeninfo
	#include <linux/fb.h>
2.jpeg-exchange
	sudo apt-get install libjpeg8-dev                    //no usefull
	sudo apt-get install libjpeg62-turbo-dev             //instead package
    #include <jpeglib.h>
    gcc video.c -ljpeg	
3.about segement fault
	in complie process,ocurr a question
	a) rgbData[size]              make sure size enough
	b) video_capture_fmt          set sample screnn equal the lcd screen
	c) make all the variable about screen keep same

