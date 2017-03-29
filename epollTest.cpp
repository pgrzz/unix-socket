#include "unp.h"
using namespace std;

// learn from manSee

#define MAX_EVENTS 10



/*
	���� ET ��ֻ����״̬ת������յ�EAGAIN �Ż�֪ͨ�㡣���Ա��� sockfd ����������� Ҫ����ֱ������Ϊ0.дҲ�ǡ�
	������LT ��˵�Ļ� ������û�ж����´Σ������´ε��� await ʱ ״̬û�иı䵫�� ���ɻ�֪ͨ�㡣 


	��ʱʹ�õ���ETģʽ���������ش����������ڵ�ƽ������epoll�еı��ش�������˼��ֻ���µ������ݽ���֪ͨ�����ں˻�����������Ǿ������򲻽���֪ͨ��������do_use_fd������Ӧ��ʹ������ѭ�������ܽ��ں˻������е����ݶ��ꡣ
	[cpp] view plain copy ��CODE�ϲ鿴����Ƭ�������ҵĴ���Ƭ
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
	������������û��˵������listen socket fd����δ����е�ʱ���ʹ�������̣߳�һ����������accept��һ����������epoll_wait�����������ʹ�õĻ�����listen socket fdʹ��Ĭ�ϵ�������ʽ�����ˣ������epoll_wait��accept����һ���߳��У�����ȫ����epoll_wait���м��������轫listen socket fdҲ���óɷ������ģ���������acceptҲӦ��ʹ��while�������������������recv������Ϊ��epoll_wait����ʱֻ��˵�����ӵ����ˣ���û��˵�м������ӣ�������ETģʽ��epoll_wait��������Ϊ��һ�ε����ӻ�û��������أ��������ȷʵ���ڣ�����Ϊ���������ķ���һ����ʱ�䣬������Ҫ˵�����ǣ�ÿ����һ��accept�����ں��е������Ӷ����еĶ�ͷ��ȡһ�����ӣ���Ϊ�ڲ������ʵĻ����£��п����ж�����ӡ�ͬʱ�������epoll_waitֻ������һ�Ρ�

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
 hints.ai_flags ���ñ�־ֵ ���京��

	AI_PASSIVE  �׽��ֽ����ڱ�����
	AI_CANONNAME ��֪ getaddrinfo �������������Ĺ淶����
	AI_NUMERICHOST ��ֹ�κ����͵����ֵ���ַ��ӳ�䣬hostname ����������һ����ַ��
	AI_NUMERICSERV ��ֹ�κ����͵����ֵ�����ӳ�䣬service����������һ��ʮ���ƶ˿ں�����
	AI_V4MAPPED		���ͬʱָ�� ai_family ��Ա��ֵΪ AF_INET6 ��ô���û�п��õ� AAAA ��¼���ͷ�����A��¼��Ӧ�� IPV4 ӳ��� IPV6 ��ַ
	AI_ALL ���ͬʱָ�� AI_V4MAPPED ��ô���˷�����AAAA ��¼��Ӧ�� IPV6��ַ�⣬��������A��¼��Ӧ�� IPV4 ӳ��� IPV6 ��ַ��


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
	hints.ai_flags = AI_PASSIVE;	 /* ��ʾ������ */
	
	success = getaddrinfo(nullptr, port, &hints, &result); // 1 hostname ������һ�����������ߵ�ַ�� 2 service ��һ������������ʮ���ƶ˿ں����� 
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
	
	5������:
	1 :  ����һ�����е�������(cmd = F_DUPFD ��	F_DUPFD	)
	2 :	 ��ȡ/�����ļ���������־(cmd = F_GETFD �� F_SETFD)
	3 :	 ��ȡ/�����ļ�״̬��־��cmd =F_GETFL �� F_SETFL��
	4 :  ��ȡ/�����첽I/O����Ȩ(cmd = F_GETOWN �� F_SETOWN)
	5 :  ��ȡ/���ü�¼��(cmd = F_GETLK. F_SETLK �� F_SETLKW)


	O_RDONLY ֻ��
	O_WRONLY ֻд
	O_RDWR	 ��д
	O_EXEC	 ִֻ�д�
	O_SEARCH ֻ������Ŀ¼
	O_APPEND ׷��д
	O_NONBLOCK ������ģʽ
	O_SYNC	�ȴ�д��ɣ����ݺ����ԣ�
	O_DSYNC	�ȴ�д��ɣ������ݣ�
	O_RSYNC ͬ������д
	O_FSYNC �ȴ�д���(�� FreeBSD �� Mac OS X)
	O_ASYNC �첽I/O ���� FreeBSD �� Mac OS X��
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
					//getnameinfo ��û��
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
	ev.events = EPOLLIN;		// ��ͨ�� ���ȼ������ݿɶ�
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

