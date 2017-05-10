1) **_find_roots.c_**  ---> purpose: Traverse the process tree from **_current_** till **_init_** process

                       path:   /usr/src/linux-3.14.62-dev/kernel

2) Modify  **_Makefile_**     ---> In the last line, add **_find_roots.o_** to compile the Makefile with the kernel compilation!

                       path: same as find_roots.c (/usr/src/linux-3.14.62-dev/kernel)

3) Modify **_syscall_32.tbl_** ---> In the last line, add a unique id for the **_find_roots_** syscall (id:353)

                       path:   /usr/src/linux-3.14.62-dev/arch/x86/syscalls/

4) Modify **_syscalls.h_**    ---> Add header (prototype) of **_find_roots_**

                       path:  /usr/src/linux-3.14.62-dev/include/linux

*********** DO NOT FORGET TO MOVE unistd_32.h to /usr/include/i386-linux-gnu/asm ************

5) Finally, compile the kernel while you are in path: _/usr/src/linux-3.14.62-dev_
using the following command:

                      sudo make

