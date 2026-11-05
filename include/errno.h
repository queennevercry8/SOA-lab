#ifndef __ERRNO_H
#define __ERRNO_H

#define EBADF 9
#define EAGAIN 11
#define ENOMEM 12
#define EACCES 13
#define EFAULT 14
#define EINVAL 22
#define ENOSYS 38
#define ENOBUFS 105

extern int errno;

#endif // __ERRNO_H