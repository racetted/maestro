#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>        /* errno */
#include "SeqUtil.h"
#include "QueryServer.h"
#include "SeqUtilServer.h"
#include "l2d2_commun.h"


extern int MLLServerConnectionFid;

/**
 * removeFile_svr: Removes the named file 'filename' through mserver it returns 
 * zero if succeeds  and a nonzero value if it does not
 */
int removeFile_svr (const char *filename ) {

   int status=0;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_REMOVE, filename ,"" ); 

   fprintf( stdout,"maestro.removeFile_svr() removing %s return:%d\n", filename,status );
   return (status);
}

/**
 * touch_svr : simulate a "touch" on a given file 'filename' through mserver
 */
int touch_svr (const char *filename ) {
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_TOUCH, filename ,"" ); 

   fprintf(stdout,"maestro.touch_svr(): %s return:%d\n",filename,status);
   return (status);
}


/**
 * isFileExists_svr: test through server to see if  a given file 'filename' exist 
 * returns 1 if succeeds, 0 failure 
 */
int isFileExists_svr ( const char* lockfile, const char *caller ) {
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_IS_FILE_EXISTS, lockfile ,"" ); 

   fprintf(stdout,"maestro.isFileExists_svr()from %s filename:%s return:%d\n",caller,lockfile,status);
   return (status);
}


/**
 * access_svr: test through server to see if  a given file 'filename' exist 
 *             returns 0 if succeeds, 1 failure 
 */
int access_svr ( const char* filename , int mode ) {
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_ACCESS, filename ,"" ); 

   fprintf(stdout,"maestro.access_svr(): %s return:%d\n",filename,status);
   return (status);
}

/**
 * SeqUtil_mkdir_svr: create a directory 
 *             returns 1 if succeeds, 0 failure 
 */
int SeqUtil_mkdir_svr ( const char* dirname, int notUsed ) {
   int status;
 

   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_MKDIR ,dirname, "" ); 

   fprintf(stdout,"maestro.SeqUtil_mkdir_svr(): %s return:%d\n",dirname,status);
   return (status);

}

/********************************************************************************
* globPath_svr: test through server to see if  a given expression of pathnames
*               exists
*               returns  number of matchs, 0 failure or no match 
*********************************************************************************/
int globPath_svr (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno) )
{
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_GLOBPATERN ,pattern ,"" ); 

   fprintf(stdout,"maestro.globPath_svr():%s \n",pattern);
   return (status);

}
 /*
  WriteNodeWaitedFile_svr
  Writes through the server and with the dependent to experiment mutex
  the dependency lockfile in the directory of the node that this current node is waiting for.
  
  Inputs:
     _dep_user     - the user id where the dependant node belongs
     _dep_exp_path - the SEQ_EXP_HOME of the dependant node
     _dep_node     - the path of the node including the container
     _dep_status   - the status that the node is waiting for (end,abort,etc)
     _dep_index    - the loop index that this node is waiting for (.+1+6)
     _dep_scope    - dependency scope
      Sfile        - Status file of the dependent node
*/ 

int WriteNodeWaitedFile_svr ( const char* pwname, const char* seq_xp_home, const char* nname, const char* datestamp, const char* loopArgs, 
                              const char* filename, const char* statusfile ) 
{
 
    char buffer[1024];
    int status;
   
    fprintf(stderr,"maestro.writeNodeWaitedFile(): Using WriteNodeWaitedFile_svr routine\n");

    memset(buffer,'\0',sizeof(buffer));
    
    snprintf(buffer,sizeof(buffer),"sfile=%s wfile=%s user=%s exp=%s node=%s datestamp=%s args=%s",statusfile,filename,pwname,seq_xp_home, nname, datestamp, loopArgs);
    
    status = Query_L2D2_Server(MLLServerConnectionFid, SVR_WRITE_WNF, buffer , "" );
    
    fprintf(stdout,"maestro.WriteNodeWaitedFile_svr():buffer=%s return=%d\n",buffer,status);

    return(status);

}


