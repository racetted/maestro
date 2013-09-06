#include "nodelogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <pwd.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>
#include "l2d2_socket.h"
#include <libgen.h>
#include <strings.h>

#define NODELOG_BUFSIZE 1024
#define NODELOG_FILE_LENGTH 500
#define TIMEOUT_NFS_SYNC 3


/* global variables */
static char nodelogger_buf[NODELOG_BUFSIZE];
static char nodelogger_buf_short[NODELOG_BUFSIZE];
extern int MLLServerConnectionFid;
extern int OpenConnectionToMLLServer (const char *, const char *);

static char NODELOG_JOB[NODELOG_BUFSIZE];
static char NODELOG_MESSAGE[NODELOG_BUFSIZE];
static char NODELOG_LOOPEXT[NODELOG_BUFSIZE];
static char NODELOG_DATE[NODELOG_BUFSIZE];

static char username[32];
static char LOG_PATH[1024];
static char TMP_LOG_PATH[1024];

static int write_line(int sock);
static void gen_message (const char *job, const char *type, const char* loop_ext, const char *message);
static int sync_nodelog_over_nfs(const char *job, const char *type, const char* loop_ext, const char *message, const char *dtstmp);
static int acquire_connection( char *seq_exp_home ,  char *datestamp ,  char *job );
extern char* str2md5 (const char *str, int length);

static void log_alarm_handler() { fprintf(stderr,"%%%%%%%%% EXCEEDED TIME IN LOOP ITERATIONS %%%%%%%%\n"); };
 
typedef enum {
           FROM_MAESTRO,
 	   FROM_NODELOGGER,
 	   FROM_MAESTRO_NO_SVR
} _FromWhere;
_FromWhere FromWhere;
 
