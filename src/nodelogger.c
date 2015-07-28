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
#include "nodelogger.h"
#include "l2d2_socket.h"
#include "SeqUtil.h"
#include <libgen.h>
#include <strings.h>

#define NODELOG_BUFSIZE 1024
#define NODELOG_FILE_LENGTH 512
#define TIMEOUT_NFS_SYNC 3


/* global variables */
static char nodelogger_buf[NODELOG_BUFSIZE];
static char nodelogger_buf_top[NODELOG_BUFSIZE];
static char nodelogger_buf_short[NODELOG_BUFSIZE];
static char nodelogger_buf_notify[NODELOG_BUFSIZE];
static char nodelogger_buf_notify_short[NODELOG_BUFSIZE];
extern int MLLServerConnectionFid;
extern int OpenConnectionToMLLServer (const char *, const char *);

static char NODELOG_JOB[NODELOG_BUFSIZE];
static char NODELOG_MESSAGE[NODELOG_BUFSIZE];
static char NODELOG_LOOPEXT[NODELOG_BUFSIZE];
static char NODELOG_DATE[NODELOG_BUFSIZE];

static char username[32];
static char LOG_PATH[1024];
static char TMP_LOG_PATH[1024];
static char TOP_LOG_PATH[1024];

static int write_line(int sock, int top, const char* type);
static void gen_message (const char *job, const char *type, const char* loop_ext, const char *message);
static int sync_nodelog_over_nfs(const char *job, const char *type, const char* loop_ext, const char *message, const char *dtstmp, const char *logtype);
static  void NotifyUser (int sock , int top , char mode );
extern char* str2md5 (const char *str, int length);

static void log_alarm_handler() { fprintf(stderr,"=== EXCEEDED TIME IN LOOP ITERATIONS ===\n"); };
 
typedef enum {
           FROM_MAESTRO,
 	   FROM_NODELOGGER,
 	   FROM_MAESTRO_NO_SVR
} _FromWhere;
_FromWhere FromWhere;
 
