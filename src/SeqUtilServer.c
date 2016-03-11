/* SeqUtilServer.c - Basic utilities used by the maestro daemon in the Maestro sequencer software package.
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
int removeFile_svr (const char *filename, const char * _seq_exp_home) {

   int status=0;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_REMOVE, filename ,"" ,_seq_exp_home); 

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.removeFile_svr() removing %s return:%d\n", filename,status );
   return (status);
}

/**
 * touch_svr : simulate a "touch" on a given file 'filename' through mserver
 */
int touch_svr (const char *filename , const char * _seq_exp_home) {
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_TOUCH, filename ,"" ,_seq_exp_home); 

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.touch_svr(): %s return:%d\n",filename,status);

   return (status);
}


/**
 * isFileExists_svr: test through server to see if  a given file 'filename' exist 
 * returns 1 if succeeds, 0 failure 
 */
int isFileExists_svr ( const char* lockfile, const char *caller, const char* _seq_exp_home ) {
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_IS_FILE_EXISTS, lockfile ,"" ,_seq_exp_home); 

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.isFileExists_svr()from %s filename:%s return:%d\n",caller,lockfile,status);
   return (status);
}


/**
 * access_svr: test through server to see if  a given file 'filename' exist 
 *             returns 0 if succeeds, 1 failure 
 */
int access_svr ( const char* filename , int mode , const char* _seq_exp_home) {
   int status;
   char string[2]; 
   
   memset(string,'\0',sizeof(string));
   sprintf(string,"%d",mode);
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_ACCESS, filename , string,_seq_exp_home); 

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.access_svr(): %s return:%d\n",filename,status);
   return (status);
}

/**
 * SeqUtil_mkdir_svr: create a directory 
 *             returns 1 if succeeds, 0 failure 
 */
int SeqUtil_mkdir_svr ( const char* dirname, int notUsed, const char * _seq_exp_home ) {
   int status;
 

   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_MKDIR ,dirname, "" ,_seq_exp_home); 

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.SeqUtil_mkdir_svr(): %s return:%d\n",dirname,status);
   return (status);

}

/**
* globPath_svr: test through server to see if  a given expression of pathnames
*               exists
*               returns  number of matchs,  or no match 
*/
int globPath_svr (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno) , const char * _seq_exp_home)
{
   int status;
  
   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_GLOB_PATTERN_COUNT ,pattern ,"" ,_seq_exp_home); 

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.globPath_svr():%s \n",pattern);
   return (status);

}

/**
  WriteNodeWaitedFile_svr
  Writes through the server and with the dependent to experiment mutex
  the dependency lockfile in the directory of the node that this current node is waiting for.
  
  Inputs:
     _dep_exp_path - the SEQ_EXP_HOME of the dependant node
     _dep_node     - the path of the node including the container
     _dep_status   - the status that the node is waiting for (end,abort,etc)
     _dep_index    - the loop index that this node is waiting for (.+1+6)
     _dep_scope    - dependency scope
      Sfile        - Status file of the dependent node
*/ 

int WriteNodeWaitedFile_svr ( const char* seq_exp_home, const char* nname, const char* datestamp, const char* loopArgs, 
                              const char* filename, const char* statusfile ) 
{
 
    char buffer[1024];
    int status;
   
    SeqUtil_TRACE(TL_FULL_TRACE,"maestro.writeNodeWaitedFile(): Using WriteNodeWaitedFile_svr routine\n");

    memset(buffer,'\0',sizeof(buffer));
    
    snprintf(buffer,sizeof(buffer),"sfile=%s wfile=%s exp=%s node=%s datestamp=%s args=%s",statusfile,filename,seq_exp_home, nname, datestamp, loopArgs);
    
    status = Query_L2D2_Server(MLLServerConnectionFid, SVR_WRITE_WNF, buffer , "" , seq_exp_home);
    
    SeqUtil_TRACE(TL_FULL_TRACE,"maestro.WriteNodeWaitedFile_svr():buffer=%s return=%d\n",buffer,status);

    return(status);

}


