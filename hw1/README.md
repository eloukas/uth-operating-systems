* **_find_roots.c_**  ---> purpose: Traverse the process tree from **current** till **init** process

                       /usr/src/linux-3.14.62-dev/kernel

* Modify  **_Makefile_**     ---> In the last line, add **find_roots.o** to compile the Makefile with the kernel compilation!

                      path same as find_roots.c (/usr/src/linux-3.14.62-dev/kernel)

* Modify **_syscall_32.tbl_** ---> In the last line, add unique id for find_roots syscall (id:353)

                      /usr/src/linux-3.14.62-dev/arch/x86/syscalls/syscall_32.tbl

* Modify **_syscalls.h_**    ---> Add header (prototype) of find_roots

                      /usr/src/linux-3.14.62-dev/include/linux


Finally, compile the kernel while you are in path: _/usr/src/linux-3.14.62-dev_
using the following command:

                      sudo make

