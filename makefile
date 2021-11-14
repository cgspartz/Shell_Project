all: project1.o commands.o
	gcc -o shell352 project1.o commands.o

debug: project1.o commands.o
	gcc -o shell352 -g project1.o commands.o

project1.o: project1.c commands.h
	gcc -c project1.c

commands.o: commands.c commands.h
	gcc -c commands.c

clean:
	rm shell352 project1.o commands.o