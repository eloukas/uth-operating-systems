#include "schedule.h"
#include <stdio.h>

#define DIVIDE_CONST 1000000
#define FACTOR 0.5

extern struct runqueue *rq;


/*########################## Functions aboutRq (traversal,min,max etc..) ##########################*/

//Find the process with minimum exp_burst
struct task_struct * Sjf_algo () {

    struct task_struct * curr;
    struct task_struct * tmp;
    double min_goodness;

    curr = (rq->head)->next;
    min_goodness = curr->goodness;
    tmp = curr;
    curr = curr->next;

    while (curr != rq->head) {
        if (curr->goodness < min_goodness) {
            min_goodness = curr->goodness;
            tmp = curr;
        }
        curr = curr->next;
    }

    return tmp;
}


//Find min expected burst and max waiting time in rq
void min_burst_max_waiting_in_rq(double *min_exp_burst,double *max_waiting_in_rq){

    struct task_struct *curr;

    //traverse the whole list and find the min, max
    curr = (rq->head)->next;
    *max_waiting_in_rq = curr->waiting_in_rq;
    *min_exp_burst = curr->exp_burst;

    curr = curr->next;

    while (curr != rq->head) {
        if (curr->waiting_in_rq < *max_waiting_in_rq)
        *max_waiting_in_rq = curr->waiting_in_rq;

        if (curr->exp_burst < *min_exp_burst)
        *min_exp_burst = curr->exp_burst;

        curr = curr->next;
    }
    *max_waiting_in_rq = ((sched_clock()/DIVIDE_CONST) - *max_waiting_in_rq);

    printf("Min exp_burst: %4.2lf\nMax wait_in_rq: %4.2lf\nSched_clock: %lld\n",*min_exp_burst,*max_waiting_in_rq, sched_clock()/DIVIDE_CONST);
}

/*###################################### END ######################################################*/



/*########### Functions about calculation of burst,goodness etc.. ###########*/


// Returns max{last_running_time,last_waiting_time_in_runqueue}
double calc_waiting_time_in_rq(struct task_struct *current){

    double curr_time;

    curr_time = sched_clock()/DIVIDE_CONST;

    if (curr_time > current->waiting_in_rq){
        return curr_time;
    }
    return current->waiting_in_rq;
}

// Calculation of burst
double calc_burst(struct task_struct *current){

    double curr_time = sched_clock()/DIVIDE_CONST;

    return curr_time - current->process_start_time;
}

// Calculation of expected burst
double calc_exp_burst(struct task_struct *current){

    return (current->burst + FACTOR*current->exp_burst)/(1 + FACTOR);
}

// Calculation of goodness
double calc_goodness(struct task_struct *current){

    double min_exp_burst,max_waiting_in_rq;

    // find min expected burst, max waiting time in rq
    min_burst_max_waiting_in_rq(&min_exp_burst,&max_waiting_in_rq);

    return ((1 + current->exp_burst) / (1 + min_exp_burst)) * ((1 + max_waiting_in_rq) / (1 + current->waiting_in_rq));
}

// Return starting time of process
double start_time(){

    return sched_clock()/DIVIDE_CONST;
}



/*################################# END ####################################*/
