#ifndef SJF_LIB_H
#define SJF_LIB_H

#include "schedule.h"

struct task_struct * Sjf_algo ();

double calc_waiting_time_in_rq(struct task_struct *current);
double calc_burst(struct task_struct *current);
double calc_exp_burst(struct task_struct *current);
void calc_goodness();
double start_time();

void min_burst_max_waiting_in_rq(double *min_exp_burst,double *max_waiting_in_rq);

#endif
