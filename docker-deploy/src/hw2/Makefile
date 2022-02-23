CC=g++
CFLAGS= -ggdb3 -Wall

all: main

main: main.cpp helperFunction.cpp proxy.cpp parse.cpp cache.cpp
	$(CC) $(CFLAGS) -o $@ main.cpp helperFunction.cpp proxy.cpp parse.cpp cache.cpp -pthread

clean:
	rm -rf main *.o *~