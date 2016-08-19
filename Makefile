CFLAGS += -Wall -g
CFLAGS += -lpthread `pkg-config --cflags libavcodec` `pkg-config --cflags libavformat` `pkg-config --cflags libavutil` `pkg-config --cflags libavfilter`

LDFLAGS = `pkg-config --libs libavcodec` `pkg-config --libs libavformat` `pkg-config --libs  libswresample` `pkg-config --libs libavfilter`
CC=gcc

transcode: transcoding.o
	gcc -o $@ $^ ${LDFLAGS}

audio: audiodecoder.o
	gcc -o audio audiodecoder.o ${CFLAGS} ${LDFLAGS}

tool: main.o http.o util.o threadpool.o base64.o urldecode.o mp4parser.o ffmpeg/test/utils.o ffmpeg/test/videogen.o
	gcc  -o tool main.o http.o util.o threadpool.o base64.o urldecode.o mp4parser.o ffmpeg/test/utils.o ffmpeg/test/videogen.o ${CFLAGS}


main.o: main.c
	gcc -c ${CFLAGS} main.c

http.o: http.c
	gcc -c ${CFLAGS} http.c

util.o: util.c
	gcc -c ${CFLAGS} util.c

threadpool.o: threadpool.c
	gcc -c ${CFLAGS} threadpool.c

base64.o: base64.c
	gcc -c ${CFLAGS} base64.c

urldecode.o: urldecode.c
	gcc -c ${CFLAGS} urldecode.c

mp4parser.o: mp4parser.c
	gcc -c ${CFLAGS} mp4parser.c

ffmpeg/test/utils.o: ffmpeg/test/utils.c
	gcc -c ${CFLAGS} ffmpeg/test/utils.c -o ffmpeg/test/utils.o

ffmpeg/test/videogen.o: ffmpeg/test/videogen.c
	gcc -c ${CFLAGS} ffmpeg/test/videogen.c -o ffmpeg/test/videogen.o

audiodecoder.o: audiodecoder.c
	gcc -c ${CFLAGS} audiodecoder.c 

exec:
	./tool
