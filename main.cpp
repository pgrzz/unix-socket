#include "unp.h"

/*
	read buf 
*/


/*
	maxFD 指定了 待测试的描述符个数
*/
int Select(int maxFD, fd_set * readSet, fd_set* writeSet, fd_set* exceptSet,  struct timeval* timeout) {
		
	return select(maxFD, readSet, writeSet, exceptSet,timeout);

}

int Poll(struct pollfd * fdarray, unsigned long nfds, int timeout) {

	return poll(fdarray, nfds, timeout);

}


int main()
{
	int i, maxi, maxfd, listenfd, connfd, sockfd; //maxi 是用来看装载到 client 中的数量
	int nready, client[FD_SETSIZE];   //   nready  Selector 唤醒的描述符数量，client 用来装载 完成三次握手 的 连接
	ssize_t bufSize;						// 用来表示 buf的大小
	fd_set rset, allset;					// rset 当前的set ,allset 未更新的set
	char buf[MAXLINE];		
	socklen_t clilen;			//用来表示 cliaddr 的结构大小
	struct sockaddr_in  cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0); // 根据 AF_INET ,SOCK_STREAM 创建一个 IP_V4 TCP 形式的  listenfd
	bzero(&servaddr, sizeof(servaddr));			// init
	servaddr.sin_family = AF_INET;					// IP_V4
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //ip
	servaddr.sin_port = htons(SERV_PORT);			// host
	bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr));	

	listen(listenfd, 10); // 10 是 backlog
	maxfd = listenfd; //init
	maxi = -1;		// index into	client[] array
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;
	}
	FD_ZERO(&allset);		//init   allset
	FD_SET(listenfd, &allset);		// 设置 allset 的listenFD 位为 1


	for (;;) {
		rset = allset;			//拷贝新的 fdset
		nready = Select(maxfd + 1, &rset, nullptr, nullptr, nullptr); //产生的事件描述符
		if (FD_ISSET(listenfd, &rset)) {  // new connection
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (sockaddr*)&cliaddr, &clilen);
			for (i = 0; i < FD_SETSIZE; i++) {
				if (client[i] < 0) {
					client[i] = connfd;
					break;
				}
			}
			if (i == FD_SETSIZE) {
				// err_quit  too many clients.
			}
			FD_SET(connfd, &allset);  // 设置 allset 的 connfd 位 为 1
			if (connfd > maxfd) {
				maxfd = connfd; //for select  初始时只有一个 listenfd 当新来一个 连接时会产生一个新的connfd 所以 最大描述符就要改变了。 
			}
			if (i > maxi) {
				maxi = i;  //max index in client[] array
			}
			if (--nready <= 0) {
				continue;		// no more readable descriptors 减少FD_ISSET探测数
			}

		}

		for (i = 0; i <= maxi; i++) {   //		check for data
			if ((sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if (bufSize = Read(sockfd, buf, MAXLINE) == 0) {
					// connection closed by   clinet
					close(sockfd);
					FD_CLR(sockfd, &allset); //取消 该连接的
					client[i] = -1;

				}
				else {
					Written(sockfd, buf, bufSize);
				}
				if (--nready <= 0) {
					break;
				}
			}


		}


	}
}




