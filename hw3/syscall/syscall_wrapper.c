#include <sys/syscall.h>
#include <unistd.h>
#include "syscall_wrapper.h"

int get_total_free_mem_syscall_wrapper(void) {
	return(syscall(__NR_sys_slob_get_total_free_mem) ) ;
}

int get_total_alloc_mem_syscall_wrapper(void) {
	return(syscall(__NR_sys_slob_get_total_alloc_mem) ) ;
}