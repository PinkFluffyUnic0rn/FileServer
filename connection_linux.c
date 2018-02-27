#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "globals.h"
#include "connection.h"

int listenaddr(const struct in_addr *saddr, const in_port_t *port)
{
	int listenfd;
	struct sockaddr_in servaddr;
	int reuse;

	memset(&servaddr, 0, sizeof(servaddr));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = *port;
	servaddr.sin_addr = *saddr;

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return (-1);

	reuse = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
		&reuse, sizeof(reuse)) < 0) {
		close(listenfd);

		return (-1);
	}

	if (bind(listenfd, (struct sockaddr *) (&servaddr), sizeof(servaddr))
		< 0) {
		close(listenfd);
		
		return (-1);
	}

	if (listen(listenfd, MAXCONN) < 0) {
		close(listenfd);
		
		return (-1);
	}

	return listenfd;
}

int connectionaccept(int listenfd, const struct in_addr *caddr)
{
	int datafd;
	
	while (1) {
		struct sockaddr_in aaddr;
		socklen_t addrlen;

		addrlen = sizeof(aaddr);
		
		if ((datafd = accept(listenfd, (struct sockaddr *) (&aaddr),
			&addrlen)) < 0) {
			if (errno == EINTR)
				continue;
			
			return (-1);	
		}

		if (memcmp(caddr, &(aaddr.sin_addr), sizeof(struct in_addr))
			== 0)
			break;
		else
			close(datafd);
	}
	
	close(listenfd);

	return datafd;

}

int sendcmd(int fd, char *str)
{
	char tmpstr[MAXREQLEN + 2];
	char *offset;
	size_t strsize;
	int sent;
	fd_set writefs;
	struct timeval tv;

	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		return (-1);

	strcpy(tmpstr, str);
	strcat(tmpstr, "\r\n");
	
	strsize = strlen(tmpstr);

	offset = tmpstr;

	while (offset - tmpstr != strsize) {
		int tosend;

		tosend = (tmpstr + strsize) - offset;
		
		FD_ZERO(&writefs);
		FD_SET(fd, &writefs);

		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
	
		if (select(fd + 1, NULL, &writefs, NULL, &tv) <= 0) {
			if (errno == EINTR)
				continue;
		
			return (-1);
		}
	
		if ((sent = send(fd, offset, tosend, 0)) < 0) {
			if (errno == EINTR)
				continue;
			else if (errno != EAGAIN)
				return (-1);
		}
		
		offset += sent;
	}
	
	return 0;
}

int recvcmd(int fd, char *str, int maxsize)
{
	char *offset;
	int r;
	fd_set readfs;
	struct timeval tv;

	offset = str;
	
	do {
		size_t toread;

		toread = (str + maxsize) - offset;

		FD_ZERO(&readfs);
		FD_SET(fd, &readfs);

		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
		
		if (select(fd + 1, &readfs, NULL, NULL, &tv) <= 0) {
			if (errno == EINTR)
				continue;
			
			return (-1);
		}
		
		if ((r = recv(fd, offset, toread, 0)) <= 0) {
			if (errno == EINTR)
				continue;
		
			return (-1);
		}
		
		offset += r;
	} while(!(offset - str >= 2
		&& *(offset - 2) == '\r' && *(offset - 1) == '\n'));

	*(offset - 2) = '\0';

	return 0;
}

int ftpsendfile_default(int fd, int sfd, off_t count)
{
	char buf[BUFSIZE];

	fd_set writefs;
	struct timeval tv;

	size_t bufpos;
	size_t r;
	
	bufpos = r = -1;
	while (r != 0 && count > 0) {
		size_t sent;
		
		if (bufpos == r) {
			r = read(fd, buf, BUFSIZE);
			bufpos = 0;
		}

		if (r < 0) {
			if (errno == EINTR)
				continue;

			return (-1);
		}

		FD_ZERO(&writefs);
		FD_SET(sfd, &writefs);

		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
	
		if (select(sfd + 1, NULL, &writefs, NULL, &tv) <= 0) {
			if (errno == EINTR)
				continue;
		
			return (-1);
		}

		if ((sent = send(sfd, buf + bufpos, r - sent, 0)) < 0) {
			if (errno == EINTR)
				continue;
			
			return (-1);
		}

		bufpos += sent;
		count -= sent;
	}

	return 0;
}

int ftpsendfile_linux(int fd, int sfd, off_t count)
{
	return ftpsendfile_default(fd, sfd, count);
}

int ftpsendfile(int fd, int sfd, off_t count)
{
#ifdef __linux__
	return ftpsendfile_linux(fd, sfd, count);

#else
	return ftpsendfile_default(fd, sfd, count);
#endif
}

int ftpreadfile_default(int fd, int sfd, off_t count)
{
	char buf[BUFSIZE];

	fd_set writefs;
	struct timeval tv;

	size_t bufpos;
	size_t r;
	
	bufpos = r = -1;
	while (r != 0 && count > 0) {
		size_t sent;
		
		if (bufpos == r) {
			r = read(fd, buf, BUFSIZE);
			bufpos = 0;
		}

		if (r < 0) {
			if (errno == EINTR)
				continue;

			return (-1);
		}

		FD_ZERO(&writefs);
		FD_SET(sfd, &writefs);

		tv.tv_sec = TIMEOUT;
		tv.tv_usec = 0;
	
		if (select(sfd + 1, NULL, &writefs, NULL, &tv) <= 0) {
			if (errno == EINTR)
				continue;
		
			return (-1);
		}

		if ((sent = send(sfd, buf + bufpos, r - sent, 0)) < 0) {
			if (errno == EINTR)
				continue;
			
			return (-1);
		}

		bufpos += sent;
		count -= sent;
	}

	return 0;
}

int ftpreadfile_linux(int fd, int sfd, off_t count)
{
	return ftpreadfile_default(fd, sfd, count);
}

int ftpreadfile(int fd, int sfd, off_t count)
{
#ifdef __linux__
	return ftpreadfile_linux(fd, sfd, count);

#else
	return ftpreadfile_default(fd, sfd, count);
#endif
}
