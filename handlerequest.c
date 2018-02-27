#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <locale.h>

#include "globals.h"
#include "struct.h"
#include "connection.h"

#include "handlerequest.h"

int handletype(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char resp[MAXREQLEN];

	if (strcmp(arg[0], "I") == 0) {
		sstate->dtype = DT_IMAGE;
		strcpy(resp, "200 Switching to image mode");
	} else if (strcmp(arg[0], "A") == 0) {
		sstate->dtype = DT_ASCII;
		strcpy(resp, "200 Switching to ASCII mode");
	} else {
		
	}	
	
	if (sendcmd(sstate->cmdfd, resp) < 0)
		return (-1);

	return 0;
}

int handlemode(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	printf("MODE command recieved.\n");
	
	if (sendcmd(sstate->cmdfd, "202 Command is not supported.") < 0)
		return (-1);

	return 0;
}

int handlestru(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	printf("STRU command recieved.\n");
	
	if (sendcmd(sstate->cmdfd, "202 Command is not supported.") < 0)
		return (-1);

	return 0;
}

int handleuser(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	if (sendcmd(sstate->cmdfd, "230 Login successful.") < 0)
		return (-1);

	return 0;
}

int handlepass(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	printf("PASS command recieved.\n");
	
	if (sendcmd(sstate->cmdfd, "202 Command is not supported.") < 0)
		return (-1);

	return 0;
}

int handlequit(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	if (sendcmd(sstate->cmdfd, "221 Closing control connection.") < 0)
		return (-1);

	return 1;
}

int handleport(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	printf("PORT command recieved.");

	if (sendcmd(sstate->cmdfd, "202 Command is not supported.") < 0)
		return (-1);

	return 0;
}

int handlepasv(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char *saddr1, *saddr2, *saddr3, *saddr4;
	char straddr[32];
	char resp[MAXREQLEN];
	
	struct sockaddr_in saddr;
	in_port_t port;
	socklen_t addrlen;

	addrlen = sizeof(saddr);	
	if (getsockname(sstate->cmdfd, (struct sockaddr *) (&saddr), &addrlen)
		< 0)
		goto error;

	if (inet_ntop(AF_INET, &(saddr.sin_addr), straddr, addrlen) == NULL)
		goto error;
	
	port = htons(sstate->dataport);

	if (sstate->ldatafd >= 0)
		close(sstate->ldatafd);

	if ((sstate->ldatafd = listenaddr(&(saddr.sin_addr), &port)) < 0)
		goto error;

	saddr1 = strtok(straddr, ".");
	saddr2 = strtok(NULL, ".");
	saddr3 = strtok(NULL, ".");
	saddr4 = strtok(NULL, ".");

	snprintf(resp, MAXREQLEN,
		"227 Entering passive Mode (%s,%s,%s,%s,%d,%d).",
		saddr1, saddr2, saddr3, saddr4,
		((int16_t)(sstate->dataport) & 0xff00) >> 8,
		(int16_t)(sstate->dataport) & 0x00ff);
	
	if (sendcmd(sstate->cmdfd, resp) < 0)
		return (1);

	return 0;

error:
	if (sendcmd(sstate->cmdfd, "425 Cannot open connection.") < 0)
		return (-1);

	return 0;
}

int handleretr(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char filepath[PATH_MAX];
	struct stat st;
	int datafd;
	int flags;
	int fd;

	struct sockaddr_in caddr;
	socklen_t addrlen;

	strcpy(filepath, sstate->curdir);
	strcat(filepath, "/");
	strcat(filepath, arg[0]);
	
	if ((fd = open(filepath, O_RDONLY)) < 0)
		goto fileopenerror;

	if (fstat(fd, &st) < 0)
		goto fileopenerror;
	
	if ((st.st_mode & S_IFMT) != S_IFREG) {
		close(fd);
		goto fileopenerror;
	}
	
	addrlen = sizeof(caddr);	
	if (getpeername(sstate->cmdfd, (struct sockaddr *) (&caddr), &addrlen)
		< 0) 
		goto accepterror;

	if ((datafd = connectionaccept(sstate->ldatafd, &(caddr.sin_addr)))
		< 0)
		goto accepterror;

	if (sendcmd(sstate->cmdfd, "150 Here comes file.") < 0)
		return (-1);

/*
	if (sendfile(datafd, fd, NULL, st.st_size) < 0)
		goto sendfileerror;
*/
	
	flags = fcntl(datafd, F_GETFL);
	fcntl(datafd, F_SETFL, flags | O_NONBLOCK);

	if (ftpsendfile(fd, datafd, st.st_size) < 0)
		goto sendfileerror;

	close(datafd);
	
	if (sendcmd(sstate->cmdfd, "226 Transfer complete.") < 0)
		return (-1);

	return 0;

fileopenerror:
	if (sendcmd(sstate->cmdfd, "550 Cannot open file.") < 0)
		return (-1);

	return 0;

accepterror:
	if (sendcmd(sstate->cmdfd, "425 Cannot open connection.") < 0)
		return (-1);

	return 0;

sendfileerror:
	close(datafd);

	if (sendcmd(sstate->cmdfd, "426 Cannot send data.") < 0)
		return (-1);

	return 0;
}

