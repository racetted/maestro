#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <strings.h> 
#include <signal.h> 
#include <unistd.h>
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

/* GetHostName special gethostname with IBM p690 name substitution */
int GetHostName(char *name, size_t len) 
{
  int junk;

  junk = gethostname(name, len);
  if ( name[0] == 'c' && name[2] == 'f' && name[5] == 'p' && name[7] == 'm' && name[8] == '\0' ) name[7] = 's';  /* name=cxfyypzm,return cxfyypzs instead */

  return(junk);
}

/**
 * get Parameteres token from file $HOME/.suites/.maestro_server_${version}
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
     }

     if (  (ppass=getpwnam(username)) == NULL ) {
	 fprintf(stderr, "get_Authorization: Cannot get user:%s passwd parameteres\n",username);
	 free(auth_buf);
	 return (NULL); 
     }

     snprintf(buf, sizeof(buf), "%s/.suites/%s", ppass->pw_dir,filename);

     if ( access(buf, R_OK) != 0 ) {
	 fprintf(stderr, "get_Authorization:  Parameteres file:%s do not exist \n",buf);
	 free(auth_buf);
	 return (NULL); 

     }

     if ((fd = open(buf, O_RDONLY)) == -1) {
	 fprintf(stderr, "get_Authorization: Can't open Parameteres file:%s\n",filename);
	 free(auth_buf);
	 return(NULL); 
     }

     if (read(fd, auth_buf, 1024) <= 0) {
	 fprintf(stderr, "get_Authorization: Can't read Parameteres file\n");
	 close(fd);
	 free(auth_buf);
	 return(NULL); 
     }

     close(fd);
     
     if (index(auth_buf, '\n')) {
	 *index(auth_buf, '\n') = '\0' ;
     } else {
	 fprintf(stderr, "get_Authorization: Invalid Parameteres file:%s\n",filename);
	 return (NULL); 
     }

     /* compute md5sum here */
     *m5sum=str2md5(auth_buf,strlen(auth_buf));

     return(auth_buf);
}

/**
 * write Parameteres tokens into file ~user/.suites/.maestro_server_${version} 
 */
void set_Authorization (int pid ,char * hostn , char * hip, int port , char * filename, char *username , char **m5sum) 
{
     int fd;
     char buf[1024];
     int nc;
     int ret=0;
     struct passwd *ppass;
    
     if (  (ppass=getpwnam(username)) == NULL ) {
	 fprintf(stderr, "set_Authorization: Cannot get user:%s passwd parameteres\n",username);
	 return; 
     }

     snprintf(buf, sizeof(buf), "%s/.suites/%s", ppass->pw_dir,filename);
     
     if ((fd = open(buf, O_WRONLY + O_CREAT, 0600)) == -1) {
	 fprintf(stderr,"set_Authorization: Can't open Parameteres file:%s\n",filename);
	 exit(1);
     }

     nc = snprintf(buf, sizeof(buf), "seqpid=%d seqhost=%s seqip=%s seqport=%d\n", pid, hostn, hip, port);
     ret=write(fd, buf, nc + 1);
     close(fd);
     
     /* compute md5sum here */
     *m5sum=str2md5(buf,strlen(buf));
}


/**
 * accept connections on the bound server socket return socket for incoming connection 
 * bind_sock_to_port must have been called before connection can be accpeted           
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
 * bind an existing socket to a free (automatic) port, return port number 
 * existing socket usually created by get_sock_net
 */
