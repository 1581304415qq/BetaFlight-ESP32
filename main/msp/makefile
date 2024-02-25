
CC = gcc

SRC_FILES := app\

test: msp.o
	CC -o a.out  test.c msp.o

msp.o: msp.c msp.h
	CC -c msp.c -o msp.o

.PHONY: all a.out clean
clean:
	rm -f *.o a.out
