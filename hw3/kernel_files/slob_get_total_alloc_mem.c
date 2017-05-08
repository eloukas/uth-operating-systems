#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm/measurement.h>

extern unsigned long total_alloc_mem;

SYSCALL_DEFINE0(slob_get_total_alloc_mem){
    return(total_alloc_mem);
}
