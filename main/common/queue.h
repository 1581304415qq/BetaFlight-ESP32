#pragma once
#include <stdint.h>

typedef struct {
    uint8_t *array;
    int front;
    int rear;
    int capacity;
} Queue;

Queue* createQueue(int capacity);
void freeQueue(Queue* queue);
int isEmpty(Queue* queue);
int isFull(Queue* queue);
void enqueue(Queue* queue, uint8_t item);
uint8_t dequeue(Queue* queue);