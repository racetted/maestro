#include "nodelogger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <errno.h>
#include <pwd.h>
#include <libgen.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/param.h>

#if defined(Mop_f)

#elif defined(Mop_linux)

#include <strings.h>

#else

#include <strings.h>

#endif

#define NODELOG_BUFSIZE 1024
#define NODELOG_DEBUG 1
#define NODELOG_S_FPERM 00666
#define NODELOG_FILE_LENGTH 500

/* global variables */
static int nodelogger_exit_status = 0;
static char nodelogger_buf_socketdown[] = "socket is down and out";
static char nodelogger_buf[NODELOG_BUFSIZE];
/* static char nodelogger_batch_host[] = "castor.cmc.ec.gc.ca"; */
char nodelogger_batch_host[MAXHOSTNAMELEN];
static char NODELOG_JOB[NODELOG_BUFSIZE];
static char NODELOG_MESSAGE[NODELOG_BUFSIZE];
static char NODELOG_LOOPEXT[NODELOG_BUFSIZE];
static char NODELOG_DATE[NODELOG_BUFSIZE];

static char nodelogger_socketdown[NODELOG_FILE_LENGTH];
static char nodelogger_bucket[NODELOG_FILE_LENGTH];
static char nodelogger_lockfile[NODELOG_FILE_LENGTH];
static char nodelogger_debugfile[NODELOG_FILE_LENGTH];
static char nodelogger_timelog[NODELOG_FILE_LENGTH];
static char nodelogger_svr_file[NODELOG_FILE_LENGTH];
static char username[20];
static char LOG_PATH[200];

static int nodelogger_pid, nodelogger_lockfileid=-999, nodelogger_bucketid=-999;
static int PORT_NUM;

static FILE  *nodelogger_dbugfile;

static int open_socket();
static int write_line(int sock);
static void gen_message (char *job, char *type, char* loop_ext, char *message);
static void timeout();
static void timeout_sockdown();
static int sockdown();
static void dbug_write (char *message1,char *message2);
static int dbug_open();
static int process_bucket(int sock);
static int open_lock_bucket();
static int save_msg_2_bucket();
static int check_socket();
static void die();

int check_op_cmcfi(int gid)
/* return 1 if user belongs to (cmcfi=204 or cmcfip=243)
*  else return 0
*/
{
  if ( gid == 204 || gid == 243 ) {
      return(1);
  } else {
      return(0);
  }
}

int check_op_user(int gid)
/* return 1 if user belongs to (cmcfi=204, cmcfip=243, cmcfix=227, cmcfa=226, afse=234)
*  else return 0
*/
{
  if ( gid == 204 || gid == 243 || gid == 227 || gid == 226 || gid == 234 ) {
      return(1);
  } else {
      return(0);
  }
}

