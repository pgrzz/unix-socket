#include "unp.h"
using namespace std;

// learn from manSee

#define MAX_EVENTS 10



/*
	对于 ET 它只有在状态转变或者收到EAGAIN 才会通知你。所以比如 sockfd 你如果读数据 要读完直到数据为0.写也是。
	而对于LT 来说的话 如果这次没有读完下次，尽管下次调用 await 时 状态没有改变但是 依旧会通知你。 


	此时使用的是ET模式，即，边沿触发，类似于电平触发，epoll中的边沿触发的意思是只对新到的数据进行通知，而内核缓冲区中如果是旧数据则不进行通知，所以在do_use_fd函数中应该使用如下循环，才能将内核缓冲区中的数据读完。
	[cpp] view plain copy 在CODE上查看代码片派生到我的代码片
	while (1) {
	len = recv(*******);
	if (len == -1) {
	if(errno == EAGAIN)
	break;
	perror("recv");
	break;
	}
	do something with the recved data........
	}
	在上面例子中没有说明对于listen socket fd该如何处理，有的时候会使用两个线程，一个用来监听accept另一个用来监听epoll_wait，如果是这样使用的话，则listen socket fd使用默认的阻塞方式就行了，而如果epoll_wait和accept处于一个线程中，即，全部由epoll_wait进行监听，则，需将listen socket fd也设置成非阻塞的，这样，对accept也应该使用while包起来（类似于上面的recv），因为，epoll_wait返回时只是说有连接到来了，并没有说有几个连接，而且在ET模式下epoll_wait不会再因为上一次的连接还没读完而返回，这种情况确实存在，我因为这个问题而耗费了一天多的时间，这里需要说明的是，每调用一次accept将从内核中的已连接队列中的队头读取一个连接，因为在并发访问的环境下，有可能有多个连接“同时”到达，而epoll_wait只返回了一次。

*/

void setnonblocking(int sockfd) {

	int opts;
	opts = fcntl(sockfd, F_GETFL,0);
	if (opts < 0) {
		perror("fcntl(F_GETFL)");
		exit(1);
	}
	opts = (opts | O_NONBLOCK);
	if (fcntl(sockfd, F_SETFL, opts) < 0) {
		perror("fcnl(F_SETFL)");
		exit(1);
	}
}


/*
 hints.ai_flags 可用标志值 及其含义

	AI_PASSIVE  套接字将用于被动打开
	AI_CANONNAME 告知 getaddrinfo 函数返回主机的规范名字
	AI_NUMERICHOST 防止任何类型的名字到地址的映射，hostname 参数必须是一个地址串
	AI_NUMERICSERV 防止任何类型的名字到服务映射，service参数必须是一个十进制端口号数串
	AI_V4MAPPED		如果同时指定 ai_family 成员的值为 AF_INET6 那么如果没有可用的 AAAA 记录，就返回与A记录对应的 IPV4 映射的 IPV6 地址
	AI_ALL 如果同时指定 AI_V4MAPPED 那么除了返回与AAAA 记录对应的 IPV6地址外，还返回与A记录对应的 IPV4 映射的 IPV6 地址。


*/

/*
	what's means about static function in C
	In C, a static function is not visible outside of its translation unit,
*/

static int create_and_bind(char * port) {
		
	addrinfo hints;
	addrinfo * result, *rp;
	int success, sfd;
	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;	 /* 表示被动打开 */
	
	success = getaddrinfo(nullptr, port, &hints, &result); // 1 hostname 可以是一个主机名或者地址串 2 service 是一个服务名或者十进制端口号数串 
	if (success != 0) {
		perror("getaddrinfo error");
		return -1;
	}
	for (rp = result; rp != nullptr; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_addrlen);
		if (sfd == -1)
			continue;
		success = bind(sfd, rp->ai_addr, rp->ai_addrlen);
		if (success == 0) {
			// bind successful!
			break;
		}
		close(sfd);
	}
	if (rp == nullptr) {
		perror(" bind error");
	}
	freeaddrinfo(result);
	return sfd;
}

/*
	fcntl(int fd,int cmd, ... args);
	
	5个功能:
	1 :  复制一个已有的描述符(cmd = F_DUPFD 或	F_DUPFD	)
	2 :	 获取/设置文件描述符标志(cmd = F_GETFD 或 F_SETFD)
	3 :	 获取/设置文件状态标志（cmd =F_GETFL 或 F_SETFL）
	4 :  获取/设置异步I/O所有权(cmd = F_GETOWN 或 F_SETOWN)
	5 :  获取/设置记录锁(cmd = F_GETLK. F_SETLK 或 F_SETLKW)


	O_RDONLY 只读
	O_WRONLY 只写
	O_RDWR	 读写
	O_EXEC	 只执行打开
	O_SEARCH 只搜索打开目录
	O_APPEND 追加写
	O_NONBLOCK 非阻塞模式
	O_SYNC	等待写完成（数据和属性）
	O_DSYNC	等待写完成（仅数据）
	O_RSYNC 同步读和写
	O_FSYNC 等待写完成(仅 FreeBSD 和 Mac OS X)
	O_ASYNC 异步I/O （仅 FreeBSD 和 Mac OS X）
*/

