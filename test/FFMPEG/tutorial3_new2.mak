tutorial3_new2 : tutorial3_new.o
	gcc  -o tutorial3_new2 tutorial3_new.o -L /usr/local/ffmpeg2.2/lib -lavformat -lavcodec -lavutil -lz -lm -lswscale -lpthread -lSDL -g
	rm -r tutorial3_new.o

tutorial3_new.o : tutorial3_new.c
	gcc -c tutorial3_new.c -I /usr/local/ffmpeg2.2/include

.PHONY: clean

clean :
	rm -r tutorial3_new2
