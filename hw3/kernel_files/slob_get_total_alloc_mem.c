#include <linux/kernel.h>
#include <linux/syscalls.h>

SYSCALL_DEFINE0(slob_get_total_alloc_mem){

    return(0);
}
