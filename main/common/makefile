
CC = gcc


test: queue.o
	CC -o a.out  test.c queue.o

queue.o: queue.c queue.h
	CC -c queue.c -o queue.o

.PHONY: all a.out clean
clean:
	rm -f *.o a.out
