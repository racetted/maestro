/* l2d2_socket.c - Network functions for server code of the Maestro sequencer software package.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <strings.h> 
#include <signal.h> 
#include <unistd.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/md5.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/dir.h>
#include <sys/param.h>
#include "l2d2_socket.h" 

extern char * str2md5(const char *, int );

static struct  sockaddr_in server;      /* server socket */
static socklen_t sizeserver = sizeof(server);
static int must_init_signal = 1;

/* GetHostName */
int GetHostName(char *name, size_t len) 
{
  int junk;
  junk = gethostname(name, len);
  return(junk);
}

/**
 * get Parameters token from file $HOME/.suites/.maestro_server_${version}
 * return pointer to character string upon success
 * return NULL pointer in case of error
 */
char *get_Authorization( char *filename , char *username , char **m5sum )  
{
     char *auth_buf=NULL;
     int fd;
     char buf[1024];
     struct passwd *ppass;

     if ( (auth_buf=(char*)malloc(1024)) == NULL ) {
	 fprintf(stderr, "get_Authorization: Cannot Malloc auth buffer\n");
	 exit(1); 
     } else {
         memset(auth_buf,'\0',1024); /* this is very important, if removed login can fail */
     }

     if (  (ppass=getpwnam(username)) == NULL ) {
	 fprintf(stderr, "get_Authorization: Cannot get user:%s passwd parameters\n",username);
	 free(auth_buf);
	 return (NULL); 
     }

     snprintf(buf, sizeof(buf), "%s/.suites/%s", ppass->pw_dir,filename);

     if ( access(buf, R_OK) != 0 ) {
	 fprintf(stderr, "get_Authorization:  Parameters file:%s does not exist \n",buf);
	 free(auth_buf);
	 return (NULL); 

     } 

     if ((fd = open(buf, O_RDONLY)) == -1) {
	 fprintf(stderr, "get_Authorization: Can't open Parameters file:%s\n",filename);
	 free(auth_buf);
	 return(NULL); 
     }

     if (read(fd, auth_buf, 1024) <= 0) {
	 fprintf(stderr, "get_Authorization: Can't read Parameters file\n");
	 close(fd);
	 free(auth_buf);
	 return(NULL); 
     }

     close(fd);
     
     /* compute md5sum here */
     *m5sum=str2md5(auth_buf,strlen(auth_buf));

     return(auth_buf);
}

/**
 * write Parameters tokens into file ~user/.suites/.maestro_server_${version} 
 */
void set_Authorization (unsigned int pid ,char * hostn , char * hip, int port , char * filename, char *username , char **m5sum) 
{
     int fd;
     char buf[1024];
     int nc,rt;
     struct passwd *ppass;
    
     if (  (ppass=getpwnam(username)) == NULL ) {
	 fprintf(stderr, "set_Authorization: Cannot get user:%s passwd parameters\n",username);
	 return; 
     }

     snprintf(buf, sizeof(buf), "%s/.suites/%s", ppass->pw_dir,filename);
     
     if ((fd = open(buf, O_WRONLY + O_CREAT, 0600)) == -1) {
	 fprintf(stderr,"set_Authorization: Can't open Parameters file:%s\n",filename);
	 free(ppass);
	 exit(1);
     }

     nc = snprintf(buf, sizeof(buf), "seqpid=%u seqhost=%s seqip=%s seqport=%d\n", pid, hostn, hip, port);
     rt = write(fd, buf, nc);
     rt = close(fd);
     
     /* compute md5sum here */
     *m5sum=str2md5(buf,strlen(buf));
}


/**
 * accept connections on the bound server socket return socket for incoming connection 
 * bind_sock_to_port must have been called before connection can be accepted           
 */
int accept_from_socket (int fserver) 
{
    struct sockaddr_in server;
    socklen_t sizeserver = sizeof(server) ;

    int fclient =  accept(fserver, (struct  sockaddr *)&server, &sizeserver);
    if (fclient < 0) {
          fprintf (stderr, "Accept failed!\n");
          return (-1);
    }
    return(fclient);
}

