find_roots.c  ---> purpose: travelsal the process tree from current till init process
                  path: /usr/src/linux-3.14.62-dev/kernel

Makefile      ---> Add find_roots.o to compile it with the kernel compilation!
                  path: same as find_roots.c

sys_call_32.tbl -> Add unique id for find_roots syscall (id:353)
                  path: /usr/src/linux-3.14.62-dev/arch/x86/syscalls/syscall_32.tbl

syscalls.h    ---> Add header (prototype) of find_roots
                  path: /usr/src/linux-3.14.62-dev/include/linux.


and finally compile the kernel
sudo make
path: /usr/src/linux-3.14.62-dev
