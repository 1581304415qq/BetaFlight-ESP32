#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

Queue* createQueue(int capacity) {
    Queue* queue = (Queue*)malloc(sizeof(Queue));
    if (!queue) {
        printf("Memory allocation failed!\n");
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
    free(queue);
}

int isEmpty(Queue* queue) {
    return (queue->front == queue->rear);
}

int isFull(Queue* queue) {
    return ((queue->rear + 1) % queue->capacity == queue->front);
}

int getSize(Queue* queue) {
    int length = queue->rear - queue->front;
    if (length < 0) {
        length += queue->capacity;
    }
    return length;
}

void enqueue(Queue* queue, uint8_t item) {
    if (isFull(queue)) {
        printf("Queue is full, cannot enqueue!\n");
        return;
    }
    queue->rear = (queue->rear + 1) % queue->capacity;
    queue->array[queue->rear] = item;
}

uint8_t dequeue(Queue* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty, cannot dequeue!\n");
        exit(EXIT_FAILURE);
    }
    queue->front = (queue->front + 1) % queue->capacity;
    return queue->array[queue->front];
}

bool popqueue(Queue* queue, uint32_t size) {
    if (getSize(queue) >= size) {
        queue->front = (queue->front + size) % queue->capacity;
        return true;
    }
    return false;
}