void nodelogger(const char *job, const char* type, const char* loop_ext, const char *message, const char* datestamp)
{
   int i, sock=-1,ret, write_ret;
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
   char *logtocreate = NULL;
   int pathcounter=0;
   char *pathelement=NULL, *tmpbuf=NULL;

   if ( loop_ext == NULL ) { loop_ext=strdup(""); }
   if ( message  == NULL ) { message=strdup(""); }
    
   SeqUtil_TRACE( "nodelogger job:%s signal:%s message:%s loop_ext:%s datestamp:%s\n", job, type, message, loop_ext, datestamp);

   memset(LOG_PATH,'\0',sizeof LOG_PATH);
   memset(TMP_LOG_PATH,'\0',sizeof TMP_LOG_PATH);
   memset(TOP_LOG_PATH,'\0',sizeof TOP_LOG_PATH);
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
   if (p2 == NULL) {
     fprintf( stderr, "Nodelogger::ERROR: getpwnam error... returns null.\n" );
   }
   if( (seq_exp_home=getenv("SEQ_EXP_HOME")) == NULL ) {
      fprintf( stderr, "Nodelogger::ERROR: You must provide a valid SEQ_EXP_HOME.\n" );
      exit(1);
   }

   snprintf(LOG_PATH,sizeof(LOG_PATH),"%s/logs/%s_nodelog",seq_exp_home,datestamp);
   snprintf(TMP_LOG_PATH,sizeof(TMP_LOG_PATH),"%s/sequencing/sync/%s",seq_exp_home,datestamp);
   snprintf(TOP_LOG_PATH,sizeof(TOP_LOG_PATH),"%s/logs/%s_toplog",seq_exp_home,datestamp);

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
    
    pathcounter = 0;
    tmpbuf=strdup(NODELOG_JOB);
    pathelement = strtok(tmpbuf, "/");
    while (pathelement != NULL) {
       if (strcmp(pathelement, "") != 0) {
           pathcounter=pathcounter+1;
       }
       pathelement=strtok(NULL, "/");
    }

    /* if called inside maestro, a connection is already open  */
    tmpfrommaestro = getenv("FROM_MAESTRO");
    if ( tmpfrommaestro == NULL ) {
       SeqUtil_TRACE( "\n================= NODELOGGER: NOT_FROM_MAESTRO signal:%s================== \n",type);
       FromWhere = FROM_NODELOGGER;
       if ( (sock=OpenConnectionToMLLServer( job , "LOG" )) < 0 ) { 
          gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
          if ((pathcounter <= 1) || (strcmp(type, "abort") == 0) || (strcmp(type, "event") == 0) || (strcmp(type, "info") == 0) ) {
            logtocreate = "both";
          } else {
            logtocreate = "nodelog";
          }
          ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp, logtocreate);
          return;
       }
       /* install SIGALRM handler */
       /* set the signal handler */
       memset (&sa, '\0', sizeof(sa));
       sa.sa_handler = &log_alarm_handler;
       sa.sa_flags = 0; /* was SA_RESTART  */
       sigemptyset(&sa.sa_mask);
       /* register signals */
       if ( sigaction(SIGALRM,&sa,NULL) == -1 ) fprintf(stderr,"Nodelooger::error in registring SIGALRM\n");
    } else {
       SeqUtil_TRACE( "\n================= NODELOGGER: TRYING TO USE CONNECTION FROM MAESTRO PROCESS IF SERVER IS UP signal:%s================== \n",type);
       if ( MLLServerConnectionFid > 0 ) {
          FromWhere = FROM_MAESTRO;
          sock = MLLServerConnectionFid;
          SeqUtil_TRACE( "\n================= NODELOGGER: OK,HAVE A CONNECTION FROM MAESTRO PROCESS ================== \n");
       } else {
        /* it could be that we dont have the env. variable set to use Server */
           FromWhere = FROM_MAESTRO_NO_SVR;
	   if ( (sock=OpenConnectionToMLLServer( job , "LOG" )) < 0 ) { 
               SeqUtil_TRACE( "\n================= NODELOGGER: CANNOT ACQUIRE CONNECTION FROM MAESTRO PROCESS signal:%s================== \n",type);
               gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
               if ((pathcounter <= 1) || (strcmp(type, "abort") == 0) || (strcmp(type, "event") == 0) || (strcmp(type, "info") == 0) ) {
                 logtocreate = "both";
               } else {
                 logtocreate = "nodelog";
               }
               ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp, logtocreate);
               return;
            } else {
               SeqUtil_TRACE( "\n================= ACQUIRED A NEW CONNECTION FROM NODELOGGER PROCESS ================== \n");
            }
       }
    }
 
    /* if we are here socket is Up then why the second test > -1 ??? */ 
    gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
    if ( sock > -1 ) {
      if ((write_ret=write_line(sock, 0, type )) == -1) {
        logtocreate = "nodelog";
        ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp, logtocreate); 
      }
      if ((pathcounter <= 1) || (strcmp(type, "abort") == 0) || (strcmp(type, "event") == 0) || (strcmp(type, "info") == 0) ) {
         if ((write_ret=write_line(sock, 1, type )) == -1) {
           logtocreate = "toplog";
           ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp, logtocreate); 
         }
      }
      if (write_ret == -1) return;
    } else {
      if ((pathcounter <= 1) || (strcmp(type, "abort") == 0) || (strcmp(type, "event") == 0) || (strcmp(type, "info") == 0) ) {
        logtocreate = "both";
      } else {
        logtocreate = "nodelog";
      }
      ret=sync_nodelog_over_nfs(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE, datestamp, logtocreate);
      return;
    }
        /* @@@@@@@@@@@@@@@@@@@@@@@@@@@@@ CRITICAL  : CLOSE SOCKET ONLY WHEN NOT ACQUIRED FROM MAESTRO @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ */
    switch (FromWhere)
    {

       case FROM_NODELOGGER:
       case FROM_MAESTRO_NO_SVR:
	   ret=write(sock,"S \0",3);
           close(sock);
           SeqUtil_TRACE( "\n================= ClOSING CONNECTION FROM NODELOGGER PROCESS ================== \n");
	   break;
       default:
           break;
    }
}


/**
 * write a buffer to socket with timeout
 * and receive an ack 0 || 1
 */
