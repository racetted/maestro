#ifndef L2D2_SOCKET_H
#define L2D2_SOCKET_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <strings.h> 
#include <signal.h> 
#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/param.h>

/* size of socket buffers in KiloBytes */
#define SOCK_BUF_SIZE 10
#define SOCK_TIMEOUT_CLIENT 20

static struct  sockaddr_in server;      /* server socket */
static socklen_t sizeserver = sizeof(server);
static int must_init_signal = 1;

/* prototype */
int GetHostName (char *, size_t );
char *get_Authorization( char * , char *, char **);
void  set_Authorization (unsigned int  ,char * , char * , int  , char * , char *,char **);
int accept_from_socket (int fserver);
int bind_sock_to_port (int s);
int get_socket_net();
int set_socket_opt(int s);
int get_ip_address(char *hostname);
char *get_own_ip_address();
int connect_to_hostport(char *target2);
int connect_to_host_port_by_ip (char *hostip, int portno );
int send_socket (int , char * , int  , unsigned int );
int read_socket (int , char * , int  , unsigned int ); 
int recv_socket (int , char * , int  , unsigned int ); 
int recv_full ( int sock , char * buff, int rsize );
void send_reply (int , int );  
int do_Login( int  , unsigned int  , char *, char * , char * , char * ,char **);
#endif
