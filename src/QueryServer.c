#include <stdlib.h>
#include <signal.h>
#include <sys/param.h>
#include <glob.h>
#include "SeqUtil.h"
#include "l2d2_socket.h"
#include "SeqUtilServer.h"
#include "QueryServer.h"

/**
 * 
 * Name   : Query_L2D2_Server
 * Author : Rochdi lahlou, CMOI May 2012
 * Descri : Send locking,logging Requests to Server
 * Note   : Connection is open once in maestro.c
 * 
 */

int Query_L2D2_Server ( int sock , ServerActions action , const char *buf , const char *buf2 )
{
    int bytes_read, bytes_sent;
    int ret_nfs;
    char buffer[MAXBUF];
    char Rbuffer[3];

    /* reset buffer */
    memset(buffer,'\0',sizeof(buffer));
    memset(Rbuffer,'\0',sizeof(Rbuffer));


    /* ok compose the message to be sent to server */
    switch (action) {

           case SVR_ACCESS:
                            sprintf(buffer,"A %s",buf);
	                    break;
           case SVR_TOUCH:
                            sprintf(buffer,"T %s",buf);
	                    break;
           case SVR_REMOVE:
                            sprintf(buffer,"R %s",buf);
	                    break;
           case SVR_CREATE:
                            sprintf(buffer,"C %s",buf);
	                    break;
           case SVR_IS_FILE_EXISTS:
                            sprintf(buffer,"F %s",buf);
	                    break;
           case SVR_GLOBPATERN:
                            sprintf(buffer,"G %s",buf);
	                    break;
           case SVR_MKDIR:
                            sprintf(buffer,"D %s",buf);
	                    break;
           case SVR_LOCK:
                            sprintf(buffer,"N %s",buf);
	                    break;
           case SVR_UNLOCK:
                            sprintf(buffer,"P %s",buf);
	                    break;
           case SVR_WRITE_WNF:/* note format here !!! */
                            sprintf(buffer,"W %s",buf);
	                    break;
           case SVR_LOG_NODE:
                            sprintf(buffer,"L %s",buf);
	                    break;
           case SVR_WRITE_USERDFILE: /* K  */
                            sprintf(buffer,"%s",buf);
	                    break;
           default  :
	                    fprintf (stderr,"@@@@@@@@@@@ ERROR Inrecognized action for the Server:%d @@@@@@@@@@@ \n",action);
			    return(-1);
                            break;
    }

    if ( (bytes_sent=send_socket(sock , buffer , sizeof(buffer) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
           fprintf(stderr,"%%%%%%%%%%%% Query_L2D2_Server: socket closed at send  %%%%%%%%%%%%%%\n");
	   fprintf(stderr, "====== Reverting to Nfs Routines ====== \n");
	   ret_nfs=revert_nfs ( buf , action , buf2 );
	   return(ret_nfs);
    }
    
    memset(Rbuffer,'\0', sizeof(Rbuffer));

    if ( (bytes_read=recv_socket (sock , Rbuffer , sizeof(Rbuffer) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
           fprintf(stderr,"%%%%%%%%%%%% Query_L2D2_Server: socket closed at recv   %%%%%%%%%%%%%%\n");
	   fprintf(stderr, "====== Reverting to Nfs Routines ====== \n");
	   ret_nfs=revert_nfs ( buf , action , buf2 );
	   return(ret_nfs);
    } 

    

    Rbuffer[bytes_read-1] = '\0';

    /* 00 & 11 are the only response of 
     * the server. anything else mean problems 
    */
    switch(Rbuffer[0])
    {
         case '0': 
	 case '1':
	          break;
         default : /* something happend ...
	            * server went down in the middle of session ... 
		    * Try to recuperate with nfs stuff
		    */
	          fprintf(stderr, "====== Reverting to nfs Routines Answer for action=%d:>%s< ====== \n",action, Rbuffer);
		  ret_nfs = revert_nfs ( buf , action , buf2 );
		  return (ret_nfs);
    }

    return ( (int) Rbuffer[0] - '0');

}

/** 
 * If we detect that socket is down revert to  
 * NFS routines to keep the flow going 
 */
int revert_nfs (  const char * buf , ServerActions action , const char *buf2 ) 
{
        int ret=0;
        char sfile[1024],filename[1024],pwname[1024],seq_xp_home[1024], nname[1024], datestamp[1024], loopArgs[1024];
	char mversion[128],md5sum[128];

        switch(action)
        {
                      case SVR_ACCESS:
		                fprintf(stderr,"Nfs Routine: SVR_ACCESS cmd=%s\n",buf); 
		  	        ret=access_nfs (buf,R_OK ) ;
	                        break;
                      case SVR_TOUCH:
		                fprintf(stderr,"Nfs Routine: SVR_TOUCH cmd=%s\n",buf); 
			        ret=touch_nfs(buf) ;
	                        break;
                      case SVR_REMOVE:
		                fprintf(stderr,"Nfs Routine: SVR_REMOVE cmd=%s\n",buf); 
			        ret=removeFile_nfs(buf) ;
	                        break;
                      case SVR_CREATE:
		                fprintf(stderr,"Nfs Routine: SVR_CREATE cmd=%s\n",buf); 
			        ret=touch_nfs(buf) ;
	                        break;
                      case SVR_IS_FILE_EXISTS:
		                fprintf(stderr,"Nfs Routine: SVR_IS_FILE_EXISTS cmd=%s\n",buf); 
			        ret=isFileExists_nfs(buf,"from Querey_server" ) ;
	                        break;
                      case SVR_GLOBPATERN:
		                fprintf(stderr,"Nfs Routine: SVR_GLOBPATERN cmd=%s\n",buf); 
			        ret=globPath_nfs (buf, GLOB_NOSORT, 0);
	                        break;
                      case SVR_MKDIR:
		                fprintf(stderr,"Nfs Routine: SVR_MKDIR cmd=%s\n",buf); 
		                ret=SeqUtil_mkdir_nfs (buf,1) ;
	                        break;
		      case SVR_LOCK:
		                fprintf(stderr,"Nfs Routine: SVR_LOCK cmd=%s\n",buf); 
		                ret=lock_nfs ( buf, buf2 ) ;
	                        break;
		      case SVR_UNLOCK:
		                fprintf(stderr,"Nfs Routine: SVR_UNLOCK cmd=%s\n",buf); 
		                ret=unlock_nfs ( buf, buf2 ) ;
	                        break;
                      case SVR_LOG_NODE:
		                fprintf(stderr,"===NO NFS FOR LOGGING===\n");
				break;
                      case SVR_WRITE_WNF:
		                fprintf(stderr,"Nfs Routine: SVR_WRITE_WNF cmd=%s\n",buf); 
		                memset(sfile,'\0',sizeof(sfile));
		                memset(filename,'\0',sizeof(filename));
		                memset(pwname,'\0',sizeof(pwname));
		                memset(seq_xp_home,'\0',sizeof(seq_xp_home));
		                memset(nname,'\0',sizeof(nname));
		                memset(datestamp,'\0',sizeof(datestamp));
		                memset(loopArgs,'\0',sizeof(loopArgs));
		                ret = sscanf(buf,"sfile=%s wfile=%s user=%s exp=%s node=%s datestamp=%s args=%s",sfile,filename,pwname,seq_xp_home,nname,datestamp,loopArgs);
				ret = WriteNodeWaitedFile_nfs ( pwname, seq_xp_home, nname, datestamp, loopArgs, filename, sfile);
	                        break;
                      case SVR_WRITE_USERDFILE: /* have to be reviewed , if server shutdon we may only have the first chunk of transmission 
		                                    we need the 2 of them to resolve all variables */
		                fprintf(stderr,"Nfs Routine: SVR_WRITE_USERDFILE cmd=%s\n",buf); 
		                memset(sfile,'\0',sizeof(sfile));
		                memset(filename,'\0',sizeof(filename));
		                memset(pwname,'\0',sizeof(pwname));
		                memset(mversion,'\0',sizeof(mversion));
		                memset(md5sum,'\0',sizeof(md5sum));
		                memset(datestamp,'\0',sizeof(datestamp));
		                ret = sscanf(buf,"K file=%s Dbf=%s pwd=%s mv=%s m5s=%s dstmp=%s",filename,sfile,pwname,mversion,md5sum,datestamp);
				ret = WriteInterUserDepFile_nfs( filename, sfile, pwname, mversion, datestamp, md5sum);
	                        break;
                      default  :
	                       fprintf (stderr,"@@@@@@@@@@@ ERROR Inrecognized action for the Server:%d Buffer=%s @@@@@@@@@@@ \n",action,buf);
			       return(-1);
       }

       return(ret);
}

/**  
 * function : Open connection with Maestro L2D2 Server 
 *            and return handle.
 * objet    : open and maintain a persistente connection with the server
 *            - Client must go first throught a identification phase to log in
 *
 * author   : R. lahlou CMOI, 2012.
 * rev.     : 1.0
 */
int OpenConnectionToMLLServer ( char * node , char *signal )
{
    unsigned int pid;

    int sock;
    int bytes_sent,bytes_read,ret;
    int port;

    struct stat fileStat;
    struct passwd *passwdEnt = getpwuid(getuid());

    char buffer[1024],buf_id[1024];
    char username[50];
    static char ipserver[20];
    static char htserver[20];
    static char thisHost[100];
    char resolved[MAXPATHLEN];

    char *path_status=NULL;
    char *seq_exp_home=NULL;
    char *seq_date=NULL;
    char *Auth_token=NULL;
    char *mversion=NULL;
    char *m5sum=NULL;
    char authorization_file[256];

    if ( (mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
            fprintf(stderr,"Could not get maestro version env variable ...\n");
    }

    snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",mversion);
    Auth_token=get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);

    if ( Auth_token == NULL) {
	    fprintf(stderr, "Found No Maestro Server Parameteres File\n");
            return(-1);
    } else {
	    int nscan = sscanf(Auth_token, "seqpid=%u seqhost=%s seqip=%s seqport=%d", &pid, htserver, ipserver, &port);
	    fprintf(stderr, "maestro L2D2 server parameters are: <pid=%u  host=%s ip=%s port=%d>\n",pid,htserver,ipserver,port);
	    free(Auth_token);
    }

    if ( (sock=connect_to_host_port_by_ip (ipserver,port))  < 1 ) {
                  fprintf(stderr,"Cannot connect to host=%s ip=%s and port=%d  ... exiting\n",htserver,ipserver,port);
                  return(-1);
     }

    
    gethostname(thisHost, sizeof(thisHost));

    /* Build the authorization connection */

    seq_exp_home = getenv("SEQ_EXP_HOME");

    ret=do_Login(sock,pid,node,seq_exp_home,signal,passwdEnt->pw_name,&m5sum); 
    return (ret == 0 ? sock : -2);
}

/** 
 *  close connection  with Maestro L2D2 Server 
 */

void CloseConnectionWithMLLServer ( int con ) 
{
      /* int ret; */
      int bytes_sent;
      
      if ( (bytes_sent=send_socket(con , "S \0" , 3 , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
           fprintf(stderr,"%%%%%%%%%%%% CloseConnectionWithMLLServer: socket closed at send %%%%%%%%%%%%%%\n");
      } else close(con);

}

