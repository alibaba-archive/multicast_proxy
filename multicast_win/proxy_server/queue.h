#ifndef _QUEUE_H_
#define _QUEUE_H_ 

#define QUEUE_COUNT 16
#define QUEUE_SIZE 1024
#define ELEMENT_SIZE 2048

typedef struct node
{
	char element[ELEMENT_SIZE];
	int len;
}NODE, *PNODE;
 
typedef struct queue
{
	struct node node[QUEUE_SIZE];
	int head;
	int tail;
	int element_count;
}QUEUE, *PQUEUE;

extern struct queue pkt_queue[QUEUE_COUNT];

void init_queue(struct queue *queue);
int en_queue(struct queue *queue, char *buf, int buf_len);
int de_queue(struct queue *queue, char *buf, int *buf_len);
int empty_queue(struct queue *queue);
int full_queue(struct queue *queue);

#endif