/**
 * bind an existing socket to a free port within the port range, return port number 
 * existing socket usually created by get_sock_net
 */
int bind_sock_to_port (int s, int min_port, int max_port) 
{
      struct sockaddr_in server_eff;
      socklen_t sizeserver_eff = sizeof(server_eff) ;
      int current_port=min_port, success=0;

      if ((min_port < 0) || ( min_port < 0)) {
         fprintf(stderr, "Invalid port range. Min port %d. Max port %d. Must be >= 0 \n",min_port, max_port);
         return (-1); 
      }
      if (min_port > max_port)  {
	      fprintf(stderr, "Error in port range. Min port %d > max port %d \n",min_port, max_port);
         return (-1); 
      }
      server.sin_family = AF_INET;
      server.sin_addr.s_addr = INADDR_ANY;
      while (current_port <= max_port) {
         server.sin_port = htons(current_port);
         if ((success=bind(s, (struct  sockaddr *)&server, sizeserver)) < 0) {
           ++current_port;
         } else {
            fprintf(stderr, "Bind successful on port %d \n", current_port);
            getsockname(s, (struct  sockaddr *)&server_eff, &sizeserver_eff);
            return ntohs(server_eff.sin_port);
         }
      }
	   fprintf(stderr, "Bind failed in port range %d -> %d \n", min_port,max_port);
      return(-1);
}

/**
 * create a network socket ; return socket descriptor 
 */
int get_socket_net()  
{
     /* ignore SIGPIPE signal (i.e. do no abort but return error) */

     if (must_init_signal)
       {  /* DO THIS ONLY ONCE */
	 
	 signal(SIGPIPE, SIG_IGN); 
	 must_init_signal = 0;

       }

     return socket(AF_INET, SOCK_STREAM, 0);
}

/**
 * set buffer sizes (recv and send) for a newly created socket (always returns 0) 
 */
int set_socket_opt(int s)  
{
  socklen_t optval, optsize;
  int b0 = 0;
  
  optval = SOCK_BUF_SIZE*1024;

  b0=setsockopt(s, SOL_SOCKET, SO_SNDBUF,(char *)&optval, sizeof(optval));

  if (b0 != 0)  fprintf(stderr, "Error setting SEND BUFFER: SO_SNDBUF size \n"); 

  optval = 0;
  optsize = 4;
  getsockopt(s, SOL_SOCKET, SO_SNDBUF, (char *)&optval, &optsize);

  optval = SOCK_BUF_SIZE*1024;
  b0 = setsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&optval, sizeof(optval));

  if (b0 != 0)  fprintf(stderr, "Error setting RECEIVE BUFFER: SO_RCVBUF size \n"); 

  optval = 0;
  optsize = 4;
  getsockopt(s, SOL_SOCKET, SO_RCVBUF, (char *)&optval, &optsize);

  return(0);
}

/**
 * obtain the IPV4 adress of a host specified by name 
 */
int get_ip_address(char *hostname) 
{
     int **addr_list;
     struct hostent *answer;
     int ipaddr = 0;
     int b0, b1, b2, b3;

     if (NULL == (answer = gethostbyname(hostname))) {
         fprintf(stderr, "Cannot get address for host=%s\n", hostname);
         return(-1);
     }

     addr_list = (int **)answer->h_addr_list;
     ipaddr = ntohl(**addr_list);

     b0 = ipaddr >> 24; b1 = ipaddr >> 16 ; b2 = ipaddr >> 8 ; b3 = ipaddr;
     b0 &= 255;
     b1 &= 255;
     b2 &= 255;
     b3 &= 255;

    return(ipaddr);

}

/**
 * obtain own host's IPV4 address 
 */
