/* Part of the Maestro sequencer software package.
 * Copyright (C) 2011-2015  Operations division of the Canadian Meteorological Centre
 *                          Environment Canada
 *
 * Maestro is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation,
 * version 2.1 of the License.
 *
 * Maestro is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


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

/* prototype */
int GetHostName (char *, size_t );
char *get_Authorization( char * , char *, char **);
void  set_Authorization (unsigned int  ,char * , char * , int  , char * , char *,char **);
int accept_from_socket (int fserver);
int bind_sock_to_port (int s, int min_port, int max_port);
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
