#include "unp.h"

/*
	read buf 
*/


/*
	maxFD ָ���� �����Ե�����������
*/
int Select(int maxFD, fd_set * readSet, fd_set* writeSet, fd_set* exceptSet,  struct timeval* timeout) {
		
	return select(maxFD, readSet, writeSet, exceptSet,timeout);

}

int Poll(struct pollfd * fdarray, unsigned long nfds, int timeout) {

	return poll(fdarray, nfds, timeout);

}


int main()
{
	int i, maxi, maxfd, listenfd, connfd, sockfd; //maxi ��������װ�ص� client �е�����
	int nready, client[FD_SETSIZE];   //   nready  Selector ���ѵ�������������client ����װ�� ����������� �� ����
	ssize_t bufSize;						// ������ʾ buf�Ĵ�С
	fd_set rset, allset;					// rset ��ǰ��set ,allset δ���µ�set
	char buf[MAXLINE];		
	socklen_t clilen;			//������ʾ cliaddr �Ľṹ��С
	struct sockaddr_in  cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0); // ���� AF_INET ,SOCK_STREAM ����һ�� IP_V4 TCP ��ʽ��  listenfd
	bzero(&servaddr, sizeof(servaddr));			// init
	servaddr.sin_family = AF_INET;					// IP_V4
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //ip
	servaddr.sin_port = htons(SERV_PORT);			// host
	bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr));	

	listen(listenfd, 10); // 10 �� backlog
	maxfd = listenfd; //init
	maxi = -1;		// index into	client[] array
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;
	}
	FD_ZERO(&allset);		//init   allset
	FD_SET(listenfd, &allset);		// ���� allset ��listenFD λΪ 1


	for (;;) {
		rset = allset;			//�����µ� fdset
		nready = Select(maxfd + 1, &rset, nullptr, nullptr, nullptr); //�������¼�������
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
			FD_SET(connfd, &allset);  // ���� allset �� connfd λ Ϊ 1
			if (connfd > maxfd) {
				maxfd = connfd; //for select  ��ʼʱֻ��һ�� listenfd ������һ�� ����ʱ�����һ���µ�connfd ���� �����������Ҫ�ı��ˡ� 
			}
			if (i > maxi) {
				maxi = i;  //max index in client[] array
			}
			if (--nready <= 0) {
				continue;		// no more readable descriptors ����FD_ISSET̽����
			}

		}

		for (i = 0; i <= maxi; i++) {   //		check for data
			if ((sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if (bufSize = Read(sockfd, buf, MAXLINE) == 0) {
					// connection closed by   clinet
					close(sockfd);
					FD_CLR(sockfd, &allset); //ȡ�� �����ӵ�
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




