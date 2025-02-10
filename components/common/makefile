CC = gcc

SRC_FILES = main 
INC_FILES = .

# 包含头文件目录
CFLAGS+= -I$(INC_FILES)

test: queue.o maths.o filter.o
	CC -o a.out  test.c queue.o maths.o filter.o

queue.o: queue.c
	CC $(CFLAGS) -c queue.c -o queue.o

filter.o: filter.c maths.o
	CC $(CFLAGS) -c filter.c maths.o -o filter.o

maths.o: maths.c
	CC $(CFLAGS) -c maths.c -o maths.o

# queue.o: queue.c queue.h
# 	CC -c queue.c -o queue.o

.PHONY: all a.out clean
clean:
	rm -f *.o a.out
