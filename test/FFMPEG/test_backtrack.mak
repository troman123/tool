test_backtrace : test_backtrace.o
	gcc -o test_backtrace test_backtrace.o -rdynamic
	rm -r test_backtrace.o

test_backtrace.o : test_backtrace.c
	gcc -c test_backtrace.c

.PHONY : clean

clean :
	rm -r test_backtrace

