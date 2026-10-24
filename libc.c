/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

#include <errno.h>

int errno;

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

void perror(void){
  char *buff; 
  if(errno == EBADF) buff = "Bad file descriptor\n";
  else if (errno == EACCES) buff = "Permission denied\n";
  else if (errno == EFAULT) buff = "Bad address\n";
  else if (errno == EINVAL) buff = "Invalid argument\n";
  else if (errno == ENOSYS) buff = "Function not implemented\n";
  else if (errno == ENOBUFS) buff = "No buffer space available\n";
  else if(errno == NULL) return;
  write(1, buff, strlen(buff));
}

