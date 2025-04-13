#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#define MAX(a,b) (a)>(b)?(a):(b)
#define MIN(a,b) (a)<(b)?(a):(b)

Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        printf("\033[31mMemory allocation failed!\n\033[0m");
        exit(EXIT_FAILURE);
    }
    queue->capacity = capacity + 1; // One extra space for easier implementation
    queue->array = (uint8_t*)malloc(queue->capacity * sizeof(uint8_t));
    if (!queue->array) {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = 0;
    return queue;
}

void freeQueue(Queue* queue) {
    free(queue->array);
}

int isEmpty(Queue* queue) {
    return (queue->front == queue->rear);
}

int isFull(Queue* queue) {
    return ((queue->rear + 1) % queue->capacity == queue->front);
}

int freeSize(Queue* queue) {
    return queue->capacity - getSize(queue);
}

int getSize(Queue* queue) {
    int length = queue->rear - queue->front;
    if (length < 0) {
        length += queue->capacity;
    }
    return length;
}

bool getLine(Queue* queue, char* line, size_t size)
{
    if (isEmpty(queue))return false;
    for (size_t i = 0; i < getSize(queue); i++)
    {
        if (i >= size)return false;
        line[i] = queue->array[(queue->front + 1) % queue->capacity + i];
        if (line[i] == '\n') {
            line[i + 1] = '\0';
            popqueue(queue, i + 1);
            return true;
        }
    }
    return false;
}

void skip_empty_lines(Queue* queue)
{
    while (1) {
        if (getSize(queue) < 1)return;
        char c = queue->array[(queue->front + 1) % queue->capacity];
        if (c == '\n' || c == '\r') popqueue(queue, 1);
        else return;
    }
}

int enqueue(Queue* queue, uint8_t item) {
    if (isFull(queue)) {
        printf("Queue is full, cannot enqueue!\n");
        // exit(EXIT_FAILURE);
        return EXIT_FAILURE;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
    return 0;
}

int dequeue(Queue* queue, uint8_t* c) {
    if (isEmpty(queue)) {
        printf("Queue is empty, cannot dequeue!\n");
        return EXIT_FAILURE;
    }
    queue->front = (queue->front + 1) % queue->capacity;
    *c = queue->array[queue->front];
    return 0;
}

size_t DEQUEUE(Queue* queue, char* buf, size_t buf_size)
{
    size_t l = MIN(buf_size, getSize(queue));
    for (size_t i = 0; i < l; i++)
    {
        dequeue(queue, (uint8_t*)&buf[i]);
    }
    return l;
}

bool popqueue(Queue* queue, uint32_t size) {
    if (getSize(queue) >= size) {
        queue->front = (queue->front + size) % queue->capacity;
        return true;
    }
    return false;
}