char *get_own_ip_address()  
{
     int ipaddr;
     int b0, b1, b2, b3;
     char buf[1024];

     char *ip=NULL;
     if ( (ip=(char *) malloc(1024)) == NULL ) {
	 fprintf(stderr, "Can't malloc in get_own_ip_address\n");
	 exit(1);

     }

     if(GetHostName(buf, sizeof buf)) {
	 fprintf(stderr, "Can't find hostname\n");
	 return (NULL);
     }

     ipaddr = get_ip_address(buf);
     b0 = ipaddr >> 24; /* split IP address */
     b1 = ipaddr >> 16 ;
     b2 = ipaddr >> 8 ;
     b3 = ipaddr;
     b0 &= 255;
     b1 &= 255;
     b2 &= 255;
     b3 &= 255;

     sprintf(ip, "%d.%d.%d.%d", b0, b1, b2, b3);

     return (ip); 

}

/**
 * given a [host:]port specification, connect to it
 * if host: is not specified, use localhost
 * the return value is the connected socket
 */
int connect_to_hostport(char *target2)  
{
     char buf[1024];
     int  fserver;
     int  b0, b1, b2, b3, sizeserver;
     int  ipaddr;
     char *portno;
     char *target;

     

     struct sockaddr_in server;
     sizeserver = sizeof(server);
     target = target2;

     if(NULL == strstr(target, ":"))
     {   /* no host specified, use local host */
	 portno = target;
         if (GetHostName(buf, sizeof buf)) {
	       fprintf(stderr, "Can't find hostname\n");
	       return(-1);
         } 
         ipaddr = get_ip_address(buf);
     } else {  /* use specified host, find address */
	 portno = strstr(target, ":"); 
	 *portno = '\0';
	 portno++;
	 ipaddr = get_ip_address(target);
     }


     b0 = ipaddr >> 24; b1 = ipaddr >> 16 ; b2 = ipaddr >> 8 ; b3 = ipaddr;
     b0 &= 255;
     b1 &= 255;
     b2 &= 255;
     b3 &= 255;
     snprintf(buf, sizeof(buf), "%d.%d.%d.%d", b0, b1, b2, b3);

     if ( (fserver=socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	    fprintf(stderr,"Can not acquire a socket stream ! \n");
	    return(-1);
     }

     set_socket_opt(fserver);

     server.sin_family = AF_INET;
     server.sin_port = htons(atoi(portno));
     server.sin_addr.s_addr = inet_addr(buf);

     if (connect(fserver, (struct  sockaddr *)&server, sizeserver) < 0) {
	    fprintf(stderr,"Connection to Server Failed, the server may be down! \n");
	    return(-1);
     }

     return(fserver);
}
/** 
 *  given a hostip and a port specification, connect to it
 *  The return value is the connected socket
 */
int connect_to_host_port_by_ip (char *hostip, int portno )  
{
     int fserver;
     struct sockaddr_in server;

     /* acquire a socket */
     if ( (fserver=socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
	    fprintf(stderr,"Can not acquire a socket stream ! \n");
	    return(-1);
     }
     
     /* set socket options */
     set_socket_opt(fserver);

     server.sin_family = AF_INET;
     server.sin_port = htons(portno);
     server.sin_addr.s_addr = inet_addr(hostip); 

     if (connect(fserver, (struct  sockaddr *)&server, sizeof(server)) < 0) {
	    fprintf(stderr,"Connection to Server Failed, the server may be down! \n");
	    return(-1);
     }

     return (fserver);
}

/**
 * send reply to command: 00 if success 11 if not. 
 *
 * The socket mode is non-blocking
 * We should normaly add a test on ret < 0 and  put close_conn=TRUE, but we test at the beginning
 * of the loop the client connection and we set up the variable close_conn=TRUE to remove this
 * client from the list.
 */
void send_reply(int fclient, int status )  
{
   int ret;
   if (status == 0 )
         ret=write(fclient, "00\0", 3);
   else
         ret=write(fclient, "11\0", 3);
}

/**
 * read from socket with time out 
 * Not used !
 */
int read_socket (int sock , char *buf , int size , unsigned int timeout) 
{
        int bytes_read=0;

	alarm(timeout);
	bytes_read=read(sock, buf, size );
	alarm(0);

        if ( bytes_read <=  0 ) return (-1);

	return (bytes_read);
}

/**
 * receive from socket with time out 
 * The socket mode here is blocking
 */
int recv_socket (int sock , char *buf , int size , unsigned int timeout) 
{
        int bytes_read=0;

	alarm(timeout);
	bytes_read=recv(sock, buf, size, 0);
	alarm(0);

        if ( bytes_read <=  0 ) return (-1);

	return (bytes_read);
}

/**
 * send to socket with time out 
 * The socket mode here is blocking
 */
int send_socket (int sock , char *buf , int size , unsigned int timeout) 
{
        int bytes_write=0;

	alarm(timeout);
	bytes_write=send(sock, buf, size, 0);
	alarm(0);

        if ( bytes_write <=0   ) return (-1);

	return (bytes_write);
}
/** 
 * Initiate a connection with maestro_server 
 */
int do_Login( int sock , unsigned int pid , char *node, char *xpname , char *signl , char *username ,char **m5) {

    char host[25];
    char bLogin[1024];
    char buffer[1024];
    char *path_status=NULL;
    char resolved[MAXPATHLEN];
    int  bytes_read, bytes_sent;
    struct stat fileStat;

    memset(bLogin,'\0',sizeof(bLogin));
    memset(buffer,'\0',sizeof(buffer));
    memset(resolved,'\0',MAXPATHLEN);

    gethostname(host, sizeof(host));

    /* need to have what the exp is pointing to : true_path */
    if ( (path_status=realpath(xpname,resolved)) == NULL ) {
               fprintf(stderr,"do_Login: Probleme avec le path=%s\n",xpname);
               return (1);
    } 

    /* get inode */
    if (stat(resolved,&fileStat) < 0) {    
               fprintf(stderr,"do_Login: Cannot stat on path=%s\n",resolved);
               return (1);
    }

   /* build authentication string check first length of strings, this can make sscanf on server do a memory fault !*/
    if ( strlen(xpname) >= 256 || strlen(node) >= 256 || strlen(signl) >= 256 || strlen(host) >= 256 || strlen(username) >= 256 ) {
               fprintf(stderr,"do_Login: length of string sent to server is above value 256\n");
               return (1);
    }

    snprintf(bLogin,sizeof(bLogin),"I %u %ld %s %s %s %s %s %s", pid, (long) fileStat.st_ino, xpname, node , signl , host, username, *m5);
    if ( (bytes_sent=send_socket (sock , bLogin , sizeof(bLogin) , SOCK_TIMEOUT_CLIENT)) <= 0 ) { 
                fprintf(stderr,"LOGIN FAILED (Timeout sending) with %s Maestro server from host=%s node=%s signal=%s\n",username, host, node, signl );
    	        return(1);
    } 
   
    
	    
    if ( (bytes_read=recv_socket (sock , buffer , sizeof(buffer) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
                fprintf(stderr,"LOGIN FAILED (Timeout receiving) with %s Maestro server from host=%s node=%s signal=%s\n",username, host, node, signl );
                return(1);
    }

    buffer[bytes_read > 0 ? bytes_read :0] = '\0';
		
    if ( buffer[0] != '0'  ) fprintf(stderr,"LOGIN FAILED with %s Maestro server from host=%s node=%s signal=%s\n",username, host, node, signl ); 
					  
    return (buffer[0] == '0' ? 0 : 1);

}
/**
 * recv_full 
 * receive all the stream based on a size with timeout 
 * 
 */
int recv_full ( int sock , char * buff, int rsize )
{
        int received = 0, r;

       	
        while ( received < rsize )
        {
           r = recv_socket( sock, buff + received, rsize - received, SOCK_TIMEOUT_CLIENT );
           if ( r <= 0 ) {
	           fprintf(stderr,"Client break in recv_full\n");
		   break;
           }
           received += r;
        }

	return (received==rsize)  ? 0 : 1; 
}

