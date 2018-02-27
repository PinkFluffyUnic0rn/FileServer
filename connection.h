#include <netinet/in.h>

int listenaddr(const struct in_addr *saddr, const in_port_t *port);

int connectionaccept(int listenfd, const struct in_addr *caddr);

int filelist(const char *path, char **buf);

int sendcmd(int cmdfd, char *str);

int recvcmd(int cmdfd, char *str, int maxsize);

int tryrecvcmd(int cmdfd, char *str, int maxsize);

int ftpsendfile(int fd, int sfd, off_t count);

int ftpreadfile(int fd, int sfd, off_t count);