void nodelogger(const char *job, const char* type, const char* loop_ext, const char *message, const char* datestamp)
{
   int i, sock=-1,ret;
   int send_success = 0;
   char *tmphost=NULL;
   char *tmpenv = NULL;
   char *tmpusername = NULL;
   char *tmpfrommaestro = NULL;
   char cmcnodelogger[25];
   char tmp[10];
   struct stat stbuf;
   char *seq_exp_home = NULL;
   char *basec = NULL;
   struct passwd *p, *p2;
   int gid = 0;
   char *experience;
   char line[1024];
   struct sigaction sa;

   if ( loop_ext == NULL ) { loop_ext=strdup(""); }
   if ( message  == NULL ) { message=strdup(""); }
    
   SeqUtil_TRACE( "nodelogger job:%s signal:%s message:%s loop_ext:%s datestamp:%s\n", job, type, message, loop_ext, datestamp);

   memset(LOG_PATH,'\0',sizeof LOG_PATH);
   memset(TMP_LOG_PATH,'\0',sizeof TMP_LOG_PATH);
   memset(username,'\0',sizeof username);

   if ( ( p = getpwuid ( getuid() ) ) == NULL ) {
      fprintf(stderr,"nodelogger: getpwuid() error" );
      return;
   } else {
      gid = (int) p->pw_gid;
      /* example: pw_name = afsipat */
      strcpy(username, p->pw_name);
   }

   p2 = getpwnam(username);
   if( (seq_exp_home=getenv("SEQ_EXP_HOME")) == NULL ) {
      fprintf( stderr, "Nodelogger::ERROR: You must provide a valid SEQ_EXP_HOME.\n" );
      exit(1);
   }

   snprintf(LOG_PATH,sizeof(LOG_PATH),"%s/logs/%s_nodelog",seq_exp_home,datestamp);
   snprintf(TMP_LOG_PATH,sizeof(TMP_LOG_PATH),"%s/sequencing/sync/%s",seq_exp_home,datestamp);


   /* make a duplicate of seq_exp_home because basename may return pointers */
   /* to statically allocated memory which may be overwritten by subsequent calls.*/
   basec = (char *) strdup(seq_exp_home);
   experience = (char *) basename(basec);

    /* setup an alarm so that if the logging is stuck
     * it will timeout after 60 seconds. This will prevent the
     * processes from hanging when there are network problems.
     */

    memset(NODELOG_JOB,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_JOB,job);
    memset(NODELOG_LOOPEXT,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_LOOPEXT,loop_ext);
    memset(NODELOG_MESSAGE,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_MESSAGE,message);
    memset(NODELOG_DATE,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_DATE,datestamp);

    /* if called inside maestro, a connection is already open  */
    tmpfrommaestro = getenv("FROM_MAESTRO");
    if ( tmpfrommaestro == NULL ) {
       fprintf(stderr, "\n================= NODELOGGER: NOT_FROM_MAESTRO signal:%s================== \n",type);
       FromWhere = FROM_NODELOGGER;
       if ( (sock=OpenConnectionToMLLServer( seq_exp_home , "LOG" )) < 0 ) { 
          gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
          ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp); 
          return;
       }
       /* install SIGALRM handler */
       /* set the signal handler */
       sa.sa_handler = log_alarm_handler;
       sa.sa_flags = 0; /* was SA_RESTART  */
       sigemptyset(&sa.sa_mask);
       /* register signals */
       if ( sigaction(SIGALRM,&sa,NULL) == -1 ) fprintf(stderr,"Nodelooger::error in registring SIGALRM\n");
    } else {
       fprintf(stderr, "\n================= NODELOGGER: TRYING TO USE CONNECTION FROM MAESTRO PROCESS IF SERVER IS UP signal:%s================== \n",type);
       if ( MLLServerConnectionFid > 0 ) {
          FromWhere = FROM_MAESTRO;
          sock = MLLServerConnectionFid;
          fprintf(stderr, "================= NODELOGGER: OK,HAVE A CONNECTION FROM MAESTRO PROCESS ================== \n");
       } else {
        /* it could be that we dont have the env. variable set to use Server */
           FromWhere = FROM_MAESTRO_NO_SVR;
           if ( (sock=OpenConnectionToMLLServer( seq_exp_home , "LOG" )) < 0 ) { 
               fprintf(stderr, "================= NODELOGGER: CANNOT ACQUIRE CONNECTION FROM MAESTRO PROCESS signal:%s================== \n",type);
               gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
               ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp); 
               return;
            } else {
               fprintf(stderr, "================= ACQUIRED A NEW CONNECTION FROM NODELOGGER PROCESS ================== \n");
            }
       }
    }
 
    /* if we are here socket is Up then why the second test > -1 ??? */ 
    gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
    if ( sock > -1 ) {
        if ( write_line(sock) == -1 ) { 
            ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp); 
            return;
        }
    } else {
        ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp); 
        return;
    }
        /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@ CRITICAL  : CLOSE SOCKET ONLY WHEN NOT ACQUIRED FROM MAESTRO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
    switch (FromWhere)
    {

       case FROM_NODELOGGER:
       case FROM_MAESTRO_NO_SVR:
	   ret=write(sock,"S \0",3);
           close(sock);
           fprintf(stderr, "================= ClOSING CONNECTION FROM NODELOGGER PROCESS ================== \n");
	   break;
       default:
           break;
    }
}


/**
 * write a buffer to socket
 *
 *
 */
