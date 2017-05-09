#include <stdio.h>
#include "syscall_wrapper.h"


int main(int argc, char* argv[] ) {

	//Print Allocated Memory
	printf("Average memory which is allocated: %lu \n",slob_get_total_alloc_mem());

	//Print Free Memory
	printf("Average memory which is free: %lu \n", slob_get_total_free_mem());

	return 0;
}