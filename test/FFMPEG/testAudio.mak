testAudio : testAudio.o
	gcc -o testAudio testAudio.o -L /usr/local/ffmpeg/lib -lavutil -lavformat -lavcodec -lz \
    -lavutil -lm -lswscale -lpthread
	rm -r testAudio.o

testAudio.o : testAudio.c
	gcc -c testAudio.c -I /usr/local/ffmpeg/include

.PHONY: clean

clean :
	rm -r testAudio.o