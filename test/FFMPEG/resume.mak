resume : resume.o
	gcc -o resume resume.o


resume.o : resume.c
	gcc -c resume.c


.PHONY : clean

clean : 
	rm resume
