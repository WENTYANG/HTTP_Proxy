CC=g++
CFLAGS= -Wall

main: main.cpp helperFunction.h proxy.h
	$(CC) $(CFLAGS) -o $@ main.cpp -lpthread

clean:
	rm -rf main *.o *~ proxy.log