void nodelogger(char *job,char* type,char* loop_ext, const char *message, char* datestamp)
{
   int i, sock;
   int send_success = 0;
   char *tmphost=NULL;
   char *tmpenv = NULL;
   char *tmpusername = NULL;
   char cmcnodelogger[25];
   char tmp[10];
   struct stat stbuf;
   char *seq_exp_home = NULL;
   char *basec = NULL;
   struct passwd *p;
   struct passwd *p2;
   int gid = 0;
   char *experience;
   char line[1024];
   FILE *infoFile = NULL;

   if( loop_ext == NULL ) {
      loop_ext=strdup("");
   }
   if( message == NULL ) {
      message=strdup("");
   }
   SeqUtil_TRACE( "nodelogger job:%s signal:%s message:%s loop_ext:%s datestamp:%s\n", job, type, message, loop_ext, datestamp);
   memset(nodelogger_socketdown,'\0',NODELOG_FILE_LENGTH);
   memset(nodelogger_bucket,'\0',NODELOG_FILE_LENGTH);
   memset(nodelogger_lockfile,'\0',NODELOG_FILE_LENGTH);
   memset(nodelogger_debugfile,'\0',NODELOG_FILE_LENGTH);
   memset(nodelogger_timelog,'\0',NODELOG_FILE_LENGTH);
   memset(nodelogger_svr_file,'\0',NODELOG_FILE_LENGTH);

   memset(LOG_PATH,'\0',sizeof LOG_PATH);
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
      fprintf( stderr, "ERROR: You must provide a valid SEQ_EXP_HOME!!\n" );
      exit(1);
   }

   sprintf(LOG_PATH,"%s/logs/%s_nodelog",seq_exp_home,datestamp);

   /* make a duplicatte of seq_exp_home because basename may return pointers */
   /* to statically allocated memory which may be overwritten by subsequent calls.*/
   basec = strdup(seq_exp_home);
   experience = basename(basec);
   sprintf(nodelogger_socketdown,"%s%s%s","/tmp/",username,".maestro_socketdown");
   sprintf(nodelogger_bucket,"%s%s%s","/tmp/",username,".maestro_bucket");
   sprintf(nodelogger_lockfile,"%s%s%s","/tmp/",username,".maestro_log_lockfile");
   sprintf(nodelogger_debugfile,"%s%s%s","/tmp/",username,".maestro_log_dbugfile");
   strcpy(nodelogger_svr_file, getenv("HOME"));
   strcat(nodelogger_svr_file,"/.suites/.nodelogger_client");

   /* gets server info port & host from file_svr_info */
   if ((infoFile = fopen(nodelogger_svr_file,"r")) != NULL ) {
      if( fgets( line, sizeof(line), infoFile ) != NULL ) {
         sscanf( line, "seqhost=%s seqport=%d", &nodelogger_batch_host, &PORT_NUM );
      }
      fclose( infoFile );
   } else {
      fprintf( stderr, "ERROR: Cannot open svr info file:%s\n", nodelogger_svr_file );
      return;
   }

    /* setup an alarm so that if the logging is stuck
     * it will timeout after 60 seconds. This will prevent the
     * processes from hanging when there are network problems.
     */

    nodelogger_exit_status = 0;
   
    if (NODELOG_DEBUG)
    {
	if (dbug_open() == 1) {
           return;
        }
	nodelogger_pid = getpid();
        dbug_write("userID=",username);
        dbug_write("node=",job);
	dbug_write("msg=",message);
    }

    memset(NODELOG_JOB,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_JOB,job);
    memset(NODELOG_LOOPEXT,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_LOOPEXT,loop_ext);
    memset(NODELOG_MESSAGE,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_MESSAGE,message);
    memset(NODELOG_DATE,'\0',NODELOG_BUFSIZE);
    strcpy(NODELOG_DATE,datestamp);

    tmpenv = getenv("CMCNODELOG");

    if ( tmpenv != NULL )
    {
	for(i=0;i<strlen(tmpenv);++i) cmcnodelogger[i]=toupper(tmpenv[i]);
	cmcnodelogger[i]='\0';
    }

    if (strcmp(cmcnodelogger,"ON") != 0)
    {
	SeqUtil_TRACE("\nWARNING: nodelogger - CMCNODELOG debug mode\n");
	gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
	SeqUtil_TRACE("message=%s\n",nodelogger_buf);
	return;
    }

    if (NODELOG_DEBUG) dbug_write("passed CMCNODELOG check","");

    /* if the lockfile "nodelogger_socketdown" exists, there is a problem writing to the
     * castor socket. In this case, append the message to the bucket and
     * make a quick attempt to see if the socket is reachable. 
     *
     * With irix 6.4, /tmp is now a sticky-bit directory. You can no longer
     * remove files belonging to another user. If the nodelogger_socketdown file exists
     * AND it is not empty, there is a problem. 
     */
    if ( stat(nodelogger_socketdown,&stbuf) == 0 )
    {
	if ( stbuf.st_size > 0 )
	{
	    if (NODELOG_DEBUG) dbug_write("socketdown file detected....msg appended to bucket","");

	    /* generate a formatted msg into "nodelogger_buf" */
	    gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
	    if (save_msg_2_bucket() == 1) {
                if (NODELOG_DEBUG) fclose(nodelogger_dbugfile);
                return;
	    }

            /* verify that the socket is still not responding */
	    if (check_socket() == 1) {
                if (NODELOG_DEBUG) fclose(nodelogger_dbugfile);
                return; 
	    }
            if (NODELOG_DEBUG) fclose(nodelogger_dbugfile);
            return;
	}
    }

    /* set a 1 minute alarm on open_socket */
    signal(SIGALRM,timeout_sockdown);
    alarm(30);
    sock = open_socket();
    alarm(0); /* cancel alarm */
    if (nodelogger_exit_status == 1) {
       return;
    }

    if (NODELOG_DEBUG)
    {
	sprintf(tmp,"%d",sock);
	dbug_write("socket opened",tmp);
    }

    if ( sock > -1 ) /* verify open_sock worked properly */
    {
	/* check if the bucket is empty or not. If it contains data,
	 * process the contents {or at least try} 
	 */
	if (stat(nodelogger_bucket,&stbuf) == -1 ) {
	    if (NODELOG_DEBUG) dbug_write("nodelogger-warning: unable to stat ", nodelogger_bucket);
	    if ( creat(nodelogger_bucket, NODELOG_S_FPERM ) < 1 )
	    {
		fprintf(stderr,"nodelogger: unable to create %s\n",nodelogger_bucket);
	    }
	} else {
	    if (stbuf.st_size > 0) {
		send_success = process_bucket(sock);
                if (nodelogger_exit_status == 1) {
                   return;
		 }
	    }
	}

	/* generate a formatted msg into "nodelogger_buf" */
	gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);

	if (NODELOG_DEBUG) dbug_write("message generated","");

	/* if process_bucket failed, then the socket is down or something
	 * is broken, so just append the message to the bucket, then get out
         */
	if ( send_success == 0 )
	{
	    /* write_line: writes the contents of "nodelogger_buf" to the socket */
	    /* If write_line works, it returns a "0". If not, it returns */
	    /* a "-1", in which case, exit the loop and close the files */

	    if (NODELOG_DEBUG) dbug_write("writing buf","");
	    if ( write_line(sock) == -1 )
	    {
		if (NODELOG_DEBUG) dbug_write("write_line failed",nodelogger_buf);
		if (save_msg_2_bucket() == 1) {
                   return;
	        }
	    }
            if (nodelogger_exit_status == 1) {
               return;
	    }
	} else {
	    if (NODELOG_DEBUG) dbug_write("process_bucket failed",nodelogger_buf);
	    if (save_msg_2_bucket() == 1) {
               return;
	     }
	}

    } else {
	/* generate a formatted msg into "nodelogger_buf" */
	if (NODELOG_DEBUG) dbug_write("open bucket failed","");
	gen_message(NODELOG_JOB, type, loop_ext, NODELOG_MESSAGE);
	if (save_msg_2_bucket() == 1) {
           return;
	 }
	sockdown(); /* set a lockfile to indicate the socket is unavailable */
    }
    close(sock);
    close(nodelogger_bucketid);

    if (NODELOG_DEBUG) fclose(nodelogger_dbugfile);
}

