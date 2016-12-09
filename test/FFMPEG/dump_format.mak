dump : dump_format.o
	gcc -o dump dump_format.o -L /usr/local/ffmpeg/lib -lavformat -lavcodec -lz \
    -lavutil -lm -lswscale -lpthread
	rm -r dump_format.o

dump_format.o : dump_format.c
	gcc -c dump_format.c -I /usr/local/ffmpeg/include

.PHONY : clean

clean :
	--rm -r dump_format.o
