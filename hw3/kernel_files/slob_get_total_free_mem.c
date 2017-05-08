#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/mm/measurement.h>

extern unsigned long total_free_mem;

SYSCALL_DEFINE0(slob_get_total_free_mem){
    return(total_free_mem);
}
