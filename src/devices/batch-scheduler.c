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
typedef struct task_s {
	int direction;
	int priority;
} task_t;

/*
 * node for list of tasks.
 *
 * */
typedef struct {
  struct list_elem e;
  struct task_s task;
} task_list_node;


struct list queue;
struct list_elem *endOfPrio;
int curBusDirection = -1;
struct semaphore busSlots;
struct lock queueLock;

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



/* initializes semaphores */ 
void init_bus(void){ 
    random_init((unsigned int)123456789); 
    list_init(&queue);
    endOfPrio = list_head(&queue);
    sema_init (&busSlots, 3);
    lock_init (&queueLock);
    /* FIXME implement */

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
			thread_create ("Prioritized send", 0,
               senderPriorityTask, NULL); 
		}
		for (i = 0; i< num_priority_receive; i++)
		{
			thread_create ("Prioritized receive", 0,
               receiverPriorityTask, NULL); 
		}
		for (i = 0; i< num_tasks_send; i++)
		{
			thread_create ("Unprioritized send", 0,
               senderTask, NULL); 
		}
		for (i = 0; i< num_task_receive; i++)
		{
			thread_create ("Unprioritized receive", 0,
               receiverTask, NULL); 
		}
		printf("Threads created");
		
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
	
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
		//printf("DEBUGMSG ENTER: %s\n", thread_current()->name);
		//printf("DEBUGMSG DIRECTION: %d\n", task.direction);
		//printf("DEBUGMSG PRIORITY: %d\n", task.priority);
		long random = random_ulong();
		//printf("DEBUGMSG SLEEPING: %ld\n", random);
		timer_sleep(random);
}

/* task releases the slot */
void leaveSlot(task_t task) 
{

		//release slot
		//check if last
			//set direction
		//release lock
		//wake waiter
    //msg("NOT IMPLEMENTED");
    /* FIXME implement */
}
