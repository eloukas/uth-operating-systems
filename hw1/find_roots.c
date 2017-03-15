#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
                                                         
SYSCALL_DEFINE0(find_roots){                                                     
                                                                                  
   struct task_struct *tmp;                                                     
                                                                                    
   tmp = current;                                                               
                                                                                  
   while(tmp->pid!=1){                                                          
         printk("id: %d, name: %s\n", tmp->pid,tmp->comm);                          
         tmp = tmp->real_parent;                                                  
   }                                                                            
                                                                                   
   printk("id: %d, name: %s\n", tmp->pid,tmp->comm);                              
                                                                                  
   return(0);                                                                   
}            
