#include "unp.h"
using namespace std;

void main() {

	int i, maxi, listenfd, connfd, sockfd;
	int nready;
	ssize_t n;
	char buf[MAXLINE];
	socklen_t clilen;
	 pollfd client[FOPEN_MAX]; // 用于监听的组 
	sockaddr_in cliaddr, servaddr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	bind(listenfd,( sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, BACKLOG);
	client[0].fd = listenfd;
	client[0].events = POLLRDNORM;
	for (i = 1; i < FOPEN_MAX; i++) {
		client[i].fd = -1;
	}
	maxi = 0;
		
	for (;;) {
		nready = poll(client, maxi + 1, -1);
		if (client[0].revents & POLLRDNORM) {	// new connection   比较事件的位
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (sockaddr*)&cliaddr, &clilen);
			for (i = 1; i < FOPEN_MAX; i++) {
				if (client[i].fd < 0) {
					client[i].fd = connfd; //找一个新的空间放 connfd
				}
				break;
			
			}
			if (i == FOPEN_MAX) {
			// err_quit message:
			}
			client[i].events = POLLRDNORM;		//置 关注的事件为 普通数据可读  可以 MAN 查看或者 网路编程 P 144
			if (i > maxi) {
				maxi = i;		//对应新的就要更新新的最大fd数
			}
			if (--nready <= 0) {
				continue;		// 消费完 本次触发的事件总数      下面的读写触发已经不可能了      
			}
		}
		for (i = 1; i < maxi; i++) {			
			if ((sockfd = client[i].fd) < 0)		//	目前的这个槽里还没有描述符	可以直接不考虑	
				continue;
			if (client[i].revents & (POLLRDNORM | POLLERR)) {	// 这里 通过 | 操作符 加上 & 判断 操作数 是否存在 普通数据可读 或者 发生错误
				if (n = read(sockfd, buf, MAXLINE) < 0) {		
					if(errno == ECONNRESET) {
						// rst
						close(sockfd);
						client[i].fd = -1;

					}
					else {
					// err_sys
					}
				}
				else if (n == 0) {
				//close by client
					close(sockfd);
					client[i].fd = -1;
				}
				else {
					Written(sockfd, buf, n);	
				}
				if (--nready <= 0)
					break;
		}
	
	}

}