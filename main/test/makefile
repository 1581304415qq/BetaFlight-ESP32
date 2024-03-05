CC = gcc
SRC_FILES = main 
MSP_FILES = ../msp
COMMON_FILES = ../common
FC_FILES = ../fc
IO_FILES = ../io
DRIVER_FILES = ../drivers

# 包含头文件目录
CFLAGS+= -I$(MSP_FILES) -I$(COMMON_FILES) -I$(IO_FILES) -I$(DRIVER_FILES)

test: queue.o msp.o  
	$(CC) $(CFLAGS) -o a.out test.c msp.o queue.o

msp.o: $(MSP_FILES)/msp.c queue.o
	$(CC) $(CFLAGS) -c $(MSP_FILES)/msp.c -o msp.o queue.o

queue.o: $(COMMON_FILES)/queue.c
	$(CC) $(CFLAGS) -c $(COMMON_FILES)/queue.c -o queue.o

.PHONY: clean
clean: 
	rm -f *.o a.out