/**
*
*
*/
int WriteInterUserDepFile_svr (const char *filename, const char *DepBuf, const char *ppwdir, const char *maestro_version, 
                               const char *datestamp, const char *md5sum) 
{
    char buffer[2048];
    char tbuffer[2048];
    char * to_send=NULL;
    int len,half,reste,status;
    int total=0, bytes_written=0, bytes_left=0;

    len=strlen(filename)+strlen(DepBuf)+strlen(ppwdir)+strlen(maestro_version)+strlen(datestamp)+strlen(md5sum)+1;
    half=(int) len/2;
  

    snprintf(buffer,sizeof(buffer),"fil=%s#dbf=%s#pwd=%s#mve=%s#m5s=%s#dst=%s",filename,DepBuf,ppwdir,maestro_version,md5sum,datestamp);
    if (to_send=(char *) malloc(half+5)){
        snprintf(to_send,half+5,"K 1 %s",buffer);
    } else {
        raiseError("OutOfMemory exception in SeqUtilServer.WriteInterUserDepFile_svr()\n");
    }
    status = Query_L2D2_Server(MLLServerConnectionFid, SVR_WRITE_USERDFILE, to_send , "");  
    free(to_send);

    reste=strlen(&buffer[half]) + 5;
    if (to_send=(char *) malloc(reste)){
        snprintf(to_send,reste,"K 2 %s",&buffer[half]);
    } else {
        raiseError("OutOfMemory exception in SeqUtilServer.WriteInterUserDepFile_svr()\n");
    }

    status = Query_L2D2_Server(MLLServerConnectionFid, SVR_WRITE_USERDFILE, to_send , ""); 
    free(to_send);


   if ( 1 == 0 ) {
    snprintf(tbuffer,sizeof(tbuffer),"fil=%s#dbf=%s#pwd=%s#mve=%s#m5s=%s#dst=%s",filename,DepBuf,ppwdir,maestro_version,md5sum,datestamp);
    len=strlen(tbuffer);

    /* snprintf(buffer,sizeof(buffer),"K %d %s",len,tbuffer); */
    snprintf(buffer,sizeof(buffer),"K %d",len);
    bytes_written=write(MLLServerConnectionFid, buffer, sizeof(buffer));

    memset(buffer,'\0',sizeof(buffer));
    snprintf(buffer,sizeof(buffer),"%s",tbuffer); 
    /* send the buffer add Timeout */
    bytes_left = strlen(buffer);
    while ( total < strlen(buffer) )
    {
               bytes_written = send(MLLServerConnectionFid, buffer+total, bytes_left, 0);
               
               if ( bytes_written == -1 ) { break;}
               total += bytes_written;
               bytes_left -= bytes_written;
    }

    /* synch */
    memset(buffer,'\0',sizeof(buffer));
    status=read(MLLServerConnectionFid,buffer,sizeof(buffer));
    fprintf(stderr,"WriteInterUserDepFile_svr : read %d value=%s\n",status,buffer);

    return (bytes_written == -1)  ? 1 : 0; 
   }

   return(status);
}

/**
 * GetFile
 *   routine to download the waited_end file to 
 *   TMPDIR of client host, open it and give the handle
 *   to work with.
 */
FILE * fopen_svr ( const char * filename , int sock ) 
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

    fprintf(stderr, "====== fopen_svr: wait file=%s===== \n",filename);

    /* build command */
    sprintf(tmp,"Z %s",filename);

   if ( (bytes_sent=send_socket(sock , tmp , sizeof(tmp) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
	    fprintf(stderr,"%%%%%%%%%%%% fopen_svr: socket closed at send  %%%%%%%%%%%%%%\n");
            fprintf(stderr, "====== fopen_svr: Reverting to nfs Routines ====== \n");
   }

   memset(buffer,'\0',sizeof(buffer));

  /* the size of the buffer is given as 11 digit number BUT we are
   * only able to handle 8K max !!!! Note 11 to grab space after number 

  if ( recv_full(sock,csize,11) == 0 && recv_full(sock,buffer,atoi(csize)) == 0 ) {
       if (  (fp=fopen(wfilename,"w+")) == NULL) {
             fprintf(stderr,"Could not open local waited_end file:%s\n",wfilename);
             return(NULL);
       }
       fwrite(buffer, sizeof(char) , strlen(buffer) , fp);
       fclose(fp); fp=NULL;
  } else {
         fprintf(stdout, "ERROR can not downlaod waited file:%s\n",filename);
         return (NULL);
  }
  */

  if ( (bytes_read=recv_socket(sock , buffer , sizeof(buffer) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
	    fprintf(stderr,"%%%%%%%%%%%% fopen_svr: socket closed at receive  %%%%%%%%%%%%%%\n");
            fprintf(stderr, "====== fopen_svr: Reverting to nfs Routines ====== \n");
	    fp=fopen_nfs(filename , sock);
            return (fp);
  } else {
            if (  (fp=fopen(wfilename,"w+")) == NULL) {
                  fprintf(stderr,"Could not open local waited_end file:%s\n",wfilename);
                  return(NULL);
            }
            fwrite(buffer, sizeof(char) , strlen(buffer) , fp);
            fclose(fp); fp=NULL;

  }

  /* now the waited_end file is local (TMPDIR) , open it a give the
   * handle to routine ... */
  if ( (fp=fopen(wfilename,"r")) == NULL) { 
             fprintf(stderr,"Crap .... Could not open local waited_end file:%s\n",wfilename);
             return(NULL);
  }

  return(fp);
}

/**
 * lock_svr: create a link  which will behave as a lock    
 * returns 1 if succeeds, 0 failure 
 */
int lock_svr (  const char* filename , const char * datestamp ) {
   char *md5sum = NULL;
   int status;
   
   md5sum = (char *) str2md5(filename,strlen(filename));

   fprintf(stderr,"\nLOCK_SVR() filename:%s md5sum=%s \n",filename,md5sum);

   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_LOCK, md5sum , datestamp ); 

   fprintf(stdout,"maestro.lock_svr() filename:%s datestamp:%s return:%d\n",filename,datestamp,status);

   return (status);

}

/**
 * unlock_svr: remove link  
 * returns 1 if succeeds, 0 failure 
 */
int unlock_svr ( const char* filename , const char * datestamp ) {
   char *md5sum=NULL;
   int status;

   md5sum = (char *) str2md5(filename,strlen(filename));

   fprintf(stderr,"\nUNLOCK_SVR() filename:%s md5sum=%s \n",filename,md5sum);

   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_UNLOCK, md5sum , datestamp );

   fprintf(stdout,"maestro.unlock_svr() filename:%s datestamp:%s return:%d\n",filename,datestamp,status);
   
   return (status);
}


