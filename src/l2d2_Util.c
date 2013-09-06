#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <dirent.h>
#include <limits.h>
#include <fcntl.h>
#include <stdarg.h>
#include <grp.h>
#include <pwd.h>
#include <glob.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/param.h>
#include "l2d2_roxml.h"
#include "l2d2_server.h"
#include "l2d2_Util.h"

extern _l2d2server L2D2;
static void pdir (const char * dir_name);

/**
* Copied from SeqUtil
*
*
*/
char *getPathLeaf (const char *full_path) {
   char *split,*work_string,*chreturn =NULL;
   work_string = strdup(full_path);
   split = strtok (work_string,"/");
   while (split != NULL) {
     if ( chreturn != NULL ) {
       /* free previous allocated memory */
       free( chreturn );
     }
     chreturn = strdup (split);
     split = strtok (NULL,"/");
   }
   free( work_string );
   return chreturn;
}

/**
 * Name        : removeFile
 * Author      : R.Lahlou , cmoi 2012
 * 
 */
int removeFile(char *filename) {
   int status=0;
   
   status = remove(filename);
   return(status);
}

/**
 * Name        : CreateLock
 * Author      : R.Lahlou , cmoi 2012
 * Description : touch a file (calls touch) if file
 *               not there
 */
int CreateLock (char *filename) {
   int status=0;

   if ( access (filename, R_OK) != 0 ) {
               /* create the node begin lock file name */
                status = touch (filename);
   } else {
               /* file lock exist */
                status =  0;
   }

   return (status);
}

/**
 * Name        : touch
 * Author      : copied form SeqUtil.c 
 * Description : touch a file 
 */
int touch (char *filename) {
   FILE *actionfile;
   

   if ( access (filename, R_OK) == 0 )  return(0);
    

   if ((actionfile = fopen(filename,"r")) != NULL ) {
            fclose(actionfile);
            utime(filename,NULL); /*set the access and modification times to current time */
   } else {
      if ((actionfile = fopen(filename,"w+")) == NULL) 
      {
        fprintf(stderr,"touch: Cannot touch file:%s\n",filename);
        return(1);
      }
      
      fclose(actionfile);
      chmod(filename,00664);
   }

   return(0); 
}

/*
 * Name        : isFileExists
 * Author      : copied form SeqUtil.c
 * Description : check if file exists 
 */
int isFileExists( const char* lockfile ) {

   if ( access(lockfile, R_OK) == 0 ) return(1);
   return (0);
}

/**
 * Name        : Access
 * Author      : R.Lahlou , cmoi 2012
 * Description :  
 */
int Access ( const char* lockfile ) {

   if ( access(lockfile, R_OK) == 0 ) return (0);
   return (1);
}

/**
 * Name        : isDirExists
 * Author      : copied from SeqUtil.c 
 * Description : check if directory exists 
 */
int isDirExists ( const char* path_name ) {

   DIR *pDir = NULL;
   int Exists = 0;
   int ret;

   if ( path_name == NULL) return 0;
   pDir = opendir (path_name);

   if (pDir != NULL)
   {
      Exists = 1;
      ret=closedir(pDir);
   }

   return (Exists);
}

/**
 * create a directory  1+ level 
 */
int r_mkdir ( const char* dir_name, int is_recursive ) {
   char tmp[1000];
   char *split = NULL, *work_string = NULL;
  
   if ( is_recursive == 1) {
      work_string = strdup( dir_name );
      strcpy( tmp, "/" );
      split  = strtok( work_string, "/" );
      if( split != NULL ) {
         strcat( tmp, split );
      }
      while ( split != NULL ) {
         strcat( tmp, "/" );
         if( ! isDirExists( tmp ) ) {
            if ( mkdir(tmp,0755 ) == -1 ) {
                 fprintf (stderr, "ERROR: %s\n", strerror(errno) );
                 return(EXIT_FAILURE);
            }
         }

         split = strtok (NULL,"/");
         if( split != NULL ) {
            strcat( tmp, split );
         }
      }
   } else {
      if( ! isDirExists( dir_name ) ) {
         if ( mkdir(dir_name,0755 ) == -1 ) {
                   fprintf (stderr,"ERROR: %s\n", strerror(errno) );
                   return(EXIT_FAILURE);
         }
      }
   }
   free( work_string );
   return(0);
}

/**
 * Name        : globPath
 * Author      : R.Lahlou , cmoi 2012
 * Description : Wrapper to glob :
 *               find pathnames matching a pattern  
 */
int globPath (char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno) )
{
    glob_t glob_p;
    int ret;

    /* The real glob */
    ret = glob(pattern, GLOB_NOSORT,  0 , &glob_p);
    switch (ret) {
        case GLOB_NOSPACE:
	             fprintf(stderr,"globPath: Glob running out of memory \n");
		     break;
	case GLOB_ABORTED:
	             fprintf(stderr,"globPath: Glob read error\n");
		     break;
	case GLOB_NOMATCH:
                globfree(&glob_p);
		     return(0);
		     break;/* not reached */
    }

    ret=glob_p.gl_pathc;
    globfree(&glob_p);


    return (ret);
}

/**
 * copied from nodelogger_svr
 */
char *getPathBase (const char *full_path) {
  char *split=NULL;
  char *work_string=NULL;
  char *chreturn =NULL;

  work_string = strdup(full_path);
  split = strrchr (work_string,'/');
  if (split != NULL) {
    *split = '\0';
    chreturn = strdup (work_string);
  }
  free( work_string );
  work_string=NULL;
  return chreturn;
}

/**
 * Name        : NodeLogr
 * Author      : R.Lahlou , cmoi 2012
 * Description :  
 */