int bind_sock_to_port (int s) 
{
     struct sockaddr_in server_eff;
     socklen_t sizeserver_eff = sizeof(server_eff) ;

     server.sin_family = AF_INET;
     server.sin_port = htons(0);
     server.sin_addr.s_addr = INADDR_ANY;

     if (bind(s, (struct  sockaddr *)&server, sizeserver) < 0) {
	    fprintf(stderr, "Bind failed! \n");
	    return(-1);
     }
     getsockname(s, (struct  sockaddr *)&server_eff, &sizeserver_eff);
     return ntohs(server_eff.sin_port);
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
 * connect to it. The return value is the connected socket
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
 * send reply to command: 00 if success 11 if not  
 */
void send_reply(int fclient, int status)  
{
   int ret;
   
   if (status == 0 )
         ret=write(fclient, "00\0", 3);
   else
         ret=write(fclient, "11\0", 3);


   if (ret <= 0 ) fprintf(stderr,"Could not send reply \n");
}
/**
 * read from socket with time out 
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
 */
int recv_socket (int sock , char *buf , int size , unsigned int timeout) 
{
        int bytes_write=0;

	alarm(timeout);
	bytes_write=recv(sock, buf, size, 0);
	alarm(0);

        if ( bytes_write <=  0 ) return (-1);

	return (bytes_write);
}

/**
 * send to socket with time out 
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
int do_Login( int sock , int pid , char *node, char *xpname , char *signl , char *username ,char **m5) {

    char host[25];
    char bLogin[1024];
    char buffer[1024];
    char *path_status=NULL;
    char resolved[MAXPATHLEN];
    int bytes_read, bytes_sent;
    struct stat fileStat;

    memset(bLogin,'\0',sizeof(bLogin));
    memset(buffer,'\0',sizeof(buffer));
    memset(resolved,'\0',MAXPATHLEN);

    gethostname(host, sizeof(host));

    /* need to have what the exp is pointing to : true_path */
    if ( (path_status=realpath(xpname,resolved)) == NULL ) {
               fprintf(stderr,"do_Login:Probleme avec le path=%s\n",xpname);
               return (1);
    } 

    /* get inode */
    if (stat(resolved,&fileStat) < 0) {    
               fprintf(stderr,"do_Login:Cannot stat on path=%s\n",resolved);
               return (1);
    }

   /* build authentication string check first length of strings, this can make sscanf on server do a memory fault !*/
    if ( strlen(xpname) >= 256 || strlen(node) >= 256 || strlen(signl) >= 256 || strlen(host) >= 256 || strlen(username) >= 256 ) {
               fprintf(stderr,"do_Login: length of string sent to server is above value 256\n");
               return (1);
    }

    snprintf(bLogin,sizeof(bLogin),"I %d %u %s %s %s %s %s %s",(unsigned int) pid,(unsigned int)fileStat.st_ino, xpname, node , signl , host, username,*m5);
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
 *
 * rec_full 
 * receive all the stream base on a size given at
 * begining of the buffer.
 * Need a timeout
 */
int recv_full ( int sock , char * buff, int rsize )
{
        int received = 0;
        while ( received < rsize )
        {
           int r = recv( sock, buff + received, rsize - received, 0);
           if ( r <= 0 ) {
                fprintf(stderr,"Client break\n");
                break;
           }

           received += r;
        }

	return (received==rsize)  ? 0 : 1; 
}
/**
 *  GetFile
 *  routine to download the waited_end file to 
 *  TMPDIR of client host, open it and give the handle
 *  to work with.
 */
FILE * GetFile ( const char * filename , int sock ) 
{
  FILE *fp;
  char buffer[8192];  /* 8K waited_end file !!! */
  char wfilename[1024];
  char tmp[1024];
  char csize[11];
  int bytes_sent,bytes_read,error=0;

  if ( getenv("TMPDIR") != NULL ) {
    snprintf(wfilename,sizeof(wfilename),"%s/waitfile",getenv("TMPDIR"));
  } else {
    snprintf(wfilename,sizeof(wfilename),"/tmp/waitfile");
  }

  /* build command */
  sprintf(tmp,"Z %s",filename);

  if ( (bytes_sent=send_socket(sock , tmp , sizeof(tmp) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
    fprintf(stderr,"%%%%%%%%%%%% GetFile: socket closed at send  %%%%%%%%%%%%%%\n");
    fprintf(stderr, "====== GetFile : Reverting to nfs Routines ====== \n");
    /* ret_nfs=revert_nfs ( buf , action ); return(ret_nfs); */
  }

  memset(buffer,'\0',sizeof(buffer));

  /* the size of the buffer is given as 11 digit number BUT we are
   * only able to handle 8K max !!!! , note :11 to grab space after number */

  if ( recv_full(sock,csize,11) == 0 && recv_full(sock,buffer,atoi(csize)) == 0 ) { 
    if (  (fp=fopen(wfilename,"w+")) == NULL) {
              fprintf(stderr,"Could not open local waited_end file:%s\n",wfilename);
              return(NULL);
    }
    
    fwrite(buffer, sizeof(char) , sizeof(buffer) , fp);
    fclose(fp); fp=NULL;
  } else {
    fprintf(stdout, "ERROR can not downlaod waited file:%s\n",filename);
    return (NULL);
  }

  /* now the waited_end file is local (TMPDIR) , open it a give the
   * handle to routine ... */
  if ( (fp=fopen(wfilename,"r")) == NULL) { 
         fprintf(stderr,"Crap .... Could not open local waited_end file:%s\n",wfilename);
         return(NULL);
  }

  return(fp);
}

