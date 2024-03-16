CC = gcc
SRC_FILES = main 
INCL_FILES = .

# 包含头文件目录
CFLAGS+= -I$(INCL_FILES)

test: pid.o 
	$(CC) $(CFLAGS) -o a.out test.c pid.o

pid.o: $(INCL_FILES)/pid.c
	$(CC) $(CFLAGS) -c $(INCL_FILES)/pid.c

.PHONY: clean
clean: 
	rm -f *.o a.out