static int write_line(int sock)
{

   int bytes_read, bytes_sent;
   char bf[512];

   memset(bf, '\0', sizeof(bf));

   if ( (bytes_sent=send_socket(sock , nodelogger_buf , sizeof(nodelogger_buf) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
      fprintf(stderr,"%%%%%%%%%%%% NODELOGGER: socket closed at send  %%%%%%%%%%%%%%\n");
      return(-1);
   }

   if ( (bytes_read=recv_socket (sock , bf , sizeof(bf) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
      fprintf(stderr,"%%%%%%%%%%%% NODELOGGER: socket closed at recv   %%%%%%%%%%%%%%\n");
      return(-1);
   } 

   bf[bytes_read > 0 ? bytes_read : 0] = '\0';
   if ( bf[0] != '0' ) {
      fprintf(stderr,"Nodelogger::write_line: errno=%d bf=%s nodelogger_buf=%s\n",errno,bf,nodelogger_buf);
      return(-1);
   }
   return(0);

}

/* gen_message: write a formatted log message */

static void gen_message (const char *node,const char *type,const char* loop_ext, const char* message)
{

    time_t time();
    time_t timval;

    int i;
    int joblength;
    char nodepath[100];
    int c_month, c_day, c_hour, c_min, c_sec, c_year;


    struct tm *localtime(), *tmptr;

    (void) time(&timval);
    tmptr = localtime(&timval);

    /* obtain the current date */

    c_year  = tmptr->tm_year+1900;
    c_month = tmptr->tm_mon+1;
    c_day   = tmptr->tm_mday;
    c_hour  = tmptr->tm_hour;
    c_min   = tmptr->tm_min;
    c_sec   = tmptr->tm_sec;

    joblength=strlen(node);
    /* if ( joblength > 14 ) joblength = 14; */

    memset(nodepath, '\0', sizeof nodepath);

    /* write the message into "nodelogger_buf", which will be sent to a socket if server up */
    memset(nodelogger_buf, '\0', NODELOG_BUFSIZE );
    memset(nodelogger_buf_short, '\0', NODELOG_BUFSIZE );
    fprintf (stdout,"NODELOGGER node:%s type:%s message:%s\n", node, type, message );

    if ( loop_ext != NULL ) {
        snprintf(nodelogger_buf,sizeof(nodelogger_buf),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQLOOP=%s:SEQMSG=%s\n",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,loop_ext,message);
        snprintf(nodelogger_buf_short,sizeof(nodelogger_buf_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQLOOP=%s:SEQMSG=%s\n",c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,loop_ext,message);
    } else {
        snprintf(nodelogger_buf,sizeof(nodelogger_buf),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQMSG=%s\n",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,message);
        snprintf(nodelogger_buf_short,sizeof(nodelogger_buf_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQMSG=%s\n",c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,message);
    }
}



/**
  *  Synchronize clients logging to Experiment nodelog
  *  author : Rochdi Lahlou
  *  Algo   :
  *
  *
  *
  *
  *
  */
static int sync_nodelog_over_nfs (const char *node, const char * type, const char * loop_ext, const char * message, const char * datestamp)
{
    FILE *fd;
    struct passwd *ppass;
    struct stat fileStat;
    struct sigaction sa;
    int fileid,num,nb,my_turn,ret,len,status=0;
    int success=0, loop=0;
    unsigned int pid;
    struct timeval tv;
    struct tm* ptm;
    struct stat st;
    static DIR *dp = NULL;
    struct dirent *d;
    time_t now;
    char *seq_exp_home=NULL, *truehost=NULL, *path_status=NULL, *mversion=NULL;
    char resolved[MAXPATHLEN];
    char host[100], lock[1024],flock[1024],lpath[1024],buf[1024];
    char time_string[40],Stime[40],Etime[40],Atime[40];
    char ffilename[1024],filename[256];
    char host_pid[50],TokenHostPid[250],underline[2];
    double Tokendate,date,modiftime,actualtime;
    double diff_t;

    /* get user parameters */
    if (  (ppass=getpwnam(username)) == NULL ) {
        fprintf(stderr, " Nodelogger::Cannot get user:%s passwd parameteres\n",username);
        return (1); 
    }
    /* get host , pid */
    gethostname(host, sizeof(host));
    pid = getpid();
     
    /* env TRUE HOST should be defined */
    if ( (truehost=getenv("TRUE_HOST")) == NULL ) {
       fprintf(stderr, "Nodelogger::Cannot get env. var TRUE_HOST\n");
       truehost=host;
    }
 
    /* env MAESTRO_VERSION */
    if ( (mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
        fprintf(stderr, "Nodelogger::Cannot get env. var SEQ_MAESTRO_VERSION\n");
    }
 
    /* env SEQ_EXP_HOME */
    if ( (seq_exp_home=getenv("SEQ_EXP_HOME")) == NULL ) {
        fprintf(stderr,"Nodelogger::cannot get SEQ_EXP_HOME variable ...\n");
    }
 
    /* construct a unique bucket name  */
    memset(lock,'\0', sizeof(lock));
    memset(flock,'\0', sizeof(flock));
    memset(lpath,'\0', sizeof(lpath));
 
    /* I need an xp Inode */
    if ( (path_status=realpath(seq_exp_home,resolved)) == NULL ) {
       fprintf(stderr,"Nodelogger::Probleme avec seq_exp_home=%s\n",seq_exp_home);
       return (-1);
    } 
 						     
    /* get inode */
    if (stat(resolved,&fileStat) < 0) {
       fprintf(stderr,"Nodelogger::Cannot stat on path=%s\n",resolved);
       return (-1);
    }
     
    /* create sync directory if not there */
    snprintf(lpath,sizeof(lpath),"%s/v%s",TMP_LOG_PATH,mversion);
    if ( access(lpath,R_OK) != 0 ) status=SeqUtil_mkdir_nfs(lpath , 1);
     
    snprintf(lock,sizeof(lock),"%s/v%s/node_logger_lock",TMP_LOG_PATH,mversion);
 
    get_time(Stime,3);
    Tokendate = atof(Stime);
    snprintf (flock,sizeof(flock),"%s/%s_%s_%u",lpath,Stime,host,pid);
     
    fprintf(stdout,"my Token date=%s host_pid=%s %u\n",Stime,host,pid);
    fprintf(stdout," lpath=%s\n",lpath);
    fprintf(stdout," lock=%s\n",lock);
    fprintf(stdout," flock=%s\n",flock);

    sprintf (TokenHostPid,"%s_%u",host,pid);
    
    if ( (fd=fopen(flock,"w+")) == NULL ) {
       fprintf(stderr,"Nodelogger::could not open filename:%s\n",flock);
       /* decide what to do here */ 
       return(-1);
    } else {
       fclose(fd);
    }

    time(&now);
    alarm(TIMEOUT_NFS_SYNC);
    get_time(Stime,3);
    while ( loop < 1000 ) {
       if ( (dp=opendir(lpath)) == NULL ) {
          fprintf(stderr,"Nodelogger::Cannot open dir:%s\n",lpath);
          continue; 
       }
       while ( d=readdir(dp)) {
          snprintf(ffilename,sizeof(ffilename),"%s/%s",lpath,d->d_name);
          snprintf(filename,sizeof(filename),"%s",d->d_name);
 
          if ( stat(ffilename,&st) != 0 ) { continue; }
	  if ( ( st.st_mode & S_IFMT ) == S_IFREG ) {
 	     /* skip hidden files */
 	     if ( filename[0] == '.' ) continue;
             nb = sscanf(filename,"%lf%1[_]%s",&date,underline,host_pid);
             if ( nb == 3 ) {
                if (  strcmp(TokenHostPid,host_pid) == 0 ) {
                   my_turn = 1;
                   continue;
                }
                if ( Tokendate > date ) {
                   /* modif time should  be less than 2 sec */
                   if ( (stat(ffilename,&st)) < 0 ) {
 	              continue;
 	           }
 		   if ( (diff_t=difftime(now,st.st_mtime)) > 2 ) {
 		      ret=unlink(ffilename); /* token file should be removed */
 		   }
 		   my_turn = 0;
 		   break;
 		} else {
 		   my_turn = 1;
 		}
            }
         }
      } /* first while */
      
      if ( my_turn == 1 ) {
          if ( (status=link(flock,lock)) == -1 ) {
          /*  have to examine the life of the lock to see if it's belong to a living process */
             if ( (lstat(lock,&st)) < 0 ) {
 	        continue;
             } else if ( (diff_t=difftime(now,st.st_mtime)) > 2 ) {
 			ret=unlink(lock); /* lock file removed */
 	     }
          } else {
 	     get_time(Atime,3);
             if ( (status=stat(flock,&st)) == 0 ) {
                if ( st.st_nlink == 2 ) {
                   if ( (fileid = open(LOG_PATH,O_WRONLY|O_CREAT,0755)) < 1 ) {
                      fprintf(stderr,"Nodelogger::could not open filename:%s\n",LOG_PATH);
                   } else {
 	              off_t s_seek=lseek(fileid, 0, SEEK_END);
                      num = write(fileid, nodelogger_buf_short, strlen(nodelogger_buf_short));
 	              fsync(fileid);
 	              close(fileid);
 	              ret=unlink(lock);
 	              ret=unlink(flock);
                      closedir(dp);
 	              success=1;
 		      break;
                   }
                } else {
                   fprintf(stderr,"Nodelogger::Number of link not equal to 2\n");
                }
             } 
          }
      } else {
         /* update own modif time to signal other that still active */
         ret=utime(flock,NULL);
      }
 
      loop++;
      closedir(dp);
    }
    get_time(Etime,3);
    if ( success == 1 ) { 
       fprintf(stderr,"Nodelogger NFS_SYNC::Started at:%s Grabbed at:%s Ended at:%s tries=%d\n",Stime,Atime,Etime,loop);
    } else  {
       fprintf(stderr,"Nodelogger NFS_SYNC DID NOT GET the lock Started at:%s Ended at:%s tries=%d\n",Stime,Atime,loop);
       /* erase the lock  will lower load on the remaining clients */
       ret=unlink(flock);
    }
    alarm(0);
    return(0);
}




/**
 *  acquire a connection with l2d2 server, the process is
 *  1- reading the authority file for server parametres.
 *  2- doing a login to be authenticated by the l2d2 server.
*/
int acquire_connection (char *seq_exp_home ,  char *datestamp ,  char *job )
{
   static char ipserver[20];
   static char htserver[20];
   static char host[100];
   static char buf_id[250];
   static char buffer[250];
 
   char authorization_file[256];
   char *Auth_token=NULL;
   char *m5sum=NULL;
   char resolved[MAXPATHLEN];
   char *path_status=NULL, *mversion=NULL;
   struct stat stbuf;
   unsigned int pid;
   int port, sock , nscan, ret;
   int bsent , bread ;
   struct passwd *passwdEnt = getpwuid(getuid());

   gethostname(host, sizeof(host));

   if ( ( mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
            fprintf(stderr,"Nodelogger::Could not get maestro version ...\n");
   }
    
   snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",mversion);

   if ( (Auth_token=get_Authorization (authorization_file,passwdEnt->pw_name,&m5sum)) != NULL) {
      nscan = sscanf(Auth_token, "seqpid=%u seqhost=%s seqip=%s seqport=%u", &pid, htserver, ipserver, &port);
      fprintf(stderr, "Maestro Server parameters are: pid=>%u<  host=>%s< ip=>%s< port=>%u<\n",pid,htserver,ipserver,port);
      /* try to get a connection  */ 
      if ( (sock=connect_to_host_port_by_ip (ipserver,port))  < 0 ) {
         fprintf(stderr,"Nodelogger Cannot connect to maestro_server at host:%s and port:%d \n", htserver, port);
         return(-1);
      } else {
         if ( (ret=do_Login(sock, pid, job, seq_exp_home, "LOG", passwdEnt->pw_name, &m5sum)) != 0 ) return(-1);
      }
   } else {
      fprintf(stderr, "Nodelogger::Found No maestro_server parametres file\n");
      return(-1);
   }

   return(sock);
}