int handlestor(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char filepath[PATH_MAX];
	struct stat st;
	int datafd;
	int flags;
	int fd;

	struct sockaddr_in caddr;
	socklen_t addrlen;

	strcpy(filepath, sstate->curdir);
	strcat(filepath, "/");
	strcat(filepath, arg[0]);
	
	if ((fd = open(filepath, O_CREAT | O_TRUNC | O_WRONLY)) < 0)
		goto fileopenerror;

	if (fstat(fd, &st) < 0)
		goto fileopenerror;
	
	if ((st.st_mode & S_IFMT) != S_IFREG) {
		close(fd);
		goto fileopenerror;
	}
	
	addrlen = sizeof(caddr);	
	if (getpeername(sstate->cmdfd, (struct sockaddr *) (&caddr), &addrlen)
		< 0) 
		goto accepterror;

	if ((datafd = connectionaccept(sstate->ldatafd, &(caddr.sin_addr)))
		< 0)
		goto accepterror;

	if (sendcmd(sstate->cmdfd, "150 Here comes file.") < 0)
		return (-1);

	flags = fcntl(datafd, F_GETFL);
	fcntl(datafd, F_SETFL, flags | O_NONBLOCK);

	if (ftpreadfile(fd, datafd, st.st_size) < 0)
		goto readfileerror;

	close(datafd);
	
	if (sendcmd(sstate->cmdfd, "226 Transfer complete.") < 0)
		return (-1);

	return 0;

fileopenerror:
	if (sendcmd(sstate->cmdfd, "550 Cannot open file.") < 0)
		return (-1);

	return 0;

accepterror:
	if (sendcmd(sstate->cmdfd, "425 Cannot open connection.") < 0)
		return (-1);

	return 0;

readfileerror:
	close(datafd);

	if (sendcmd(sstate->cmdfd, "426 Cannot read data.") < 0)
		return (-1);

	return 0;
}

int handlepwd(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char resp[MAXREQLEN];

	snprintf(resp, MAXREQLEN, "257 \"%s\" is the current directory.",
		sstate->curdir);

	if (sendcmd(sstate->cmdfd, resp) < 0)
		return (-1);

	return 0;
}

void changedir(char *dir, const char *arg)
{
	if (arg[0] == '/')
		strcpy(dir, arg);
	else {
		if (dir[strlen(dir) - 1] != '/')
			strcat(dir, "/");
				
		strcat(dir, arg);
	}
}

int checkpath(const char *path)
{
	struct stat s;

	if (stat(path, &s) < 0 || !S_ISDIR(s.st_mode))
		return 0;

	return 1;
}

int handlecwd(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char tmpdir[PATH_MAX];

	strcpy(tmpdir, sstate->curdir);

	changedir(tmpdir, arg[0]);

	if (!checkpath(tmpdir)) {
		if (sendcmd(sstate->cmdfd, "550 Path doesn't exist.") < 0)
			return (-1);
	
		return 0;
	}
	
	strcpy(sstate->curdir, tmpdir);

	if (sendcmd(sstate->cmdfd, "250 Directory successfully changed.") < 0)
		return (-1);

	return 0;
}

