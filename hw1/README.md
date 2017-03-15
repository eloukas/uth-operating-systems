* **_find_roots.c_**  ---> purpose: Traverse the process tree from **_current_** till **_init_** process

                       path:   /usr/src/linux-3.14.62-dev/kernel

* Modify  **_Makefile_**     ---> In the last line, add **_find_roots.o_** to compile the Makefile with the kernel compilation!

                       path: same as find_roots.c (/usr/src/linux-3.14.62-dev/kernel)

* Modify **_syscall_32.tbl_** ---> In the last line, add a unique id for the **_find_roots_** syscall (id:353)

                       path:   /usr/src/linux-3.14.62-dev/arch/x86/syscalls/syscall_32.tbl

* Modify **_syscalls.h_**    ---> Add header (prototype) of **_find_roots_**

                       path:  /usr/src/linux-3.14.62-dev/include/linux


Finally, compile the kernel while you are in path: _/usr/src/linux-3.14.62-dev_
using the following command:

                      sudo make

