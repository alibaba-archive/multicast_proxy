#include <stdio.h>
#include <string.h> 
#include "queue.h"

struct queue pkt_queue[QUEUE_COUNT];

void init_queue(struct queue *queue)
{
    queue->head = queue->tail = 0;
    queue->element_count = 0;
}
#include "log.h"
int en_queue(struct queue *queue, char *buf, int buf_len)
{
    if(full_queue(queue) != 0)
    {
        return -1;
    }
    memcpy(queue->node[queue->tail].element, buf, buf_len);
    queue->node[queue->tail].len = buf_len;

    queue->tail = (queue->tail + 1) % QUEUE_SIZE;
    queue->element_count++;
    return 0;
}

int de_queue(struct queue *queue, char *buf, int *buf_len)
{
    if(empty_queue(queue) != 0)
    {
        return -1;
    }
    memcpy(buf, queue->node[queue->head].element, queue->node[queue->head].len);
    *buf_len = queue->node[queue->head].len;

    queue->head = (queue->head + 1) % QUEUE_SIZE;
    queue->element_count--;
    return 0;
}

int empty_queue(struct queue *queue)
{
    if(queue->element_count <= 0)
    {
        return 1;
    }
    return 0;
}

int full_queue(struct queue *queue)
{
    if(queue->element_count >= QUEUE_SIZE)
    {
        return 1;
    }
    return 0;
}
