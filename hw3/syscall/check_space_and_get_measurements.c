#include <stdio.h>
#include "syscall_wrapper.h"

int main(int argc, char* argv[] ) {

	//Print Allocated Memory
	printf("Average memory which is allocated: %lu \n",get_total_alloc_mem_syscall_wrapper());

	//Print Free Memory
	printf("Average memory which is free: %lu \n", get_total_free_mem_syscall_wrapper());

	return 0;
}
