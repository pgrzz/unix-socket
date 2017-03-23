#include <stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/time.h>
#include <errno.h>
#include<netinet/in.h>
#include<strings.h>
#include<poll.h>
#include<SYS/stropts.h>
#include<limits.h>
#include<sys/epoll.h>
using namespace std;
#ifndef MAXLINE
#define MAXLINE 1024
#endif // !MAXLINE

#ifndef SERV_PORT
#define SERV_PORT 8888
#endif // !SERV_PORT

#ifndef BACKLOG
#define BACKLOG 10
#endif // !BACKLOG


ssize_t Read(int fd, void* vptr, size_t n);
ssize_t Written(int fd, const void *vptr, size_t n);
static ssize_t my_read(int fd, char *ptr);
ssize_t ReadLine(int fd, void* vptr, size_t maxlen);