static int write_line(int sock, int top, const char* type)
{

   int bytes_read, bytes_sent;
   int fileid;
   char bf[512];


   memset(bf, '\0', sizeof(bf));
   
   if (top == 0) {
     if ( ((bytes_sent=send_socket(sock , nodelogger_buf , sizeof(nodelogger_buf) , SOCK_TIMEOUT_CLIENT)) <= 0)) {
        fprintf(stderr,"%%%%%%%%%%%% NODELOGGER: socket closed at send  %%%%%%%%%%%%%%\n");
        return(-1);
     }
   } else {
     /* create tolog file */
     if ((fileid = open(TOP_LOG_PATH,O_WRONLY|O_CREAT,0755)) < 1 ) {
                        fprintf(stderr,"Nodelogger::could not create toplog:%s\n",TOP_LOG_PATH);
			/* return something */
     }

     if ( ((bytes_sent=send_socket(sock , nodelogger_buf_top , sizeof(nodelogger_buf_top) , SOCK_TIMEOUT_CLIENT)) <= 0)) {
       fprintf(stderr,"%%%%%%%%%%%% NODELOGGER: socket closed at send  %%%%%%%%%%%%%%\n");
       return(-1);
     }
   }

   if ( (bytes_read=recv_socket (sock , bf , sizeof(bf) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
     fprintf(stderr,"%%%%%%%%%%%% NODELOGGER: socket closed at recv   %%%%%%%%%%%%%%\n");
     return(-1);
   }
   if ( bf[0] != '0' ) {
     bf[bytes_read > 0 ? bytes_read : 0] = '\0';  
     fprintf(stderr,"Nodelogger::write_line: Error=%s bf=%s nodelogger_buf=%s\n",strerror(errno),bf,nodelogger_buf);
     return(-1);
   }
   /* Notify user if he is using nfs mode, this will be done at begin and end of root node 
      Note : we notify user when using inter-dependencies
   if ( top != 0 && (strcmp(type,"begin") == 0 || strcmp(type,"endx") == 0 ) ) NotifyUser ( sock , top ,'S' );  */
   

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
    memset(nodelogger_buf_notify, '\0', NODELOG_BUFSIZE );
    memset(nodelogger_buf_notify_short, '\0', NODELOG_BUFSIZE );
    SeqUtil_TRACE ( "\nNODELOGGER node:%s type:%s message:%s\n", node, type, message );

    if ( loop_ext != NULL ) {
        snprintf(nodelogger_buf,sizeof(nodelogger_buf),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQLOOP=%s:SEQMSG=%s\n",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,loop_ext,message);
	snprintf(nodelogger_buf_top,sizeof(nodelogger_buf_top),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQLOOP=%s:SEQMSG=%s\n",username,TOP_LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,loop_ext,message);
        snprintf(nodelogger_buf_short,sizeof(nodelogger_buf_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQLOOP=%s:SEQMSG=%s\n",c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,loop_ext,message);
	 snprintf(nodelogger_buf_notify,sizeof(nodelogger_buf_notify),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=info:SEQLOOP=%s:SEQMSG=",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,loop_ext); 
         snprintf(nodelogger_buf_notify_short,sizeof(nodelogger_buf_notify_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=info:SEQLOOP=%s:SEQMSG=",c_year,c_month,c_day,c_hour,c_min,c_sec,node,loop_ext); 
    } else {
        snprintf(nodelogger_buf,sizeof(nodelogger_buf),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQMSG=%s\n",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,message);
	snprintf(nodelogger_buf_top,sizeof(nodelogger_buf_top),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQMSG=%s\n",username,TOP_LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,message);
        snprintf(nodelogger_buf_short,sizeof(nodelogger_buf_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQMSG=%s\n",c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,message);
	snprintf(nodelogger_buf_notify,sizeof(nodelogger_buf_notify),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=info:SEQMSG=",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node); 
        snprintf(nodelogger_buf_notify_short,sizeof(nodelogger_buf_notify_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=info:SEQMSG=",c_year,c_month,c_day,c_hour,c_min,c_sec,node); 
    }
    /* root node is NOT a loop node , no need to duplicate under loop_ext 
    snprintf(nodelogger_buf_notify,sizeof(nodelogger_buf_notify),"L %-7s:%s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=info:SEQMSG=",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node);
    snprintf(nodelogger_buf_notify_short,sizeof(nodelogger_buf_notify_short),"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=info:SEQMSG=",c_year,c_month,c_day,c_hour,c_min,c_sec,node);
    */
    
}



/**
  *  Synchronize clients logging to Experiment nodelog file
  *  author : Rochdi Lahlou, cmoi 2013
  *  Algo   : hypotheses : nfs 2+, garanties that link and rename are atomic
  *           build on this to make clients writes to nodelog file based 
  *           on a timed round robin methods. 
  *
  *
  *
  */
static int sync_nodelog_over_nfs (const char *node, const char * type, const char * loop_ext, const char * message, const char * datestamp, const char *logtype)
{
    FILE *fd;
    struct passwd *ppass;
    struct stat fileStat;
    struct sigaction sa;
    int fileid,topfileid,num,nb,my_turn,ret,len,status=0;
    int success=0, loop=0;
    unsigned int pid;
    struct timeval tv;
    struct tm* ptm;
    struct stat st;
    static DIR *dp = NULL;
    struct dirent *d;
    time_t now;
    char *seq_exp_home=NULL, *truehost=NULL, *path_status=NULL, *mversion=NULL, *tmp_log_path=NULL;
    char resolved[MAXPATHLEN];
    char host[128], lock[1024],flock[1024],lpath[1024],buf[1024];
    char time_string[40],Stime[40],Etime[40],Atime[40];
    char ffilename[1024],filename[256];
    char host_pid[64],TokenHostPid[256],underline[2];
    double Tokendate,date,modiftime,actualtime;
    double diff_t;

    if (logtype == "both") {
       sync_nodelog_over_nfs(node,type,loop_ext,message,datestamp,"nodelog");
       sync_nodelog_over_nfs(node,type,loop_ext,message,datestamp,"toplog");
       return(0);
    }
    
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
     
    SeqUtil_TRACE( "\nToken date=%s host=%s pid=%u flock=%s\n",Stime,host,pid,flock);

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
          loop=loop+1;
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
		  if ( logtype == "toplog" ) {
		    if ((fileid = open(TOP_LOG_PATH,O_WRONLY|O_CREAT,0755)) < 1 ) {
                        fprintf(stderr,"Nodelogger::could not open toplog:%s\n",TOP_LOG_PATH);
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
                  }
		}else {
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
    /* Notify user if server not running  here we are at the level of root node so
       Only one task (root Node) is running
       Note : we notify user when using inter-dependencies
    if ( (strcmp(logtype,"toplog") == 0)  && (strcmp(type,"begin") == 0 || strcmp(type,"endx") == 0 ) ) NotifyUser ( -1 , 1 ,'N' ); */
    
    alarm(0);
    return(0);
}

/** 
* Notify User if he is using nfs mode, This notification will happen twice a run :
* in the beginning and end of root Node
*/
static void NotifyUser (int sock , int top , char mode)
{
   char bf[512];
   char *rcfile, *lmech ;
   int bytes_sent, bytes_read, fileid, num;

   if ( top != 0 ) {
         if ( (rcfile=malloc( strlen (getenv("HOME")) + strlen("/.maestrorc") + 2 )) != NULL ) {
	     sprintf(rcfile, "%s/.maestrorc", getenv("HOME"));
	     if ( (lmech=SeqUtil_getdef( rcfile, "SEQ_LOGGING_MECH" )) != NULL ) {
		  switch ( mode ) {
		        case 'S': /* mserver is up but logging mechanism is through NFS */
			         strcat(nodelogger_buf_notify,"Please initialize SEQ_LOGGING_MECH=server in ~/.maestrorc file\n");
	                         if ( strcmp(lmech,"nfs") == 0 ) {
                                     if ( ((bytes_sent=send_socket(sock , nodelogger_buf_notify , sizeof(nodelogger_buf_notify) , SOCK_TIMEOUT_CLIENT)) <= 0)) {
	                                     free(rcfile);free(lmech);
                                             return;
                                     }
                                     if ( (bytes_read=recv_socket (sock , bf , sizeof(bf) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
	                                     free(rcfile);free(lmech);
                                             return;
                                     }
		                  }
				  break;
		        case 'N': /* mserver is down and logging mech. is through NFS */
			         strcat(nodelogger_buf_notify_short,"Please start mserver and initialize SEQ_LOGGING_MECH=server in ~/.maestrorc file\n");
	                         if ( strcmp(lmech,"nfs") == 0 ) {
                                       if ( (fileid = open(LOG_PATH,O_WRONLY|O_CREAT,0755)) > 0 ) {
 	                                        off_t s_seek=lseek(fileid, 0, SEEK_END);
		                                num = write(fileid, nodelogger_buf_notify_short, strlen(nodelogger_buf_notify_short));
		                                fsync(fileid);
		                                close(fileid);
                                       } 
				 } 
				 break;
                  }
		  free(lmech);
	     }
	     free(rcfile);
	 }
   }

}
