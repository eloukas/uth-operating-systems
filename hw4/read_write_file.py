import os

fd_c = open('copy.txt', 'r')
fd_p = open('paste.txt', 'w')

for line in fd_c.readline:
    fd_p.write(line)
    os.fsync(fd_p)

fd_c.close()
fd_p.close()
