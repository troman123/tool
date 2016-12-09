main : HttpMain.o HttpConn.o
	$(CC) -o main HttpMain.o HttpConn.o -DHAVE_PTHREADS

HttpMain.o : HttpMain.cpp
	$(CC) -c HttpMain.cpp

HttpConn.o : HttpConn.cpp
	$(CC) -c HttpConn.cpp


.PHONY:
		clean

clean : 
	rm main *.o
