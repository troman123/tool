tool: main.o
	gcc -o tool main.o


main.o: main.c
	gcc -c main.c

exec:
	./tool