int NodeLogr (char *nodeLogerBuffer , int pid)
{
     int NodeLogfile;
     int bwrite, num=0,ret;
     char user[10];
     char firsin[512],Stime[40],Etime[40];
     char logBuffer[1024];

     if ( nodeLogerBuffer == NULL ) {
            fprintf(stderr,"NodeLogr: arg. nodeLogerBuffer is NULL \n");
            return (1);
     }

     memset(firsin,'\0',sizeof(firsin));
     if ( (num=sscanf(nodeLogerBuffer,"%[^:]:%[^:]:%[^\n]",user,firsin,logBuffer)) != 3 ) {
             fprintf(stderr,"NodeLogr: Error with the format of nodeLogerBuffer\n");
	     return (1);
     }
     
     /* test existence of Exp. and datestamp  */
     if ( access(firsin,R_OK) != 0 ) {
             fprintf(stderr,"NodeLogr: Experiment:%s do not exists\n",firsin);
	     return (1);
     }

     strcat(logBuffer,"\n");
     if ((NodeLogfile = open(firsin, O_WRONLY|O_APPEND|O_CREAT, 00666)) != -1 ) {
           bwrite = write(NodeLogfile,logBuffer , strlen(logBuffer));
	   fsync(NodeLogfile);
           close(NodeLogfile);
	   ret=0;
     } else {
           fprintf(stderr,"NodeLogr: Could not Open nodelog file for Experiment:%s pid=%d logBuffer:%s\n",firsin,pid,logBuffer);
           ret=1; 
     }


     return(ret);
}

/**
 * Name        : writeNodeWaitedFile
 * Author      : R.lahlou , cmoi 2012
 * Description : write the node waited file under dependent-ON Xp. 
 * Return value: 
 */
int  writeNodeWaitedFile ( const char * string ) 
{
    FILE *waitingFile;
    char tmp_line[1024];
    char line[1024];
    char statusFile[1024],waitfile[1024],user[250],exp[250],node[256],datestamp[25],loopArgs[128];
    int n,found=0;

    memset(tmp_line,'\0',sizeof(tmp_line));
    memset(line,'\0',sizeof(tmp_line));
    memset(statusFile,'\0',sizeof(statusFile));
    memset(waitfile,'\0',sizeof(waitfile));
    memset(user,'\0',sizeof(user));
    memset(exp,'\0',sizeof(exp));
    memset(node,'\0',sizeof(node));
    memset(datestamp,'\0',sizeof(datestamp));
    memset(loopArgs,'\0',sizeof(loopArgs));
   
    n=sscanf(string,"sfile=%s wfile=%s user=%s exp=%s node=%s datestamp=%s args=%s",statusFile,waitfile,user,exp,node,datestamp,loopArgs);
    
    if ( (n <= 5 ) || ( n == 6 && strlen(loopArgs) != 0) ) {
        fprintf(stderr,"wrong number of argument given by sscanf for writeNodeWaitFile:%d should be 7\n",n);
	return(1);
    }
   
    /* Test status File to see wether the task has ended while we were writing node file. If yes 
     * do not write wait file and return status that should should submit task (no wait) and erase
     * in maestro waiting file.
     * Remember that we are now holding the Mutex of the Xperiment we are depending on ... 
     NO NEED!!! 
    if ( access(statusFile,R_OK) == 0 ) {
                fprintf(stderr,"writeNodeWaitedFile(mserver) found status file:%s\n",waitfile );
                return (9);
    }
    */

    /* if ((waitingFile=open(waitfile,O_WRONLY|O_APPEND|O_CREAT, 00666)) == -1 ) { */
    if ((waitingFile=fopen(waitfile,"a")) == NULL ) {
                fprintf(stderr,"writeNodeWaitedFile(mserver) cannot open file:%s for appending \n",waitfile );
		return(1);
    }
  
    sprintf( tmp_line, "user=%s exp=%s node=%s datestamp=%s args=%s\n",user,exp,node,datestamp,loopArgs );
    while( fgets(line, 1024, waitingFile) != NULL ) {
           if( strcmp( line, tmp_line ) == 0 ) {
              found = 1;
              break;
           }
    }
     
    /* if ( !found ) n = write(waitingFile,tmp_line , strlen(tmp_line)); */

    if ( !found ) fprintf( waitingFile,"%s", tmp_line ); 
    fclose( waitingFile );

    
    return(0);

}

/**
 *
 *
 *
 */
int writeInterUserDepFile (const char * tbuffer )
{
     FILE * fp=NULL;
     char buff[1024];
     char DepFileName[1024];
     char filename[1024],DepBuf[2048],ppwdir[1024],mversion[128],md5sum[128],datestamp[128]; 
     char *token, *saveptr1;
     char *tmpString; 
     const char delimiter[] = "#";
     struct stat st;
     char mode[] = "0444";
     char wbuffer[2048];
     char junk[2048];
     char csize[5];
     int n,to_read=0, total=0,received=0;

     int r,i;
     int size;

     tmpString=strdup(tbuffer); 
     /* get the first token */
     token = strtok_r(tmpString, delimiter, &saveptr1);
     free(tmpString); 
	   
     /* walk through other tokens */
     while( token != NULL ) 
     {
       if ( strncmp(token,"fil",3) == 0 ) {
            sprintf(filename,"%s",&token[4]);
       } else if ( strncmp(token,"dbf",3) == 0 ) {
            sprintf(DepBuf,"%s",&token[4]);
       } else if ( strncmp(token,"pwd",3) == 0 ) {
            sprintf(ppwdir,"%s",&token[4]);
       } else if ( strncmp(token,"mve",3) == 0 ) {
            sprintf(mversion,"%s",&token[4]);
       } else if ( strncmp(token,"m5s",3) == 0 ) {
            sprintf(md5sum,"%s",&token[4]);
       } else if ( strncmp(token,"dst",3) == 0 ) {
            sprintf(datestamp,"%s",&token[4]);
       } else {
             fprintf(stderr,"Inrecognized string:%s\n",token);
       }
         token = strtok_r(NULL, delimiter, &saveptr1);
     }

     if ( (r=touch(filename)) != 0 ) {
               fprintf(stderr,"maestro server cannot create interUser dependency file:%s\n",filename );
	       return(1);
     }

     /* in case of ocm dep. and depending on  a loop the file exist already 
      * from last iteration */
     if ((fp=fopen(filename,"w")) == NULL) {
               fprintf(stderr,"maestro server cannot write to interUser dependency file:%s\n",filename );
	       return(1);
     }

     fwrite(DepBuf, 1, strlen(DepBuf) , fp);
     fclose(fp);

     if ( stat(filename,&st) != 0 ) {
               fprintf(stderr,"maestro server cannot stat interUser dependency file:%s\n",filename );
               free(tmpString); 
	       return(1);
     } else fprintf(stderr,"size of InterUserDepFile is :%ld\n",(long)st.st_size);

     /* create server dependency directory (based on maestro version) 
     * Note: multiple client from diff. experiment could try to create this when do not exist. Give a static inode for this action */
     snprintf(buff, sizeof(buff), "%s/.suites/maestrod/dependencies/polling/v%s",ppwdir,mversion);

     if ( access(buff,R_OK) != 0 ) {
          if ( r_mkdir ( buff , 1 ) != 0 ) {
                  fprintf(stderr,"Could not create dependency directory:%s\n",buff);
	          return(1);
          }
     }

     /* build dependency filename and link it to true dependency file under the xp. tree*/
     snprintf(DepFileName,sizeof(DepFileName),"%s/.suites/maestrod/dependencies/polling/v%s/%s_%s",ppwdir,mversion,datestamp,md5sum);

     /* have to check for re-runs  */
     r=unlink(DepFileName);
     if ( (r=symlink(filename,DepFileName)) != 0 ) {
             fprintf(stderr,"writeInterUserDepFile: symlink returned with error:%d\n",r );
     }
     
     return(0);
}


