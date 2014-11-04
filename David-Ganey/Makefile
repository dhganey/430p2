all: pj02

pj02: main.o
	g++ main.o -o pj02

main.o: main.cpp main.h
	g++ -std=c++11 -c main.cpp main.h

clean:
	rm pj02
	rm *.o
	rm *.out