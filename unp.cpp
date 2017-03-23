#include "unp.h"
#include	<stdarg.h>

static int read_cnt;
static char* read_ptr;
static char read_buf[MAXLINE];




ssize_t Read(int fd, void* vptr, size_t n) {
	size_t nleft;
	ssize_t nread;
	char *ptr;
	ptr = (char *)vptr;
	nleft = n;
	while (nleft > 0) {
		if (nread = read(fd, ptr, nleft) < 0) {
			if (errno == EINTR) {
				nread = 0;
			}
			else if (nread == 0) {
				break;
			}
		}
		nleft -= nread;
		ptr += nread;
	}
	return (n - nleft);
}

ssize_t Written(int fd, const void *vptr, size_t n) {
	size_t nleft;
	ssize_t nwritten;
	const char *ptr;
	ptr = (char *)vptr;
	nleft = n;
	while (nleft > 0) {
		if (nwritten = write(fd, ptr, nleft) <= 0) {
			if (nwritten < 0 && errno == EINTR) {
				nwritten = 0;
			}
			else
			{
				return -1;
			}
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
}
static ssize_t my_read(int fd, char *ptr) {
	if (read_cnt <= 0) {
	again:
		if (read_cnt = read(fd, read_buf, sizeof(read_buf)) < 0) {
			if (errno == EINTR) {//жиЪд from man   The call was interrupted by a signal before any data was read; see signal(7).
				goto again;
			}
			return -1;
		}
		else if (read_cnt == 0)
			return 0;
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return 1;
}

ssize_t ReadLine(int fd, void* vptr, size_t maxlen) {
	ssize_t n, rc;
	char c, *ptr;
	ptr = (char *)vptr;

	for (n = 1; n < maxlen; n++) {
		if (rc = my_read(fd, &c) == 1) {
			*ptr++ = c;
			if (c == '\n') {
				break;
			}
			else if (rc == 0) {
				*ptr = 0;
				return(n - 1);
			}
			else
				return -1;
		}
	}

}