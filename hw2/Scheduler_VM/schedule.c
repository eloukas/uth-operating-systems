/* schedule.c
 * This file contains the primary logic for the
 * scheduler.
 */
#include "schedule.h"
#include "macros.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "privatestructs.h"
#include "sjf_lib.h"

#define NEWTASKSLICE (NS_TO_JIFFIES(100000000))

/* Local Globals
 * rq - This is a pointer to the runqueue that the scheduler uses.
 * current - A pointer to the current running task.
 */
struct runqueue *rq;
struct task_struct *current;

/* External Globals
 * jiffies - A discrete unit of time used for scheduling.
 *			 There are HZ jiffies in a second, (HZ is
 *			 declared in macros.h), and is usually
 *			 1 or 10 milliseconds.
 */
extern long long jiffies;
extern struct task_struct *idle;

/*-----------------Initilization/Shutdown Code-------------------*/
/* This code is not used by the scheduler, but by the virtual machine
 * to setup and destroy the scheduler cleanly.
 */

 /* initscheduler
  * Sets up and allocates memory for the scheduler, as well
  * as sets initial values. This function should also
  * set the initial effective priority for the "seed" task
  * and enqueu it in the scheduler.
  * INPUT:
  * newrq - A pointer to an allocated rq to assign to your
  *			local rq.
  * seedTask - A pointer to a task to seed the scheduler and start
  * the simulation.
  */
void initschedule(struct runqueue *newrq, struct task_struct *seedTask)
{
	seedTask->next = seedTask->prev = seedTask;
	newrq->head = seedTask;
	newrq->nr_running++;
}

/* killschedule
 * This function should free any memory that
 * was allocated when setting up the runqueu.
 * It SHOULD NOT free the runqueue itself.
 */
void killschedule()
{
	return;
}

void print_me(struct task_struct *curr){ 
}


void print_rq () {
	struct task_struct *curr;

	printf("---------------------------------------------------------------------------------\n");
    printf("%-25s%-10s%-25s%-10s\n", "Name","Burst","Expected Burst","Goodness");
	curr = rq->head;
	if (curr){
        printf("%-25s%-10.2lf%-25.2lf%-10.2lf\n", curr->thread_info->processName,curr->burst,curr->exp_burst,curr->goodness);
	}
	while(curr->next != rq->head) {
		curr = curr->next;
        printf("%-25s%-10.2lf%-25.2lf%-10.2lf\n", curr->thread_info->processName,curr->burst,curr->exp_burst,curr->goodness);
	};
	printf("---------------------------------------------------------------------------------\n");
	printf("\n");
}

/*-------------Scheduler Code Goes Below------------*/
/* This is the beginning of the actual scheduling logic */

/* schedule
 * Gets the next task in the queue
 */
void schedule()
{
	static struct task_struct *nxt = NULL;
	struct task_struct *curr;

    //tmp variables in case sjf returns the same process
    double tmp_burst,tmp_exp_burst,tmp_goodness,tmp_waiting_in_rq,tmp_process_start_time;


	printf("In schedule\n");
	print_rq();

	current->need_reschedule = 0; /* Always make sure to reset that, in case *
								   * we entered the scheduler because current*
								   * had requested so by setting this flag   */

	if (rq->nr_running == 1) {
		context_switch(rq->head);
		nxt = rq->head->next;
	}
	else {

	    curr = nxt;
		nxt = nxt->next;
		if (nxt == rq->head)    /* Do this to always skip init at the head */
			nxt = nxt->next;	/* of the queue, whenever there are other  */
								/* processes available					   */
 
        // keep previous values
        tmp_burst = current->burst;
        tmp_exp_burst = current->exp_burst;
        tmp_goodness = current->goodness;
        tmp_waiting_in_rq = current->waiting_in_rq;
        tmp_process_start_time = current->process_start_time;

		// Calc burst,goodness etc.. 
        current->waiting_in_rq = calc_waiting_time_in_rq(current);
		current->burst = calc_burst(current);
		current->exp_burst = calc_exp_burst(current);    
        current->goodness = calc_goodness(current);
        
        if (curr != current){
            context_switch(curr);

		    // calc start time of current task
		    current->process_start_time = start_time();
        }
        else{
    		// Restore old values
            current->waiting_in_rq = tmp_waiting_in_rq;
		    current->burst = tmp_burst;
		    current->exp_burst = tmp_exp_burst;    
            current->goodness = tmp_goodness;
            current->process_start_time = tmp_process_start_time;
        }
	}
    print_rq();
    printf("Running process: %s\n", current->thread_info->processName);
    printf("~!!!!Done with scheduling!!!!~\n");

}


/* sched_fork
 * Sets up schedule info for a newly forked task
 */
void sched_fork(struct task_struct *p)
{
	p->time_slice = 100;

	// Initialize values to zero
	p->burst=0;
	p->exp_burst=0;
	p->goodness=0;
	p->process_start_time=0;
	p->waiting_in_rq=0;
}

/* scheduler_tick
 * Updates information and priority
 * for the task that is currently running.
 */
void scheduler_tick(struct task_struct *p)
{
	schedule();
}

/* wake_up_new_task
 * Prepares information for a task
 * that is waking up for the first time
 * (being created).
 */
void wake_up_new_task(struct task_struct *p)
{
	p->next = rq->head->next;
	p->prev = rq->head;
	p->next->prev = p;
	p->prev->next = p;

	rq->nr_running++;
}

/* activate_task
 * Activates a task that is being woken-up
 * from sleeping.
 */
void activate_task(struct task_struct *p)
{
	p->next = rq->head->next;
	p->prev = rq->head;
	p->next->prev = p;
	p->prev->next = p;

	rq->nr_running++;

    // last time it joins rq
    p->waiting_in_rq = calc_waiting_time_in_rq(p);
}

/* deactivate_task
 * Removes a running task from the scheduler to
 * put it to sleep.
 */
void deactivate_task(struct task_struct *p)
{
    // lost cpu because of system_call
    p->waiting_in_rq = calc_waiting_time_in_rq(p);
	p->prev->next = p->next;
	p->next->prev = p->prev;
	p->next = p->prev = NULL; /* Make sure to set them to NULL *
							   * next is checked in cpu.c      */

	rq->nr_running--;
}