static int open_socket()
{
    int sock;
    /* struct servent *service_entry = NULL; */
    struct sockaddr_in server;
    struct hostent *hostentry = NULL;
    int port;

    if( (nodelogger_batch_host==NULL) || (gethostbyname(nodelogger_batch_host) ==  NULL))
    {
	printf("Bad batch host");
	return(-1);
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
	printf("Cannot open stream socket");
	return(-1);
    }

    if (fcntl(sock, F_SETFL, O_SYNC) == -1)
    {
	printf("fcntl error, command not executed");
	return(-1);
    }

    /* harwire the port entry to eliminate yp-traffic */
    port=PORT_NUM;
    memset(&server,'\0',sizeof server);

    server.sin_family= AF_INET;
    server.sin_port=htons(port);
    hostentry = gethostbyname(nodelogger_batch_host);
    memcpy( (char*)&server.sin_addr.s_addr, hostentry->h_addr, hostentry->h_length);

    if (connect(sock,&server,sizeof server)<0)
    {
	printf("send_line: errno=%d\n",errno);
	printf("Cannot connect socket, no connection available!\n");
        printf("LOG SERVER HOST=%s\n",nodelogger_batch_host);
        printf("PORT=%d\n",PORT_NUM);
	return(-1);
    }
 
    return(sock);
}

