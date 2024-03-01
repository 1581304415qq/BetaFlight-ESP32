CC = gcc
SRC_FILES = main 
MSP_FILES = ../msp
QUEUE_FILES = ../common

# 包含头文件目录
CFLAGS+= -I$(MSP_FILES)
CFLAGS+= -I$(QUEUE_FILES)

test: queue.o msp.o  
	$(CC) $(CFLAGS) -o a.out test.c msp.o queue.o

msp.o: $(MSP_FILES)/msp.c 
	$(CC) $(CFLAGS) -c $(MSP_FILES)/msp.c -o msp.o

queue.o: $(QUEUE_FILES)/queue.c
	$(CC) $(CFLAGS) -c $(QUEUE_FILES)/queue.c -o queue.o

.PHONY: clean
clean: 
	rm -f *.o a.out