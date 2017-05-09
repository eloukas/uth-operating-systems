#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slob.h>

SYSCALL_DEFINE0(slob_get_total_alloc_mem){
    return(get_alloc_mem());
}
