#include <sys/syscall.h>
#include <unistd.h>
#include "syscall_wrapper.h"

unsigned long get_total_free_mem_syscall_wrapper(void) {
	return(syscall(__NR_slob_get_total_free_mem) ) ;
}

unsigned long get_total_alloc_mem_syscall_wrapper(void) {
	return(syscall(__NR_slob_get_total_alloc_mem) ) ;
}
