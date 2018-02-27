#ifndef HANDLEREQUEST_H
#define HANDLEREQUEST_H

#include "struct.h"

int handlereq(enum command cmd, const char *arg[MAXARG + 1],
	struct serverstate *sstate);

#endif
