#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <arpa/inet.h>

#include "struct.h"
#include "globals.h"
#include "connection.h"
#include "handlerequest.h"

void sigchild(int signo)
{
	int stat;

	while(waitpid(-1, &stat, WNOHANG) > 0) {}
}

int initserver()
{
	int servfd;
	struct sockaddr_in servaddr;

	memset(&servaddr, 0, sizeof(servaddr));
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(CMDPORT);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if ((servfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket: ");
		return (-1);
	}
	
	if (bind(servfd, (struct sockaddr *) (&servaddr),
		sizeof(servaddr)) < 0) {
		perror("Cannot bind socket: ");
		close(servfd);
		return (-1);
	}

	if (listen(servfd, MAXCONN) < 0) {
		perror("Cannot listen socket: ");
		close(servfd);
		return (-1);
	}

	return servfd;
}

int trimstring(char *s)
{

	return 0;
}

enum command parsereq(char *req, char *arg[MAXARG + 1])
{
	enum command cmd;

	while (1) {
		char *curc;
		
		for (curc = req; *curc != ' ' && *curc != '\0'; ++curc) {}
		
		if (*curc == '\0')
			break;

		*curc = '\0';
		
		*arg++ = ++curc;
	}

	*arg = NULL;

	if (strcmp(req, "TYPE") == 0)
		cmd = CMD_TYPE;
	else if (strcmp(req, "MODE") == 0)
		cmd = CMD_MODE;
	else if (strcmp(req, "STRU") == 0)
		cmd = CMD_STRU;
	else if (strcmp(req, "USER") == 0)
		cmd = CMD_USER;
	else if (strcmp(req, "PASS") == 0)
		cmd = CMD_PASS;
	else if (strcmp(req, "QUIT") == 0)
		cmd = CMD_QUIT;
	else if (strcmp(req, "PORT") == 0)
		cmd = CMD_PORT;
	else if (strcmp(req, "PASV") == 0)
		cmd = CMD_PASV;
	else if (strcmp(req, "RETR") == 0)
		cmd = CMD_RETR;
	else if (strcmp(req, "STOR") == 0)
		cmd = CMD_STOR;
	else if (strcmp(req, "PWD") == 0)
		cmd = CMD_PWD;
	else if (strcmp(req, "CWD") == 0)
		cmd = CMD_CWD;
	else if (strcmp(req, "CDUP") == 0)
		cmd = CMD_CDUP;
	else if (strcmp(req, "LIST") == 0)
		cmd = CMD_LIST;
	else if (strcmp(req, "NOOP") == 0)
		cmd = CMD_NOOP;
	else
		cmd = CMD_UNKNOWN;

	return cmd;
}

int getsockaddrstr(int fd, char *saddr)
{
	struct sockaddr_in addr;
	socklen_t addrlen;

	addrlen = sizeof(addr);
	
	if (getsockname(fd, (struct sockaddr *) (&addr), &addrlen) < 0)
		return (-1);

	if (inet_ntop(AF_INET, &(addr.sin_addr), saddr, addrlen) == NULL)
		return (-1);

	return 0;
}

int getpeeraddrstr(int fd, char *saddr)
{
	struct sockaddr_in addr;
	socklen_t addrlen;

	addrlen = sizeof(addr);

	if (getpeername(fd, (struct sockaddr *) (&addr), &addrlen) < 0)
		return (-1);

	if (inet_ntop(AF_INET, &(addr.sin_addr), saddr, addrlen) == NULL)
		return (-1);

	return 0;
}

void serveclient(int cmdfd)
{
	struct serverstate sstate;
	int state;

	strcpy(sstate.curdir, HOMEDIR);
	sstate.dataport = DEFDATAPORT;
	sstate.active = 0;
	sstate.dtype = DT_ASCII;
	sstate.cmdfd = cmdfd;
	sstate.ldatafd = -1;

	if (sendcmd(cmdfd, "220 Server ready.") < 0) {
		fprintf(stderr, "Cannot send data.\n");
		state = 1;
	}
	
	state = 0;
	while (!state) {
		char req[MAXREQLEN];
		enum command cmd;
		char *args[MAXARG + 1];

		if (recvcmd(cmdfd, req, MAXREQLEN) < 0) {
			fprintf(stderr, "Connection lost or time out.\n");
			state = 1;
			continue;
		}

		cmd = parsereq(req, args);
	
		state = handlereq(cmd, (const char **) args, &sstate);
	}
}

int main()
{
	int servfd;
	
	if ((servfd = initserver()) < 0)
		return 1;

	if (signal(SIGCHLD, sigchild) == SIG_ERR)
		return 1;

	while (1) {
		int cmdfd;
		int pid;
		int flags;
			
		if ((cmdfd = accept(servfd, NULL, NULL)) < 0) {
			perror("Cannot accept connection: ");
			continue;
		}	
		
		flags = fcntl(cmdfd, F_GETFL);
		fcntl(cmdfd, F_SETFL, flags | O_NONBLOCK);

		printf("client connected.\n");

		if ((pid = fork()) == 0) {
			close(servfd);

			serveclient(cmdfd);
			
			printf("client disconnected.\n");
			
			close(cmdfd);
			return 0;
		}
		else if (pid < 0) {
			perror("Cannot fork process: ");
			
			close(cmdfd);
			continue;
		}
	}
	
	return 0;
}