static int write_line(int sock)
{

    /* write_line: write "nodelogger_buf" to "sock" */

    int num;
    alarm(30);
    num = write(sock, nodelogger_buf, sizeof(nodelogger_buf));
    alarm(0); /* Cancel the alarm */
    if (NODELOG_DEBUG) fprintf(nodelogger_dbugfile,"write_line: %d write_line num=%d\n",nodelogger_pid,num);
    if ( num <= 0)
    {
	printf("nodelogger: write_line: errno=%d\n",errno);
	printf("nodelogger: Cannot send command\n");
        printf("nodelogger: LOG SERVER HOST=%s\n",nodelogger_batch_host);
	return(-1);
    }

    return(0);
}

static void gen_message (node, type, loop_ext, message)
char *node;
char *type;
char *loop_ext;
char *message;
{
    /* gen_message: write a formatted log message */

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
   /*
    for (i=0; i < joblength; i++)
	nodepath[i] = toupper(node[i]);
   */
    /* write the message into "nodelogger_buf", which will be sent
     *  to a socket
     */
    memset(nodelogger_buf, '\0', NODELOG_BUFSIZE );

    if ( PORT_NUM == 33000 ) {
       /* sprintf(nodelogger_buf,"%.2d/%.2d%.2d:%.2d:%.2d:%c%-14s:%-90s\n",c_month,c_day,c_hour,c_min,c_sec,toupper(host),nodepath,message); */
    } else {
       printf ("NODELOGGER node:%s type:%s message:%s\n", node, type, message );
       if( loop_ext != NULL ) {
         sprintf(nodelogger_buf,"%-7s:%-200s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQLOOP=%s:SEQMSG=%s\n",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,loop_ext,message);
      } else {
         sprintf(nodelogger_buf,"%-7s:%-200s:TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d:SEQNODE=%s:MSGTYPE=%s:SEQMSG=%s\n",username,LOG_PATH,c_year,c_month,c_day,c_hour,c_min,c_sec,node,type,message);
      }
    }
}

static void timeout()
{
    /* timeout: write a timeout message to a logfile 
     *          then append the current message to the bucket */

    char command[256];

    if (NODELOG_DEBUG) dbug_write("timeout:","");

    gen_message(NODELOG_JOB, "timeout", NODELOG_LOOPEXT, NODELOG_MESSAGE);
    if (save_msg_2_bucket() == 1) return;

    printf("nodelogger: timeout\n");
    memset(command,'\0',256);
    sprintf(command,"echo nodelogger timeout: `date` >> %s",nodelogger_timelog);
    system(command);

    if (NODELOG_DEBUG) dbug_write("timeout: ","timeout timeout timeout timeout");
    if (NODELOG_DEBUG) fclose(nodelogger_dbugfile);
    nodelogger_exit_status = 1;
}

static void timeout_sockdown()
{
    if (NODELOG_DEBUG) dbug_write("timeout_sockdown: ","unable to open socket");
    sockdown();
    timeout();
}

