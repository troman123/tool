fftest : fftest.o
	gcc -o fftest fftest.o -g -L /usr/local/ffmpeg23/lib -lavutil -lavformat -lavcodec -lz \
    -lavutil -lm -lswscale -lswresample -lpthread -lrt

fftest.o : fftest.c
	gcc -c -g fftest.c -I /usr/local/ffmpeg23/include

.PHONY: clean

clean :
	rm -r fftest.o