/**
 * Wrapper to nanosleep which is thread safe
 */
int _sleep (double sleep_time)
{
  struct timespec tv;
    /* Construct the timespec from the number of whole seconds...  */
    tv.tv_sec = (time_t) sleep_time;
    /* ... and the remainder in nanoseconds.  */
    tv.tv_nsec = (long) ((sleep_time - tv.tv_sec) * 1e+9);

    while (1)
    {
      /* Sleep for the time specified in tv.  If interrupted by a
         signal, place the remaining time left to sleep back into tv.  */
         int rval = nanosleep (&tv, &tv);
         if (rval == 0)
                /* Completed the entire sleep time; all done.  */
                return 0;
         else if (errno == EINTR)
                /* Interruped by a signal.  Try again.  */
                continue;
         else 
                /* Some other error; bail out.  */
                return rval;
    }

    return 0;
}

/**
 * HTML source for the start of the process listing page.  
 */
char* page_start_dep = 
  "<html>\n"
  " <body>\n"
  " <meta http-equiv=\"refresh\" content=\"10\">\n"
  "  <center><b>Dependencies</b></center>\n"
  "  <table cellpadding=\"4\" cellspacing=\"0\" border=\"1\" width=\"100%\" bgcolor=\"#E6E6FA\">\n"
  "   <thead>\n"
  "    <tr>\n"
  "     <th>Date Registered</th>\n"
  "     <th>Experiment Parameters</th>\n"
  "    </tr>\n"
  "   </thead>\n"
  "   <tbody>\n";

/**
 * HTML source for the end of the process listing page. 
 */
char* page_end_dep =
  "   </tbody>\n"
  "  </table>\n"
  " </body>\n"
  "</html>\n";


char* page_start_blocked = 
  "  <table cellpadding=\"4\" cellspacing=\"0\" border=\"1\" width=\"100%\" bgcolor=\"#E6E6FF\">\n"
  "  <thead>\n"
  "   <tr>\n"
  "    <th>Host</th>\n"
  "    <th>User</th>\n"
  "    <th>Logging Time</th>\n"
  "    <th>Experiment</th>\n"
  "    <th>Node</th>\n"
  "    <th>Signal</th>\n"
  "   </tr>\n"
  "  </thead>\n"
  "  <tbody>\n";

char* page_end_blocked =
  "   </tbody>\n"
  "   </table>\n";


/**
 * parse xml configuration file using libroxml
 */
