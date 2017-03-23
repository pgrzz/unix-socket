#include "unp.h"
using namespace std;

// learn from manSee

#define MAX_EVENTS 10



void setnonblocking(int sockfd) {

	int opts;
	opts = fcntl(sockfd, F_GETFL);
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
void main() {

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
	servaddr.sin_port = htons(SERV_PORT);
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
						Written(events[n].data.fd, buf, MAXLINE);
					}


				}


			}
		}

}