static int sockdown()
{
    /* sockdown: set and write to a lockfile indicating a problem writing to 
     *           the castor socket.
     */

    int status_create=0, num;
    if (NODELOG_DEBUG) dbug_write("sockdown: ","unable to open socket");

    /* create/reset the lockfile */
    status_create = creat(nodelogger_socketdown, NODELOG_S_FPERM );
    if ( status_create < 1 )
    {
       fprintf(stderr,"nodelogger: unable to create %s\n",nodelogger_socketdown);
    }

    chown (nodelogger_socketdown, 119,204);
    /* write anything to it */
    num = write(status_create, nodelogger_buf_socketdown, strlen(nodelogger_buf_socketdown));
    if ( num <= 0)
    {
	printf("nodelogger: sockdown: errno=%d\n",errno);
	printf("nodelogger: cannot write to %s\n",nodelogger_socketdown);
        printf("nodelogger: LOG SERVER HOST=%s\n",nodelogger_batch_host);
	return(-1);
    }
    return(0);
}

static void dbug_write (message1, message2)
char *message1;
char *message2;
{

    /* debug_line: write a formatted log message */

    int c_year, c_month, c_day, c_hour, c_min, c_sec;
    time_t time();
    time_t timval;

    struct tm *localtime(), *tmptr;

    (void) time(&timval);
    tmptr = localtime(&timval);

    /* obtain the current date */

    c_year  = tmptr->tm_year;
    c_month = tmptr->tm_mon+1;
    c_day   = tmptr->tm_mday;
    c_hour  = tmptr->tm_hour;
    c_min   = tmptr->tm_min;
    c_sec   = tmptr->tm_sec;

    fprintf(nodelogger_dbugfile,"TIMESTAMP=%.4d%.2d%.2d.%.2d:%.2d:%.2d %d %s%s\n",c_year, c_month,c_day,c_hour,c_min,c_sec,nodelogger_pid,message1,message2);
}

static int dbug_open ()
{
    nodelogger_dbugfile = fopen(nodelogger_debugfile, "a+");
    if ( nodelogger_dbugfile == NULL )
    {
        fprintf(stderr,"nodelogger: Cannot open file: %s\n",nodelogger_debugfile);
        fprintf(stderr,"nodelogger: LOG SERVER HOST=%s\n",nodelogger_batch_host);
	return(1);
    }
    chmod (nodelogger_debugfile, 00666);
    /* do not close the nodelogger_dbugfile here because we are writting to it */
    return(0);
}

static int process_bucket (sock)
int sock;
{

    int send_success = 0;
    alarm(20);
    if (open_lock_bucket() == 1) {
      alarm(0);
      return(1);
    }
    alarm(0);
    memset(nodelogger_buf, '\0', NODELOG_BUFSIZE );
    while ( read(nodelogger_bucketid, nodelogger_buf, NODELOG_BUFSIZE ) > 0 )
    {

	/* send_line: sends the contents of "nodelogger_buf" to the socket */
	/* If send_line works, it returns a "0". If not, it returns */
	/* a "1", in which case, exit the loop and close the files */

	if (NODELOG_DEBUG) dbug_write("process_bucket: bucket read",nodelogger_buf);
	if (NODELOG_DEBUG) dbug_write("process_bucket: bucket read-terminator",nodelogger_buf);
	alarm(30);
	if ( write_line(sock) == -1 )
	{
	    if (NODELOG_DEBUG) dbug_write("process_bucket: write_line failed",nodelogger_buf);
	    send_success = -1;
	    break;
	}
        if (nodelogger_exit_status == 1) {
	  alarm(0);
	  return(1);
	}
	memset(nodelogger_buf, '\0', NODELOG_BUFSIZE );

    }

    alarm(0); /* cancel alarm */

    if ( send_success == 0 )
    {
	if (NODELOG_DEBUG) dbug_write("process_bucket: closing and truncating bucket","");
	close(nodelogger_bucketid);
	nodelogger_bucketid = open(nodelogger_bucket, O_TRUNC, NODELOG_S_FPERM);
    }

    return(send_success);

}