/**
*  WriteInterUserDepFile_svr
*  Routine to upload the inter user waited file to server, it will then 
*  write it under proper tree (interdepen/....) 
*  The routine split the buffer in chunks
*/
/*
int WriteInterUserDepFile_svr (const char *filename, const char *DepBuf, const char *ppwdir, const char *maestro_version, 
                               const char *datestamp, const char *md5sum) 
{
    char buffer[2048];
    char *to_send=NULL;
    int len,half,reste,status;
    int total=0, bytes_written=0, bytes_left=0;

    len=strlen(filename)+strlen(DepBuf)+strlen(ppwdir)+strlen(maestro_version)+strlen(datestamp)+strlen(md5sum)+1;
    half=(int) len/2;
  

    snprintf(buffer,sizeof(buffer),"fil=%s#dbf=%s#pwd=%s#mve=%s#m5s=%s#dst=%s",filename,DepBuf,ppwdir,maestro_version,md5sum,datestamp);
    if (to_send=(char *) malloc(half+5)) {
        snprintf(to_send,half+5,"K 1 %s",buffer);
    } else {
        raiseError("OutOfMemory exception in SeqUtilServer.WriteInterUserDepFile_svr()\n");
    }
    status = Query_L2D2_Server(MLLServerConnectionFid, SVR_WRITE_USERDFILE, to_send , "", seq_exp_home);  
    free(to_send);


    reste=strlen(&buffer[half]) + 5;
    if (to_send=(char *) malloc(reste)) {
        snprintf(to_send,reste,"K 2 %s",&buffer[half]);
    } else {
        raiseError("OutOfMemory exception in SeqUtilServer.WriteInterUserDepFile_svr()\n");
    }

    status = Query_L2D2_Server(MLLServerConnectionFid, SVR_WRITE_USERDFILE, to_send , ""); 
    free(to_send);

    return(status);
}
*/


/**
 * fopen_svr
 *   routine to download the waited_end (any size) file from server  
 *   to TMPDIR of client host, open it and give the handle
 *   to work with.
 * Note : socket mode is blocking
 */