int handlecdup(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	char tmpdir[PATH_MAX];
	char *curc;

	strcpy(tmpdir, sstate->curdir);

	for (curc = tmpdir + strlen(tmpdir);
		*curc != '/' && curc != tmpdir;
		--curc) {}	
	*curc = (curc != tmpdir) ? '\0' : *curc;

	if (!checkpath(tmpdir)) {
		if (sendcmd(sstate->cmdfd, "550 Path doesn't exist.") < 0)
			return (-1);
	
		return 0;
	}
	
	strcpy(sstate->curdir, tmpdir);

	if (sendcmd(sstate->cmdfd, "250 Directory successfully changed.") < 0)
		return (-1);

	return 0;
}

int strfilelist(const char *path, char **buf)
{
	DIR *d;
	struct dirent *de;
	size_t maxoutsz;
	size_t outsz;

	if ((d = opendir(path)) == NULL) {
		closedir(d);
		return (-1);
	}
	
	outsz = maxoutsz = 1;
	if ((*buf = realloc(NULL, maxoutsz)) == NULL) {
		closedir(d);
		return (-1);
	}

	(*buf)[0] = '\0';
	while ((de = readdir(d)) != NULL) {
		outsz += (strlen(de->d_name) + 1);
			
		if (outsz > maxoutsz) {
			maxoutsz *= 2;	
			if (((*buf) = realloc((*buf), maxoutsz)) == NULL) {
				closedir(d);
				return (-1);
			}
		}
		
		strcat(*buf, de->d_name);
		strcat(*buf, "\n");
	}
	
	closedir(d);

	return 0;
}

int lstofd(const char *dir, int fd)
{
	int pid1;
	int status;

	if ((pid1 = fork()) == 0) {
		close(1);
		dup(fd);

		if (setenv("LC_ALL", "C", 1) < 0)
			exit(1);

		execlp("ls", "ls", "-l", dir, NULL);
	
		exit(1);
	}

	if (waitpid(pid1, &status, 0) < 0)
		return (-1);

	return (status == 0) ? 0 : (-1);
}

int handlelist(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	struct sockaddr_in caddr;
	socklen_t addrlen;
	int datafd;

	addrlen = sizeof(caddr);	
	if (getpeername(sstate->cmdfd, (struct sockaddr *) (&caddr), &addrlen)
		< 0)
		goto accepterror;

	if ((datafd = connectionaccept(sstate->ldatafd, &(caddr.sin_addr)))
		< 0)
		goto accepterror;

	if (sendcmd(sstate->cmdfd, "150 Here comes the directory listing.")
		< 0)
		return (-1);
			
	if (lstofd(sstate->curdir, datafd) < 0)
		goto lserror;
	
	close(datafd);
	
	if (sendcmd(sstate->cmdfd, "226 Transfer complete.") < 0)
		return (-1);

	return 0;

accepterror:
	if (sendcmd(sstate->cmdfd, "425 Cannot open connection.") < 0)
		return (-1);

	return 0;

lserror:
	close(datafd);

	if (sendcmd(sstate->cmdfd, "426 Cannot send data.") < 0)
		return (-1);
	
	return 0;
}

int handlenoop(const char *arg[MAXARG + 1], struct serverstate *sstate)
{
	if (sendcmd(sstate->cmdfd, "200 NOOP ok.") < 0)
		return (-1);

	return 0;
}

int handlereq(enum command cmd, const char *arg[MAXARG + 1],
	struct serverstate *sstate)
{
	switch (cmd) {
	case CMD_TYPE:
		return handletype(arg, sstate);
	case CMD_MODE:
		return handlemode(arg, sstate);
	case CMD_STRU:
		return handlestru(arg, sstate);
	case CMD_USER:
		return handleuser(arg, sstate);
	case CMD_PASS:
		return handlepass(arg, sstate);
	case CMD_QUIT:
		return handlequit(arg, sstate);
	case CMD_PORT:
		return handleport(arg, sstate);
	case CMD_PASV:
		return handlepasv(arg, sstate);
	case CMD_RETR:
		return handleretr(arg, sstate);
	case CMD_STOR:
		return handlestor(arg, sstate);
	case CMD_PWD:
		return handlepwd(arg, sstate);
	case CMD_CWD:
		return handlecwd(arg, sstate);
	case CMD_CDUP:
		return handlecdup(arg, sstate);
	case CMD_LIST:
		return handlelist(arg, sstate);
	case CMD_NOOP:
		return handlenoop(arg, sstate);
	default:
		printf("Unknown command recieved.\n");
		if (sendcmd(sstate->cmdfd, "202 Command is not supported.")
			< 0)
			return (-1);
		
		return 0;
	}

	return 1;
}
