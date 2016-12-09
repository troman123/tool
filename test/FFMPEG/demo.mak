demo.o : demo.h
	gcc -c demo.c -I /usr/local/ffmpeg/include

.PHONY: clean
clean:
	rm -r demo.o