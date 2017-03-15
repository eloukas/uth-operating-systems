* **_find_roots.c_**  ---> purpose: Traverse the process tree from **current** till **init** process

                       /usr/src/linux-3.14.62-dev/kernel

* **_Makefile_**     ---> Add **find_roots.o** in the Makefile to compile it with the kernel compilation!

                      path same as find_roots.c (/usr/src/linux-3.14.62-dev/kernel)

* **_sys_call_32.tbl_** ---> Add unique id for find_roots syscall (id:353)

                      /usr/src/linux-3.14.62-dev/arch/x86/syscalls/syscall_32.tbl

* **_syscalls.h_**    ---> Add header (prototype) of find_roots

                      /usr/src/linux-3.14.62-dev/include/linux


Finally, compile the kernel while you are in path: _/usr/src/linux-3.14.62-dev_
using the following command

sudo make

