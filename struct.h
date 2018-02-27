#ifndef STRUCT_H
#define STRUCT_H

enum command {
	CMD_TYPE, CMD_MODE, CMD_STRU,
	CMD_USER, CMD_PASS, CMD_QUIT,
	CMD_PORT, CMD_PASV, CMD_RETR,
	CMD_STOR, CMD_PWD, CMD_CWD,
	CMD_CDUP, CMD_LIST, CMD_NOOP,
	CMD_UNKNOWN
};

enum datatype {
	DT_ASCII, DT_IMAGE
};

struct serverstate {
	char curdir[PATH_MAX];
	int dataport;
	int active;
	enum datatype dtype;
	int cmdfd;
	int ldatafd;
};

#endif
