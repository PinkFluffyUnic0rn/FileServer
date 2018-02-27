#include <linux/limits.h>

#define MIN(a, b) (a < b ? a : b)
#define MAX(a, b) (a > b ? a : b)

#define MAXCONN 10
#define MAXREQLEN 1024
#define MAXARG 2
#define BUFSIZE 4096
#define TIMEOUT 120

#define HOMEDIR "/"

#define SAVEPERMISSIONS (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)

#define CMDPORT 21
#define DEFDATAPORT 20
