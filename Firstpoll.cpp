#include "unp.h"
using namespace std;

void main() {

	int i, maxi, listenfd, connfd, sockfd;
	int nready;
	ssize_t n;
	char buf[MAXLINE];
	socklen_t clilen;
	 pollfd client[FOPEN_MAX]; // ���ڼ������� 
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
		if (client[0].revents & POLLRDNORM) {	// new connection   �Ƚ��¼���λ
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (sockaddr*)&cliaddr, &clilen);
			for (i = 1; i < FOPEN_MAX; i++) {
				if (client[i].fd < 0) {
					client[i].fd = connfd; //��һ���µĿռ�� connfd
				}
				break;
			
			}
			if (i == FOPEN_MAX) {
			// err_quit message:
			}
			client[i].events = POLLRDNORM;		//�� ��ע���¼�Ϊ ��ͨ���ݿɶ�  ���� MAN �鿴���� ��·��� P 144
			if (i > maxi) {
				maxi = i;		//��Ӧ�µľ�Ҫ�����µ����fd��
			}
			if (--nready <= 0) {
				continue;		// ������ ���δ������¼�����      ����Ķ�д�����Ѿ���������      
			}
		}
		for (i = 1; i < maxi; i++) {			
			if ((sockfd = client[i].fd) < 0)		//	Ŀǰ��������ﻹû��������	����ֱ�Ӳ�����	
				continue;
			if (client[i].revents & (POLLRDNORM | POLLERR)) {	// ���� ͨ�� | ������ ���� & �ж� ������ �Ƿ���� ��ͨ���ݿɶ� ���� ��������
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