static int make_socket_non_blocking(int sfd) {
	
	int flags, success;
	flags = fcntl(sfd, F_GETFL, 0);
	if (flags == -1) {
		perror("fcntl");
	}
	flags |= O_NONBLOCK;
	success = fcntl(sfd, F_SETFL, flags);
	if (success == -1) {
		perror("fcntl F_SETFL ");
		return -1;
	}
	return 0;
}

/*
The calloc() function allocates memory for an array of nmemb elements of size bytes each and returns a pointer
to the allocated memory.  The memory is set to zero.  If nmemb or size is  0,  then  calloc()  returns  either
NULL, or a unique pointer value that can later be successfully passed to free().
*/
int main() {

	int sfd, s;
	int efd;
	epoll_event event;
	epoll_event *events;
	sfd = create_and_bind(SERV_PORT);
	if (sfd == -1) {
		abort(); //   If the abort() function causes process termination, all open streams are closed and flushed.
	}
	s = make_socket_non_blocking(sfd);
	if (s == -1) {
		perror("make non_blocking");
		abort();
	}
	s = listen(sfd, BACKLOG);
	if (s == -1) {
		perror("listen");
		abort();
	}
	efd = epoll_create(0);   // default  LT
	if (efd == -1) {
		perror("epoll_create");
		abort();
	}
	event.data.fd = sfd;
	event.events = EPOLLIN | EPOLLOUT;
	s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
	if (s == -1) {
		perror("epoll_ctl listenFD error");
		abort();
	}
	events = (epoll_event*)calloc(MAX_EVENTS, sizeof(event));

	// Event Loop
	while (1) {
		int n, i;
		n = epoll_wait(efd, events, MAX_EVENTS, -1); //  -1 wait untill event hapeened
		for (int i = 0; i < n; i++) {
			
			if ((events[i].events & EPOLLERR)
				|| (events[i].events & EPOLLHUP)
				|| !(events[i].events & EPOLLIN)) {
				perror("epoll error");
				close(events[i].data.fd);
				continue;
			}

			else if (sfd == events[i].data.fd) {
				//accpet
				sockaddr  *in_addr;
				socklen_t in_len;
				int connfd;
				char hbuf[MAXLINE], sbuf[MAXLINE];
				in_len = sizeof(in_addr);
				connfd = accept(sfd, in_addr,&in_len);
				if (connfd == -1) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						break;
					}
					else
					{
						perror("conn error");
						break;
					}
				}
				s = getnameinfo(in_addr, in_len, hbuf, sizeof hbuf, sbuf, sizeof sbuf,NI_NUMERICHOST | NI_NUMERICSERV);
				if (s == 0) {
					//getnameinfo 还没看
				}
				if (s == -1) {
					abort();
				}
				event.data.fd = connfd;
				event.events = EPOLLIN | EPOLLET;				// conn  set ET
				s = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, events);
				if (s == -1) {
					perror("connfd add to events error");
					abort();
				}
				continue;
			}
			else
			{
				int done = 0;
				while (1) {
					ssize_t count;
					char buf[512];
					count = read(events[i].data.fd, buf, sizeof buf);
					if (count == -1) {
						if (errno != EAGAIN) {
							perror("read");
							done = 1;
						}
						break;
					}
					else if (count == 0) {
						// remote close
						done = 1;
						break;
					}
					s = write(events[i].data.fd, buf, count);
					if (s == -1) {
						perror("write error");
						abort();
					}

				}
				if (done) {
					close(events[i].data.fd);
				}


			}


		}
	
	}
	free(events);
	close(sfd);
	return EXIT_SUCCESS;

}


void main1() {

	epoll_event ev, events[MAX_EVENTS];

	char buf[MAXLINE];

	int listen_fd, conn_fd, nfds, epoll_fd;
	int n;
	sockaddr_in servaddr, cildaddr;
	socklen_t servalen;
	listen_fd=socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8888);
	servalen = sizeof(servaddr);
	bind(listen_fd, (sockaddr*)&servaddr, sizeof(servaddr));
	listen(listen_fd, BACKLOG);

	// epoll
	epoll_fd = epoll_create(10);
	if (epoll_fd == -1) {
		perror("epoll_create");
		exit(EXIT_FAILURE);
	}
	ev.events = EPOLLIN;		// 普通或 优先级带数据可读
	ev.data.fd = listen_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
		perror("epoll_ctl: listen_fd");
		exit(EXIT_FAILURE);
	}
	for (;;) {
		nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			perror("epoll_wait");
			exit(EXIT_FAILURE);
		}
		for (n = 0; n < nfds; ++n) {
			if (events[n].data.fd == listen_fd) {
				conn_fd = accept(listen_fd, (sockaddr*)&servaddr, &servalen);
				if (conn_fd == -1) {
					perror("accept");
					exit(EXIT_FAILURE);
				}
				setnonblocking(conn_fd);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = conn_fd;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev) == -1) {
					perror("epoll ctl: conn_fd");
					exit(EXIT_FAILURE);
				}
				else {
					// use fd
					if ((n = read(events[n].data.fd, buf, MAXLINE)) == -1) {
						
						perror("read error");
						exit(1);
					}
					else if (n == 0) {
						close(events[n].data.fd); // manSee  say will be removed auto for set
				//		epoll_ctl(listen_fd, EPOLL_CTL_DEL, events[n].data.fd, &events);
					}
					else {
						// do_use_fd 

						Written(events[n].data.fd, buf, MAXLINE);
					}


				}


			}
		}

}