FILE * fopen_svr ( const char * filename , int sock ) 
{
  FILE *fp;
  char *buffer; 
  char wfilename[1024];
  char tmp[1024];
  char csize[11];
  int  size;
  int  bytes_sent,bytes_read,error=0;

  unsigned int pid=getpid();

  if ( getenv("TMPDIR") != NULL ) {
          snprintf(wfilename,sizeof(wfilename),"%s/waitfile",getenv("TMPDIR"));
  } else { 
          snprintf(wfilename,sizeof(wfilename),"/tmp/waitfile.%d",pid);
  } 

  SeqUtil_TRACE(TL_FULL_TRACE,"fopen_svr(): wait file:%s===== \n",filename);

  /* build command */
  snprintf(tmp,sizeof(tmp),"Z %s",filename);
  if ( (bytes_sent=send_socket(sock , tmp , sizeof(tmp) , SOCK_TIMEOUT_CLIENT)) <= 0 ) {
	    SeqUtil_TRACE(TL_MEDIUM,"fopen_svr(): socket closed at send\n");
            SeqUtil_TRACE(TL_MEDIUM,"fopen_svr(): Reverting to nfs Routines\n");
	    fp=fopen_nfs(filename , sock);
            return (fp);
  }

  /* read (get) size of file */
  if ( (bytes_read=recv_socket(sock, csize, sizeof(csize), SOCK_TIMEOUT_CLIENT)) <= 0 ) {
             /* should revert to nfs routine */
             SeqUtil_TRACE(TL_MEDIUM,"fopen_svr(): Could not receive size of waited file ... Reverting to nfs Routines\n");
             fp=fopen_nfs(filename , sock);
             return (fp);
  } else {
             size=atoi(csize);
             SeqUtil_TRACE(TL_FULL_TRACE,"Received size of waited file=%d\n",size);
  }

  /* if size is 0 , server did not find the wait file, try nfs */
  if ( size == 0 ) {
             SeqUtil_TRACE(TL_MEDIUM,"fopen_svr(): received 0 size of waited file ... Reverting to nfs Routines\n");
             fp=fopen_nfs(filename , sock);
             return (fp);
  }

  /* allocate an extra 1 byte for null char */
  if ( (buffer=(char *) malloc( (1+size) * sizeof(char))) == NULL ) {
	     raiseError("ERROR: OutOfMemory in fopen_svr()\n");
             return(NULL); /* not reached */
  }
  

  /* ok , now receive the buffer */
  memset(buffer,'\0',1+size);
  if (  recv_full(sock,buffer,size) == 0 ) {
       if (  (fp=fopen(wfilename,"w+")) == NULL) {
	     free(buffer);
             SeqUtil_TRACE(TL_MEDIUM,"fopen_svr(): Could not open local (on host) waited_end file ... Reverting to nfs Routines\n");
	     fp=fopen_nfs(filename , sock);
             return (fp);
       }
       buffer[size]='\0';
       fwrite(buffer, sizeof(char) , size , fp);
       fflush(fp);
       fsync(fileno(fp));  /* fsync work with file descriptor */
       SeqUtil_TRACE(TL_FULL_TRACE, "\nReceived Buffer length=%d:%s\n",strlen(buffer),buffer);
       free(buffer); 
       fclose(fp); /* rewind at beg. of file */
  } else {
         SeqUtil_TRACE(TL_MEDIUM, "fopen_svr(): Could not download waited file:%s to host tmpdir ... Reverting to nfs Routines\n",filename);
	 free(buffer);
	 fp=fopen_nfs(filename , sock);
         return (fp);
  }

  /* now the waited_end file is local (TMPDIR) , open it a give the handle to routine ... */
  if ( (fp=fopen(wfilename,"r")) == NULL) { 
             SeqUtil_TRACE(TL_MEDIUM,"fopen_svr():Crap, Could not open local waited_end file:%s ... Reverting to nfs Routines\n",wfilename);
	     fp=fopen_nfs(filename , sock);
             return (fp);
  }

  return(fp);
}


/**
 * lock_svr: create a link  (on server /tmp) which will behave as a lock    
 * returns 1 if succeeds, 0 on failure 
 */
int lock_svr (  const char* filename , const char * datestamp, const char* _seq_exp_home ) {
   char *md5sum = NULL;
   int i,status;
   
   md5sum = (char *) str2md5(filename,strlen(filename));

   SeqUtil_TRACE(TL_FULL_TRACE,"\nLOCK_SVR() filename:%s md5sum=%s \n",filename,md5sum);

   /* try to acquire the link for 5*0.5 = 2.5 sec */
   for ( i=0 ; i < 5 ; i++ ) {
       status = Query_L2D2_Server(MLLServerConnectionFid, SVR_LOCK, md5sum , datestamp , _seq_exp_home); 
       if ( status == 0 ) { break; }
       usleep(500000);
   }

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.lock_svr() filename:%s datestamp:%s return:%d\n",filename,datestamp,status);

   return (status);

}

/**
 * unlock_svr: remove link  
 * returns 1 if succeeds, 0 on failure 
 */
int unlock_svr ( const char* filename , const char * datestamp , const char * _seq_exp_home) {
   char *md5sum=NULL;
   int status;

   md5sum = (char *) str2md5(filename,strlen(filename));

   SeqUtil_TRACE(TL_FULL_TRACE,"\nUNLOCK_SVR() filename:%s md5sum=%s \n",filename,md5sum);

   status = Query_L2D2_Server(MLLServerConnectionFid, SVR_UNLOCK, md5sum , datestamp , _seq_exp_home);

   SeqUtil_TRACE(TL_FULL_TRACE,"maestro.unlock_svr() filename:%s datestamp:%s return:%d\n",filename,datestamp,status);
   
   return (status);
}


