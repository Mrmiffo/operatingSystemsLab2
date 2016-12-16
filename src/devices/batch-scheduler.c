/* Tests cetegorical mutual exclusion with different numbers of threads.
 * Automatic checks only catch severe problems like crashes.
 */
#include <stdio.h>
#include "tests/threads/tests.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/timer.h"
#include "lib/random.h" //generate random numbers
#include "lib/kernel/list.h"
#include "threads/interrupt.h"

#define BUS_CAPACITY 3
#define SENDER 0
#define RECEIVER 1
#define NORMAL 0
#define HIGH 1

/*
 *	initialize task with direction and priority
 *	call o
 * */
typedef struct {
	int direction;
	int priority;
} task_t;


struct lock queue_lock;
struct condition prio_queue;
struct condition normal_queue;
int prio_queue_size;
int cur_bus_direction;
int slots;


void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive);

void senderTask(void *);
void receiverTask(void *);
void senderPriorityTask(void *);
void receiverPriorityTask(void *);


void oneTask(task_t task);/*Task requires to use the bus and executes methods below*/
void getSlot(task_t task); /* task tries to use slot on the bus */
void transferData(task_t task); /* task processes data on the bus either sending or receiving based on the direction*/
void leaveSlot(task_t task); /* task release the slot */
void queue(task_t task); /*Puts the provided task in the proper queue */


/* initializes semaphores */ 
void init_bus(void){ 
    random_init((unsigned int)123456789); 
    lock_init(&queue_lock);
    cond_init(&prio_queue);
    cond_init(&normal_queue);
    prio_queue_size = 0;
    cur_bus_direction = 0;
    slots = BUS_CAPACITY;
}

/*
 *  Creates a memory bus sub-system  with num_tasks_send + num_priority_send
 *  sending data to the accelerator and num_task_receive + num_priority_receive tasks
 *  reading data/results from the accelerator.
 *
 *  Every task is represented by its own thread. 
 *  Task requires and gets slot on bus system (1)
 *  process data and the bus (2)
 *  Leave the bus (3).
 */

void batchScheduler(unsigned int num_tasks_send, unsigned int num_task_receive,
        unsigned int num_priority_send, unsigned int num_priority_receive)
{
		unsigned int i;
		for (i = 0; i< num_priority_send; i++)
		{
			thread_create ("Prioritized send", PRI_DEFAULT,
               senderPriorityTask, NULL);
		}
		for (i = 0; i< num_priority_receive; i++)
		{
			thread_create ("Prioritized receive", PRI_DEFAULT,
               receiverPriorityTask, NULL);
		}
		for (i = 0; i< num_tasks_send; i++)
		{
			thread_create ("Unprioritized send", PRI_DEFAULT,
               senderTask, NULL);
		}
		for (i = 0; i< num_task_receive; i++)
		{
			thread_create ("Unprioritized receive", PRI_DEFAULT,
               receiverTask, NULL);
		}		
}

/* Normal task,  sending data to the accelerator */
void senderTask(void *aux UNUSED){
        task_t task = {SENDER, NORMAL};
        oneTask(task);
}

/* High priority task, sending data to the accelerator */
void senderPriorityTask(void *aux UNUSED){
        task_t task = {SENDER, HIGH};
        oneTask(task);
}

/* Normal task, reading data from the accelerator */
void receiverTask(void *aux UNUSED){
        task_t task = {RECEIVER, NORMAL};
        oneTask(task);
}

/* High priority task, reading data from the accelerator */
void receiverPriorityTask(void *aux UNUSED){
        task_t task = {RECEIVER, HIGH};
        oneTask(task);
}

/* abstract task execution*/
void oneTask(task_t task) {
  getSlot(task);
  transferData(task);
  leaveSlot(task);
}


/* task tries to get slot on the bus subsystem */
void getSlot(task_t task) 
{
	lock_acquire(&queue_lock);
	while(1)
	{
		if (task.priority == NORMAL && prio_queue_size > 0) 
		{
			queue(task);
		}
		else if (cur_bus_direction == task.direction && slots > 0) 
		{
			break;
		}
		else if (slots == BUS_CAPACITY) 
		{
			cur_bus_direction = task.direction;
			break;
		}
		else 
		{
			queue(task);
		}
	}
	slots--;
	lock_release(&queue_lock);
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
		printf("Sleeping...\n");
		long random = random_ulong();
		timer_sleep(random%100);
		printf("Waking...\n");
}

/* task releases the slot */
void leaveSlot(task_t task) 
{
	lock_acquire(&queue_lock);
	slots++;
	if (prio_queue_size >0) 
	{
		int i;
		for(i=0;i<slots;i++)
			cond_signal(&prio_queue, &queue_lock);
	}
	else
	{
	int i;
		for(i=0;i<slots;i++)
			cond_signal(&normal_queue, &queue_lock);
	} 
	lock_release(&queue_lock);
}

void queue(task_t task)
{
	if (task.priority == HIGH)
	{
		prio_queue_size++;
		cond_wait(&prio_queue,&queue_lock);
		prio_queue_size--;
	}
	else cond_wait(&normal_queue, &queue_lock);
}