int ParseXmlConfigFile(char *filename ,  _l2d2server *pl2d2 )
{

      FILE *doc=NULL;

      char bf[250];
      char buffer[2048];
      char *c;
      int size,status;


      /* default */
      memset(bf, '\0' , sizeof(bf));
      memset(buffer, '\0' , sizeof(buffer));

      if  ( (doc=fopen(filename, "r")) == NULL ) {
               fprintf(stderr,"Cannot Open XML Configuration file for mserver:%s ... Setting Defaults\n",filename);

	       sprintf(buffer,"%s/.suites/log/v%s",getenv("HOME"),pl2d2->mversion);
	       status=r_mkdir(buffer , 1);

	       sprintf(buffer,"%s/.suites/log/v%s/mlog",getenv("HOME"),pl2d2->mversion);
	       strcpy(pl2d2->mlog,buffer);
	       sprintf(buffer,"%s/.suites/log/v%s/mlogerr",getenv("HOME"),pl2d2->mversion);
	       strcpy(pl2d2->mlogerr,buffer);
	       
	       sprintf(buffer,"%s/.suites/log/v%s/dmlog",getenv("HOME"),pl2d2->mversion);
	       strcpy(pl2d2->dmlog,buffer);
	       sprintf(buffer,"%s/.suites/log/v%s/dmlogerr",getenv("HOME"),pl2d2->mversion);
	       strcpy(pl2d2->dmlogerr,buffer);

	       sprintf(buffer,"%s/public_html",getenv("HOME"));
	       if ( access(buffer,R_OK) != 0 ) {
	              status=mkdir(buffer , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
               }
	       sprintf(buffer,"%s/public_html/dependencies_stat_v%s.html",getenv("HOME"),pl2d2->mversion);
	       strcpy(pl2d2->web_dep,buffer);

	       sprintf(buffer,"%s/public_html/blocked_clients_v%s.html",getenv("HOME"),pl2d2->mversion);
	       strcpy(pl2d2->web_blk,buffer);
	      
	       pl2d2->numProcessAtstart=5;
	       pl2d2->pollfreq=30;
	       pl2d2->dependencyTimeOut=24;
	       return(0);
      }

      if (doc) {
          fread(buffer,2048,1,doc);
	  fclose(doc);
      }

      node_t *root = roxml_load_buf(buffer);
      node_t *item = roxml_get_chld(root,NULL,0);
      char *r_txt  = roxml_get_name(item,bf,sizeof(bf));

      if ( strcmp(bf,"mserver") == 0 ) {
             node_t *log_n = roxml_get_chld(item,"log",0);
             if ( log_n != NULL ) { 
                   node_t *log_txt = roxml_get_txt(log_n,0);
                   if ( (c=roxml_get_content(log_txt,bf,sizeof(bf),&size)) != NULL ) {
	                    sprintf(pl2d2->logdir,"%s/v%s",bf,pl2d2->mversion);
	                    status=r_mkdir(pl2d2->logdir , 1);
	                    snprintf(pl2d2->mlog,sizeof(pl2d2->mlog),"%s/mlog",pl2d2->logdir);
	                    snprintf(pl2d2->mlogerr,sizeof(pl2d2->mlogerr),"%s/mlogerr",pl2d2->logdir);
			    /* set files */
			    snprintf(pl2d2->dmlog,sizeof(pl2d2->dmlog),"%s/dmlog",pl2d2->logdir);
	                    snprintf(pl2d2->dmlogerr,sizeof(pl2d2->dmlogerr),"%s/dmlogerr",pl2d2->logdir);
			    fprintf(stdout,"In Xml Config File found log directory=%s\n",pl2d2->logdir);
                   } else {
	                    sprintf(pl2d2->logdir,"%s/.suites/log/v%s",getenv("HOME"),pl2d2->mversion);
	                    status=r_mkdir(pl2d2->logdir , 1);
			    /* set files */
	                    sprintf(pl2d2->mlog,"%s/mlog",pl2d2->logdir);
	                    sprintf(pl2d2->mlogerr,"%s/mlogerr",pl2d2->logdir);
	                    
			    sprintf(pl2d2->dmlog,"%s/dmlog",pl2d2->logdir);
	                    sprintf(pl2d2->dmlogerr,"%s/dmlogerr",pl2d2->logdir);
                            fprintf(stderr,"Setting Defaults for log directory:%s\n",pl2d2->logdir);
                          }
             }
             node_t *web_n = roxml_get_chld(item,"web",0);
	     if ( web_n != NULL ) {
                   node_t *web_txt = roxml_get_txt(web_n,0);
                   if ( (c=roxml_get_content(web_txt,bf,sizeof(bf),&size)) != NULL ) {
	                    sprintf(pl2d2->web,"%s/v%s",bf,pl2d2->mversion);
	                    status=r_mkdir(pl2d2->web , 1);
	                    sprintf(pl2d2->web_dep,"%s/dependencies.html",pl2d2->web);
	                    sprintf(pl2d2->web_blk,"%s/blocked_clients.html",pl2d2->web);
			    fprintf(stdout,"In Xml Config File found web directory=%s\n",pl2d2->web);
                   } else {
	                    sprintf(pl2d2->web,"%s/public_html/v%s",getenv("HOME"),pl2d2->mversion);
	                    status=r_mkdir(pl2d2->web , 1);
	                    sprintf(pl2d2->web_dep,"%s/dependencies_stat_v%s.html",pl2d2->web,pl2d2->mversion);
	                    sprintf(pl2d2->web_blk,"%s/blocked_clients_v%s.html",pl2d2->web,pl2d2->mversion);
                            fprintf(stderr,"Setting Defaults for web path:%s\n",pl2d2->web);
                          }
             }
             node_t *prc_n = roxml_get_chld(item,"numProcessAtstart",0);
	     if ( prc_n != NULL ) {
                   node_t *prc_txt = roxml_get_txt(prc_n,0);
                   if ( (c=roxml_get_content(prc_txt,bf,sizeof(bf),&size)) != NULL ) {
	                    if ( (pl2d2->numProcessAtstart=atoi(bf)) <= 1 ) {
			              pl2d2->numProcessAtstart=1;
			              fprintf(stdout,"Forcing numProcessAtstart=%d\n",pl2d2->numProcessAtstart);
                            } else if ( (pl2d2->numProcessAtstart=atoi(bf)) > 10 ) {
			              pl2d2->numProcessAtstart=10;
			              fprintf(stdout,"Forcing numProcessAtstart=%d\n",pl2d2->numProcessAtstart);
                            } else {
			         fprintf(stdout,"In Xml Config File found numProcessAtstart=%d\n",pl2d2->numProcessAtstart);
                            }
                   } else {
		            pl2d2->numProcessAtstart=5;
                            fprintf(stderr,"Setting Defaults for numProcessAtstart:%d\n",pl2d2->numProcessAtstart);
                          }
             }

             node_t *param_n = roxml_get_chld(item,"dparams",0);

	     if ( param_n != NULL ) {
                   node_t *pfq = roxml_get_attr(param_n, "poll-freq",0);
                   if ( (c=roxml_get_content(pfq,bf,sizeof(bf),&size)) != NULL ) {
	                    if ( (pl2d2->pollfreq=atoi(bf)) <= 15 || (pl2d2->pollfreq=atoi(bf)) > 120 ) { 
			           pl2d2->pollfreq=30;
			           fprintf(stdout,"Forcing polling frequency=%d\n",pl2d2->pollfreq);
                            } else {
			           pl2d2->pollfreq=atoi(bf);
			           fprintf(stdout,"In Xml Config File found polling frequency=%d\n",pl2d2->pollfreq);
                            } 
                   } else {
	                    pl2d2->pollfreq=30;
                            fprintf(stderr,"Setting Defaults for polling frequency\n",pl2d2->pollfreq);
		   }
             
		   node_t *dto = roxml_get_attr(param_n, "dependencyTimeOut",0);
	           if ( dto != NULL ) {
                          if ( (c=roxml_get_content(dto,bf,sizeof(bf),&size)) != NULL ) {
	                          if (  (pl2d2->dependencyTimeOut=atoi(bf)) <= 0 || (pl2d2->dependencyTimeOut=atoi(bf)) > 48 ) {
	                                 pl2d2->dependencyTimeOut=24;
			                 fprintf(stdout,"Forcing dependencyTimeOut=%d\n",pl2d2->dependencyTimeOut);
                                  } else {
	                                 pl2d2->dependencyTimeOut=atoi(bf);
                                         fprintf(stderr,"In Xml Config file found dependency time out periode\n",pl2d2->dependencyTimeOut);
				  }
                          } else {
	                          pl2d2->dependencyTimeOut=24;
                                  fprintf(stderr,"Setting Defaults for dependency time out periode\n",pl2d2->dependencyTimeOut);
			  }
                   }
	     }
             node_t *dbz_n = roxml_get_chld(item,"debug_zone",0);
	     if ( dbz_n != NULL ) {
                    node_t *dbz_txt = roxml_get_txt(dbz_n,0);
                    if ( (c=roxml_get_content(dbz_txt,bf,sizeof(bf),&size)) != NULL ) {
		            if ( (pl2d2->dzone=atoi(bf)) <= 0 ) {
		                  pl2d2->dzone=1;
                            } else {
			           pl2d2->dzone=atoi(bf);
			           fprintf(stdout,"In Xml Config File found dzone=%d\n",pl2d2->dzone);
                            }
                    } else {
		            pl2d2->dzone=1;
                            fprintf(stderr,"Setting Defaults for debuging Zone\n",pl2d2->dzone);
		    }
             }
      } else {
             fprintf(stderr,"Incorrect root node name in xml config file:%s ... Setting Defaults \n",filename);
	     pl2d2->numProcessAtstart=5;
	     pl2d2->pollfreq=30;
	     pl2d2->dependencyTimeOut=120;
             pl2d2->dzone=1;

	     sprintf(pl2d2->web,"%s/public_html/v%s",getenv("HOME"),pl2d2->mversion);
	     status=r_mkdir(pl2d2->web , 1);

	     sprintf(pl2d2->web_dep,"%s/dependencies.html",pl2d2->web);
	     sprintf(pl2d2->web_blk,"%s/blocked_clients.html",pl2d2->web);
	     sprintf(pl2d2->logdir,"%s/.suites/log/v%s",getenv("HOME"),pl2d2->mversion);
	     status=r_mkdir(pl2d2->logdir , 1);
	     sprintf(pl2d2->mlog,"%s/mlog",pl2d2->logdir);
	     sprintf(pl2d2->mlogerr,"%s/mlogerr",pl2d2->logdir);
	     sprintf(pl2d2->dmlog,"%s/dmlog",pl2d2->logdir);
	     sprintf(pl2d2->dmlogerr,"%s/dmlogerr",pl2d2->logdir);
      }

      roxml_release(RELEASE_ALL);
      roxml_close(root);

      return (0);
}
/**
 * parse dependency file (polling for the moment )
 */
struct _depParameters * ParseXmlDepFile(char *filename )
{

      FILE *doc=NULL;
      node_t *dep, *name, *inode, *lock , *sub;
      struct _depParameters *listParam=NULL;
      char bf[250];
      char bfl[1024];
      char buffer[2048];
      char tmpbf[2048];
      char *c;
      int size,i;

      memset(bf, '\0' , sizeof(bf));
      memset(buffer, '\0' , sizeof(buffer));

      if  ( (doc=fopen(filename, "r")) == NULL ) {
               fprintf(stderr,"ParseXmlDepFile: Cannot Open XML Polling dependency file:%s \n",filename);
	       return(NULL);
      }

      if (doc) {
          fread(tmpbf,2048,1,doc);
	  fflush(doc);
	  snprintf(buffer,sizeof(buffer),"%s",tmpbf);
	  fclose(doc);
      }

      node_t *root = roxml_load_buf(buffer);
      node_t *item = roxml_get_chld(root,NULL,0);
      char *r_txt  = roxml_get_name(item,bf,sizeof(bf));

      node_t *type = roxml_get_attr(item, "type",0);
      c=roxml_get_content(type,bf,sizeof(bf),&size);

      if ( strcmp(bf,"pol") != 0 ) {
             fprintf(stderr,"ParseXmlDepFile: Incorrect root node name in xml polling dependency file:%s\n",filename);
	     return (NULL);
      } else if  ( (listParam=(struct _depParameters *) malloc(sizeof(struct _depParameters)))  == NULL ) {
	          fprintf(stderr,"ParseXmlDepFile: Cannot malloc on heap inside ParseXmlDepFile ... exiting \n");
		  exit(1);
      }
		    
      node_t *xp_n = roxml_get_chld(item,"xp",0);
      node_t *xp_txt = roxml_get_txt(xp_n,0);
      if ( xp_n != NULL && xp_txt != NULL ) {
                  c=roxml_get_content(xp_txt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_name,bf);
      } else {
                  strcpy(listParam->xpd_name,"");
      }

      node_t *xp_node = roxml_get_chld(item,"node",0);
      node_t *xp_nodetxt = roxml_get_txt(xp_node,0);
      if ( xp_node != NULL && xp_nodetxt != NULL ) {
                  c=roxml_get_content(xp_nodetxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_node,bf);
      } else {
                  strcpy(listParam->xpd_node,"");
      }
                    
      node_t *xp_indx = roxml_get_chld(item,"indx",0);
      node_t *xp_indxtxt = roxml_get_txt(xp_indx,0);
      if ( xp_indx != NULL && xp_indxtxt != NULL ) {
                  c=roxml_get_content(xp_indxtxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_indx,bf);
      } else {
                  strcpy(listParam->xpd_indx,"");
      }
      node_t *xp_xdate = roxml_get_chld(item,"xdate",0);
      node_t *xp_xdatetxt = roxml_get_txt(xp_xdate,0);
      if ( xp_xdate != NULL && xp_xdatetxt != NULL ) {
                  c=roxml_get_content(xp_xdatetxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_xpdate,bf);
      } else {
                  strcpy(listParam->xpd_xpdate,"");
      }
      node_t *xp_status = roxml_get_chld(item,"status",0);
      node_t *xp_statustxt = roxml_get_txt(xp_status,0);
      if ( xp_status != NULL && xp_statustxt != NULL ) {
                  c=roxml_get_content(xp_statustxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_status,bf);
      } else {
                  strcpy(listParam->xpd_status,"");
      }
      node_t *xp_largs = roxml_get_chld(item,"largs",0);
      node_t *xp_largstxt = roxml_get_txt(xp_largs,0);
      if ( xp_largs != NULL && xp_largstxt != NULL ) {
                  c=roxml_get_content(xp_largstxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_largs,bf);
      } else {
                  strcpy(listParam->xpd_largs,"");
      }

      node_t *xp_susr = roxml_get_chld(item,"susr",0);
      node_t *xp_susrtxt = roxml_get_txt(xp_susr,0);
      if ( xp_susr != NULL && xp_susrtxt != NULL ) {
                  c=roxml_get_content(xp_susrtxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_susr,bf);
      } else {
                  strcpy(listParam->xpd_susr,"");
      }
             
      node_t *xp_sxp = roxml_get_chld(item,"sxp",0);
      node_t *xp_sxptxt = roxml_get_txt(xp_sxp,0);
      if ( xp_sxp != NULL && xp_sxptxt != NULL ) {
                  c=roxml_get_content(xp_sxptxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_sname,bf);
      } else {
                  strcpy(listParam->xpd_sname,"");
      }

      node_t *xp_snode = roxml_get_chld(item,"snode",0);
      node_t *xp_snodetxt = roxml_get_txt(xp_snode,0);
      if ( xp_snode != NULL && xp_snodetxt != NULL ) {
                  c=roxml_get_content(xp_snodetxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_snode,bf);
      } else {
                  strcpy(listParam->xpd_snode,"");
      }
	     
      node_t *xp_sxdate = roxml_get_chld(item,"sxdate",0);
      node_t *xp_sxdatetxt = roxml_get_txt(xp_sxdate,0);
      if ( xp_sxdate != NULL && xp_sxdatetxt != NULL ) {
                  c=roxml_get_content(xp_sxdatetxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_sxpdate,bf);
      } else {
                  strcpy(listParam->xpd_sxpdate,"");
      }

      node_t *xp_slargs = roxml_get_chld(item,"slargs",0);
      node_t *xp_slargstxt = roxml_get_txt(xp_slargs,0);
      if ( xp_slargs != NULL && xp_slargstxt != NULL ) {
                  c=roxml_get_content(xp_slargstxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_slargs,bf);
      } else {
                  strcpy(listParam->xpd_slargs,"");
      }
	     
      node_t *xp_lockfile = roxml_get_chld(item,"lock",0);
      node_t *xp_lockfiletxt = roxml_get_txt(xp_lockfile,0);
      if ( xp_lockfile != NULL && xp_lockfiletxt != NULL ) {
                  c=roxml_get_content(xp_lockfiletxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_lock,bf);
      } else {
                  strcpy(listParam->xpd_lock,"");
      }

      node_t *xp_ndfile = roxml_get_chld(item,"depfilename",0);
      node_t *xp_ndfiletxt = roxml_get_txt(xp_ndfile,0);
      if ( xp_ndfile != NULL && xp_ndfiletxt != NULL ) {
                  c=roxml_get_content(xp_ndfiletxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_dfilename,bf);
      } else {
                  strcpy(listParam->xpd_dfilename,"");
      }

      node_t *xp_cnt = roxml_get_chld(item,"container",0);
      node_t *xp_cnttxt = roxml_get_txt(xp_cnt,0);
      if ( xp_cnt != NULL && xp_cnttxt != NULL ) {
                  c=roxml_get_content(xp_cnttxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_container,bf);
      } else {
                  strcpy(listParam->xpd_container,"");
      }

      node_t *xp_mversion = roxml_get_chld(item,"mversion",0);
      node_t *xp_mversiontxt = roxml_get_txt(xp_mversion,0);
      if ( xp_mversion != NULL && xp_mversiontxt != NULL ) {
                  c=roxml_get_content(xp_mversiontxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_mversion,bf);
      } else {
                  strcpy(listParam->xpd_mversion,"");
      }
	     
      node_t *xp_mdomain = roxml_get_chld(item,"mdomain",0);
      node_t *xp_mdomaintxt = roxml_get_txt(xp_mdomain,0);
      if ( xp_mdomain != NULL && xp_mdomaintxt != NULL ) {
                  c=roxml_get_content(xp_mdomaintxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_mdomain,bf);
      } else {
                  strcpy(listParam->xpd_mdomain,"");
      }
	     
      node_t *xp_key = roxml_get_chld(item,"key",0);
      node_t *xp_keytxt = roxml_get_txt(xp_key,0);
      if ( xp_key != NULL && xp_keytxt != NULL ) {
                  c=roxml_get_content(xp_keytxt,bf,sizeof(bf),&size);
                  strcpy(listParam->xpd_key,bf);
      } else {
                  strcpy(listParam->xpd_key,"");
      }

      node_t *xp_regtime = roxml_get_chld(item,"regtime",0);
      if ( xp_regtime != NULL ) {
                   node_t *xp_regtimedate = roxml_get_attr(xp_regtime,"date",0);
		   if ( xp_regtimedate != NULL ) {
                             c=roxml_get_content(xp_regtimedate,bf,sizeof(bf),&size);
                             strcpy(listParam->xpd_regtimedate,bf);
                   } else strcpy(listParam->xpd_regtimedate,"");
                   
		   node_t *xp_regtimepoch = roxml_get_attr(xp_regtime,"epoch",0);
		   if ( xp_regtimepoch != NULL ) {
                             c=roxml_get_content(xp_regtimepoch,bf,sizeof(bf),&size);
                             strcpy(listParam->xpd_regtimepoch,bf);
                   } else strcpy(listParam->xpd_regtimepoch,"");
	           /* fprintf(stderr,"xpd_regtimedate=%s  xpd_regtimepoch=%s\n",listParam->xpd_regtimedate,listParam->xpd_regtimepoch);  */
     } else {
            fprintf(stderr,"regtime null\n");
            strcpy(listParam->xpd_regtimedate,"");
            strcpy(listParam->xpd_regtimepoch,"");
     }
	     

     roxml_release(RELEASE_ALL);
     roxml_close(root);

     return (listParam);
}
/**
*  function : pdir
*             walk through directory
*/
static void pdir (const char * dir_name)
{
    DIR * d;
    struct dirent * entry;
    struct stat st;
    const char * d_name;
    int path_length;
    char path[PATH_MAX]; /* limits.h defines "PATH_MAX". */

    /* Open the directory specified by "dir_name". */
    d = opendir (dir_name);

    /* Check it was opened. */
    if (! d) {
        fprintf (stderr, "Cannot open directory '%s': %s\n", dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }

    while (1) {
        /* "Readdir" gets subsequent entries from "d". */
        entry = readdir (d);
        if (! entry) {
            /* There are no more entries in this directory, so break
               out of the while loop. */
            break;
        }
        d_name = entry->d_name;
	if ( stat(d_name,&st) != 0 ) {
	      fprintf(stderr,"Cannot stat on :%s\n",d_name);
	      continue;
        }

        /* See if "entry" is a subdirectory of "d". */
        if ( typeofFile(st.st_mode) == 'd' ) {
            /* Check that the directory is not "d" or d's parent. */
            if (strcmp (d_name, "..") != 0 && strcmp (d_name, ".") != 0) {
                path_length = snprintf(path, PATH_MAX, "%s/%s", dir_name, d_name);
                if (path_length >= PATH_MAX) {
                         fprintf (stderr, "Path length has got too long.\n");
                         exit (EXIT_FAILURE);
                }
		/* do work  */

                /* Recursively call "pdir" with the new path. */
                pdir (path);
            }
        }
    }
    /* After going through all the entries, close the directory. */
    if (closedir (d)) {
            fprintf (stderr, "Could not close '%s': %s\n", dir_name, strerror (errno));
            exit (EXIT_FAILURE);
    }
}

/** 
*  logZone : log to stdout/err
*  this need synchro. btw all the processes
*  Note : errors are logged what ever the Zone is 
*/
void logZone(int this_Zone, int conf_Zone, int console , char * txt, ...)
{
      va_list za;

      if ( this_Zone == conf_Zone ) {
             va_start( za, txt );
             switch (console)
             {
                 case CONSOLE_OUT:
                                  vfprintf(stdout,txt,za);
                                  break;
                 case CONSOLE_ERR:
                                  vfprintf(stderr,txt,za);
                                  break;
             }
             va_end(za);
      } else if ( console == CONSOLE_ERR ) {
                 vfprintf(stderr,txt,za);
      }
}
/**
*   Function : typeofFile
*   object   : return a code corresponding to the type
*              of the file
*/
char typeofFile(mode_t mode)
{
    switch(mode & S_IFMT) {
        case S_IFREG:
        return('r');
        case S_IFDIR:
           return('d');
       case S_IFCHR:
           return('c');
       case S_IFBLK:
          return('b');
       case S_IFLNK:
          return('l');
       case S_IFIFO:
          return('p');
       case S_IFSOCK:
          return('s');
    }
  return ('-');
}
/**
 * SendFile
 * Download waited_end file to host TMPDIR 
 *
 *
 */
int SendFile_previous (const char * filename , int sock ) 
{
    char buff[8192];
    char kris[8192];
    char fsize[11];
    FILE * waitf;
    int size;
    int bytes_written=0, bytes_read=0, bytes_left=0, total=0;
    
    struct stat st;

    /* get & format size of file in bytes */ 
    if ( stat(filename,&st) != 0 ) {
          fprintf(stderr,"SendFile:mserver cannot stat waitfile:%s\n", filename );
          return(1);
    }

    /* size = st.st_size; */
    snprintf(fsize,sizeof(fsize),"%10ld",(long)st.st_size);

    if ((waitf=fopen(filename,"r")) != NULL ) { 
	  fread(kris,sizeof(kris),1,waitf);
    } else {
          fprintf(stderr,"SendFile:mserver cannot read waited_end file:%s\n", filename );
          return(1);
    }
    fclose(waitf);


    sprintf(buff,"%s %s",fsize,kris); 

    /* One sending bytes_written = write (sock , buff, sizeof(buff)); */
   
    bytes_left = strlen(buff);

    while ( total < strlen(buff) ) 
    {
        bytes_written = send(sock, buff+total, bytes_left, 0);

	if ( bytes_written == -1 ) break;
	total += bytes_written;
	bytes_left -= bytes_written;
    }

    return (bytes_written == -1)  ? 1 : 0;
}
int SendFile (const char * filename , int sock ) 
{
    char buff[1024];
    char kris[1024];
    FILE * waitf;
    int size;
    int bytes_written=0, bytes_read=0, bytes_left=0, total=0;
    
    struct stat st;

    /* get & format size of file in bytes */ 
    if ( stat(filename,&st) != 0 ) {
          fprintf(stderr,"SendFile:mserver cannot stat waitfile:%s\n", filename );
          return(1);
    }

    memset(kris,'\0',sizeof(kris));
    if ((waitf=fopen(filename,"r")) != NULL ) { 
	  fread(kris,sizeof(kris),1,waitf);
    } else {
          fprintf(stderr,"SendFile:mserver cannot read waited_end file:%s\n", filename );
          return(1);
    }
    fclose(waitf);


    bytes_written = write (sock , kris, sizeof(kris)); 
  
    return (bytes_written == -1)  ? 1 : 0;
}
/**
 * Obtain a lock on a file , try a limited time if no success
 * return 0 if lock obtained 1 if not 
 * also check if symlink is old by x sec remove it
 */
int lock ( char *md5Token , _l2d2server L2D2 , char *xpn , char *node ) 
{
   int i, ret;
   char *base,*leaf;
   char src[1024],dest[1024],Ltime[25];
   struct stat st;
   time_t now;
   double diff_t;
   
   sprintf(src,"%s/end_task_lock",L2D2.tmpdir);
   if ( access(src,R_OK) != 0 ) { 
           if ( (ret=touch(src)) != 0 ) fprintf(stderr,"cannot Touch file: lock on Tmpdir Xp=%s Node=%s\n",xpn,node);
   }
  
   sprintf(dest,"%s/%s",L2D2.tmpdir,md5Token);

   for ( i=0 ; i < 5 ; i++ ) {
        get_time(Ltime,3);
        ret=symlink("end_task_lock",dest);
        if ( ret == 0 )  {
	       /* fprintf(stdout,"symlink obtained loop=%d AT:%s xpn=%s node=%s Token:%s\n",i,Ltime,xpn,node,md5Token); */
	       break;
        }
	_sleep(0.5);
   }
 
   if ( ret != 0 ) {
        if ( (stat(dest,&st)) == 0 ) {
              time(&now);
	      if ( (diff_t=difftime(now,st.st_mtime)) > LOCK_TIME_TO_LIVE ) {
	             ret=unlink(dest);
                     fprintf(stderr,"symlink timeout xpn=%s node=%s Token:%s\n",xpn,node,md5Token);
	      }
	} 
   } 

   return(ret);
}

/** 
 * remove a lock, 
 * return 0 if success 1 if not 
 */
int unlock ( char *md5Token , _l2d2server L2D2, char *xpn, char *node) 
{
   int ret;
   char src[1024],Ltime[25];
    
   sprintf(src,"%s/%s",L2D2.tmpdir,md5Token);

   get_time(Ltime,3);
   if ( access(src,R_OK) == 0 ) { 
       if ( (ret=unlink(src)) != 0 ) {
             fprintf(stderr,"unlink error:%d AT:%s xpn=%s node=%s Token:%s\n",ret,Ltime,xpn,node,md5Token);
	     return(1);
       } else {
             /* fprintf(stdout,"symlink released AT:%s xpn=%s node=%s Token:%s\n",Ltime,xpn,node,md5Token); */
       }
   } 
   

   return(0); 
}
/*
** initsem() -- inspired by W. Richard Stevens' UNIX Network
** Programming 2nd edition, volume 2, lockvsem.c, page 295.
*/
int initsem(key_t key, int nsems)  /* key from ftok() */
{
    int i;
    union semun arg;
    struct semid_ds buf;
    struct sembuf sb;
    int semid;

    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);

    if (semid >= 0) { /* we got it first */
        sb.sem_op = 1; sb.sem_flg = 0;
        arg.val = 1;
        for(sb.sem_num = 0; sb.sem_num < nsems; sb.sem_num++) { 
            /* do a semop() to "free" the semaphores. */
            /* this sets the sem_otime field, as needed below. */
            if (semop(semid, &sb, 1) == -1) {
                int e = errno;
                semctl(semid, 0, IPC_RMID); /* clean up */
                errno = e;
                return -1; /* error, check errno */
            }
        }

    } else if (errno == EEXIST) { /* someone else got it first */
        int ready = 0;

        semid = semget(key, nsems, 0); /* get the id */
        if (semid < 0) return semid; /* error, check errno */

        /* wait for other process to initialize the semaphore: */
        arg.buf = &buf;
        for(i = 0; i < MAX_RETRIES && !ready; i++) {
            semctl(semid, nsems-1, IPC_STAT, arg);
            if (arg.buf->sem_otime != 0) {
                ready = 1;
            } else {
                sleep(1);
            }
        }
        if (!ready) {
            errno = ETIME;
            return -1;
        }
    } else {
        return semid; /* error, check errno */
    }

    return semid;
}

/**
 *
 *
 */
int readline ( int sock , char *buffer, int buffsize, int timeout) 
{
     char *buffPos, *buffEnd;
     int nChars,readlen;
     typedef enum { false, true } bool;
     bool complete=false;
     fd_set fset;
     struct timeval tv;
     int sockStatus, readSize;
     unsigned long blockMode;

     FD_ZERO(&fset);
     FD_SET(sock,&fset);

     if ( timeout > 0 ) {
         tv.tv_sec = 0;
	 tv.tv_usec = timeout;
	 sockStatus = select(sock+1, &fset, NULL , &fset, &tv);
     } else {
	 sockStatus = select(sock+1, &fset, NULL , &fset, NULL);
     } 

     if ( sockStatus <= 0 ) {
           return sockStatus;
     }

     buffer[0] = '\0';
     buffPos = buffer;
     buffEnd = buffer + buffsize;
     readlen=0;
     while (!complete) {
         if ((buffEnd - buffPos) < 0 ) {
	     readSize = 0;
	 } else {
	     readSize = 1;
	 }
	 FD_ZERO(&fset);
	 FD_SET(sock,&fset);
	 tv.tv_sec = 5;
	 tv.tv_usec= 0;
	 sockStatus = select(sock+1, &fset, NULL , &fset, &tv);
	 if ( sockStatus < 0 ) {
	      return -1;
	 }
	 nChars = recv(sock, (char*) buffPos,readSize, MSG_NOSIGNAL);
	 readlen += nChars;
         if ( nChars <= 0 ) { 
	       return -1;
         }
	 if ( buffPos[nChars -1 ] == '\n' ) {
	     complete = true;
	     buffPos[nChars -1] = '\0';
         }
	 buffPos += nChars;
     }

     return readlen;
}
