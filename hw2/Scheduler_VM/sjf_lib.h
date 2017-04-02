#ifndef SJF_LIB_H
#define SJF_LIB_H

#include "schedule.h"

double calc_waiting_time_in_rq(struct task_struct *current);
double calc_burst(struct task_struct *current);
double calc_exp_burst(struct task_struct *current);
double calc_goodness(struct task_struct *current,double min_exp_burst,double max_waiting_in_rq);
double start_time();

#endif
