#include "schedule.h"

#define DIVIDE_CONST 1000000
#define FACTOR 0.5

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

double calc_goodness(struct task_struct *current,double min_exp_burst,double max_waiting_in_rq){

    return ((1 + current->exp_burst) / (1 + min_exp_burst)) * ((1 + max_waiting_in_rq) / (1 + current->waiting_in_rq));
}