static int open_lock_bucket ()
{

    /* first thing to do is lock the bucket */

    if ( (nodelogger_lockfileid = open(nodelogger_lockfile, O_RDWR|O_CREAT, NODELOG_S_FPERM )) < 1 )
    {
	perror("nodelogger: can't open lockfile....log message discarded");
        nodelogger_exit_status = 1;
	return(1);
    }

    if (NODELOG_DEBUG) dbug_write("open_lock_bucket: opened lockfile ", nodelogger_lockfile);

    if ( lockf(nodelogger_lockfileid,F_LOCK,0L) < 0 )
    {
	printf("errno=%d\n",errno);
	perror("nodelogger: can't lock the lockfile....log message discarded");
        nodelogger_exit_status = 1;
        close(nodelogger_lockfileid);
        return(1);
    }
    if (NODELOG_DEBUG) dbug_write("open_lock_bucket: lockfile locked","");

    if ( (nodelogger_bucketid = open(nodelogger_bucket, O_RDWR|O_CREAT, NODELOG_S_FPERM )) < 1 )
    {
	fprintf(stderr,"nodelogger: can't open bucket=%s\n",nodelogger_bucket);
        nodelogger_exit_status = 1;
	return(1);
    }
 
    return(0);
}

static int save_msg_2_bucket ()
{

    /* simply append nodelogger_buf to the bucket */

    if (NODELOG_DEBUG) dbug_write("save_msg_2_bucket:","");

    /* if nodelogger_bucketid is not already open, it will have a value of -999 */
    if ( nodelogger_bucketid == -999 ) {
      alarm(20);
      if (open_lock_bucket() == 1) {
	alarm(0);
	return(1);
      }
      alarm(0);
    }
    lseek(nodelogger_bucketid, 0, SEEK_END);

    write(nodelogger_bucketid, nodelogger_buf, sizeof(nodelogger_buf));
    if (NODELOG_DEBUG) dbug_write("save_msg_2_bucket:"," message appended to bucket");

    close(nodelogger_bucketid);
    close(nodelogger_lockfileid);
    return(0);
}

static int check_socket ()
{

    /* check if the socket is responding */

    int sock, status_create=0;
    char *seq_exp_home = NULL;
    char mesg[NODELOG_BUFSIZE];
    char command[500];
    signal(SIGALRM, die);
    alarm(10);
    if (NODELOG_DEBUG) dbug_write("check_socket:","");
    sock = open_socket() ;
    alarm(0);
    if (nodelogger_exit_status == 1) return(1);
    if ( sock > -1 )
    {
	/* "creat" on an existing file sets the file length to zero */
	status_create = creat(nodelogger_socketdown, NODELOG_S_FPERM );
	if ( status_create < 0 )
	{
	    fprintf(stderr,"nodelogger: can't truncate %s\n",nodelogger_socketdown);
	}

	close(status_create);
	alarm(30);
        memset(mesg,'\0',sizeof mesg);
        memset(command,'\0',sizeof command);
        sprintf(mesg,"%s%s.","info: log server socket active... previous log entries resent from ", getenv("HOST"));
	gen_message("", "info", "", mesg);
        write_line(sock);
        /*
        seq_exp_home=getenv("SEQ_EXP_HOME");
        if (seq_exp_home != NULL) {
           sprintf(command,"%s=%s; nodelogger exnodelogger x \"%s\"","export",seq_exp_home,mesg);
        } else {
           sprintf(command,"nodelogger exnodelogger x \"%s\"",mesg);
        }
	system(command);
	*/
	if (NODELOG_DEBUG) dbug_write("check_socket:",mesg);
	alarm(0);
	close(sock);
    }
    if (nodelogger_exit_status == 1) return(1);
return(0);
}

static void die()
{
    if (NODELOG_DEBUG) dbug_write("die:","");
    nodelogger_exit_status = 1;
}







