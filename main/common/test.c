#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

#define MAX_SIZE 10

int main() {
    Queue *queue = createQueue(MAX_SIZE);

    enqueue(queue, 1);
    enqueue(queue, 2);
    enqueue(queue, 3);
    enqueue(queue, 4);
    enqueue(queue, 5);

    printf("Dequeue: %d\n", dequeue(queue));
    printf("Dequeue: %d\n", dequeue(queue));

    enqueue(queue, 6);
    enqueue(queue, 7);

    printf("Dequeue: %d\n", dequeue(queue));
    printf("Dequeue: %d\n", dequeue(queue));
    printf("Dequeue: %d\n", dequeue(queue));
    printf("Dequeue: %d\n", dequeue(queue));

    // Trying to dequeue from an empty queue
    printf("Dequeue: %d\n", dequeue(queue));

    freeQueue(queue);

    uint8_t payload[120]={0};
    uint16_t l = sprintf(payload, "ret=%d\n", 88);
    printf("%d, %s", l, payload);
    return 0;
}
