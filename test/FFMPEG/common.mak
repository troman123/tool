objects = demo.o

all : demo1 demo2 demo3


demo1 : $(objects)
	gcc -o demo1 $(objects) -L /usr/local/ffmpeg/lib -lavutil -lavformat -lavcodec -lz -lavutil -lm -lswscale -lpthread

demo2 : $(objects)
	gcc -o demo2 $(objects) -L /usr/local/ffmpeg/lib -lavutil -lavformat -lavcodec -lz -lavutil -lm -lswscale -lpthread

demo3 : $(objects)
	gcc -o demo3 $(objects) -L /usr/local/ffmpeg/lib -lavutil -lavformat -lavcodec -lz -lavutil -lm -lswscale -lpthread

test : $(objects)
	gcc -o test_swscale tutorial.c  -g `pkg-config --libs libavformat` `pkg-config --libs libavcodec` `pkg-config --libs libswscale`

exec:
	./test_swscale /mnt/hgfs/D/test/video/youkuYY.mp4 

clean:
	rm test_swscale

.PHONY : all test

#include demo.mak