OUTPUT = main.run2
OBJS =
OBJS += main.o readpkt.o muxing.o
#LDFLAGS = -L
CFLAGS = -g 
LIBS = -g `pkg-config --libs x264` `pkg-config --libs libavformat` `pkg-config --libs libavcodec` `pkg-config --libs libswscale`
#-lavutil  -lavutil -lavformat -lx264 -lmp3lame -lavcodec -lz -lavutil -lm -lswscale -lswresample -lpthread -lrt -g -rdynamic
$(OUTPUT): $(OBJS)
	gcc $(CFLAGS) $^ -o $@ $(LIBS)



.PHONY: clean


clean:
	rm -rf *.o $(OUTPUT)



