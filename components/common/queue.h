#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    uint8_t* array;
    int front;
    int rear;
    int capacity;
} Queue;


#define ENQUEUE(b,d,l) do{  \
for (int i = 0; i < l; i++) \
    enqueue(b, d[i]);       \
}while (0);
size_t DEQUEUE(Queue* queue, char* buf, size_t buf_size);

Queue* createQueue(int capacity);
void freeQueue(Queue * queue);
int isEmpty(Queue * queue);
int isFull(Queue * queue);
int freeSize(Queue * queue);
int getSize(Queue * queue);
int enqueue(Queue * queue, uint8_t item);
int dequeue(Queue* queue, uint8_t* c);
bool popqueue(Queue* queue, uint32_t size);
bool getLine(Queue* queue, char* line, size_t size);
void skip_empty_lines(Queue* queue);
