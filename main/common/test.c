#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include "filter.h"
#include "maths.h"
#include <string.h>

#define MAX_SIZE 10
struct test
{
    uint8_t t1 : 1;
    uint8_t t2 : 1;
    uint8_t t3 : 1;
};


int main() {
    Queue* queue = createQueue(MAX_SIZE);

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

    uint8_t payload[120] = { 0 };
    uint16_t l = sprintf(payload, "ret=%d\n", 88);
    printf("%d, %s", l, payload);

    float sinx = sin_approx(3.4567);

    pt1Filter_t filter;
    pt1FilterInit(&filter, pt1FilterGain(1/30.0f/10.0f, 1.0/50.0f));
    for (int i = 0; i < 100; i++) {
        float ret = pt1FilterApply(&filter, i);
        printf("filter=%f\n", ret);
    }

    struct test tet={0};
    tet.t1 = 1;
    tet.t3 = 1;
    uint8_t tmp = 0;
    memcpy(&tmp, &tet, 1);
    printf("size=%d, %02x\n",sizeof(tet), tmp);
    return 0;
}
