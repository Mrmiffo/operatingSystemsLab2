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

int cur_bus_direction = -1;
int on_bus = 0;
struct lock queue_lock;
struct condition prio_queue_send;
struct condition non_prio_queue_send;
struct condition prio_queue_receive;
struct condition non_prio_queue_receive;


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
int prio_in_direction(int direction);


/* initializes semaphores */ 
void init_bus(void){ 
    random_init((unsigned int)123456789); 
    lock_init (&queue_lock);
    cond_init(&prio_queue_send);
    cond_init(&prio_queue_receive);
    cond_init(&non_prio_queue_send);
    cond_init(&non_prio_queue_receive);
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
			//printf("Created thread: %d\n",thread_create ("Prioritized send", 0,
        //       senderPriorityTask, NULL));
 thread_create ("Prioritized send", 0,
               senderPriorityTask, NULL);
		}
		for (i = 0; i< num_priority_receive; i++)
		{
			//printf("Created thread: %d\n",thread_create ("Prioritized receive", 0,
               //receiverPriorityTask, NULL));
	thread_create ("Prioritized receive", 0,
               receiverPriorityTask, NULL);
		}
		for (i = 0; i< num_tasks_send; i++)
		{
		//	printf("Created thread: %d\n",thread_create ("Unprioritized send", 0,
      //         senderTask, NULL));
			thread_create ("Unprioritized send", 0,
               senderTask, NULL);
		}
		for (i = 0; i< num_task_receive; i++)
		{
			//printf("Created thread: %d\n",thread_create ("Unprioritized receive", 0,
      //         receiverTask, NULL));
			thread_create ("Unprioritized receive", 0,
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
	if (task.priority == HIGH)
	{
		lock_acquire(&queue_lock);
		if (on_bus < BUS_CAPACITY && (cur_bus_direction == task.direction || cur_bus_direction == -1))
		{
			//Do nothing
		} 
		else 
		{
			if(task.direction == SENDER) cond_wait(&prio_queue_send, &queue_lock);
			else cond_wait(&prio_queue_receive, &queue_lock);
		}
	} 
	else 
	{
		lock_acquire(&queue_lock);
		if (on_bus < BUS_CAPACITY && (cur_bus_direction == task.direction || cur_bus_direction == -1) 
		&& !list_size(&prio_queue_receive.waiters) && !list_size(&prio_queue_send.waiters))
		{
			//Do nothing		
		} 
		else 
		{
			if(task.direction == SENDER) cond_wait(&non_prio_queue_send, &queue_lock);
			else cond_wait(&non_prio_queue_receive, &queue_lock);
		}
	}
	cur_bus_direction = task.direction;
	on_bus++;
	lock_release(&queue_lock);
	//printf("Got slot%d\n", thread_current()->tid);
}

/* task processes data on the bus send/receive */
void transferData(task_t task) 
{
		//printf("Thread: %d going to sleep...\n", thread_current()->tid);
		long random = random_ulong();
		timer_sleep(random%4000);
		//printf("Thread: %d waking up...\n",thread_current()->tid);
}

/* task releases the slot */
void leaveSlot(task_t task) 
{

	lock_acquire(&queue_lock);
	//printf("Unlocking thread:%d\n",thread_current()->tid);
	on_bus--;
	
	int other_direction = (cur_bus_direction+1)%2;
	
	size_t prio_current = get_prio_direction(cur_bus_direction);
	size_t prio_other   = get_prio_direction(other_direction);
	size_t norm_current = get_norm_direction(cur_bus_direction);
	size_t norm_other   = get_norm_direction(other_direction);
	
	
	
	if(prio_current > 0)
	{
		wake_direction(HIGH, cur_bus_direction);
	}
	else if (prio_other > 0)
	{
		if (on_bus == 0)
		{
			wake_direction(HIGH, other_direction);
			cur_bus_direction = other_direction;
		}
	}
	else if (norm_current > 0)
	{
		wake_direction(NORMAL, cur_bus_direction);
	}
	else if (norm_other > 0)
	{
		if(on_bus == 0)
		{	
			wake_direction(NORMAL, other_direction);
			cur_bus_direction = other_direction;
		}
	}
	else
	{
		if(on_bus == 0)
			cur_bus_direction = -1;
	}
	//printf("Current dir: %d\n", cur_bus_direction);
	printf("On bus: %d\n",on_bus);
	lock_release(&queue_lock);
	//printf("Thread exited...%d\n", thread_current()->tid);
}

int get_prio_direction(int direction)
{
	if(direction == SENDER)
		return list_size(&prio_queue_send.waiters);
	else
		return list_size(&prio_queue_receive.waiters);
}

int get_norm_direction(int direction)
{
	if(direction == SENDER)
		return list_size(&non_prio_queue_send.waiters);
	else
		return list_size(&non_prio_queue_receive.waiters);
}

void wake_direction(int prio, int direction)
{
	size_t bus_slots = BUS_CAPACITY - on_bus;
	size_t i;
	struct condition* to_wake;
	if(prio == HIGH && direction == SENDER)
		to_wake = &prio_queue_send;
	if(prio == HIGH && direction == RECEIVER)
		to_wake = &prio_queue_receive;
	if(prio == NORMAL && direction == SENDER)
		to_wake = &non_prio_queue_send;
	if(prio == NORMAL && direction == RECEIVER)
		to_wake = &non_prio_queue_receive;
		
	for(i=0;i<bus_slots;i++)
		cond_signal(to_wake, &queue_lock);
}


int prio_in_direction(int direction){
	if (direction == SENDER)
	{
		return list_size(&prio_queue_send.waiters);
	}
	else
	{
		return list_size(&prio_queue_receive.waiters);
	}
}
