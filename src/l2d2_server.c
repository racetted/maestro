#define _REENTRANT
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
#include <time.h>
#include <glob.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/dir.h>
#include <sys/param.h>
#include <sys/wait.h> 
#include <sys/sem.h>
#include <sys/select.h>
#include <fcntl.h>
#include <dirent.h>
#include "l2d2_Util.h"
#include "l2d2_server.h"
#include "l2d2_socket.h"

#define MAX_PROCESS 8                     /* max number of Transient workers */
#define ETERNAL_WORKER_STIMEOUT   1*60    /* 1 minute */
#define TRANSIENT_WORKER_STIMEOUT 5*60

#define SPAWNING_DELAY_TIME  5 /* seconds */

#define NOTIF_TIME_INTVAL_EW 2*60         /* heartbeat for EW */ 
#define NOTIF_TIME_INTVAL_DM 3*60

#define TRUE 1
#define FALSE 0

#define MAX_LOCK_TRY 11

/* forward functions & vars declarations */
extern void logZone(int , int , FILE *fp , char * , ...);
extern char *page_start_dep , *page_end_dep;

/* globals vars */
unsigned int pidTken = 0;
unsigned int ChildPids[MAX_PROCESS]={0};
unsigned long int epoch_diff;
double diff_t;

/* global default data for l2d2server */
_clean_times L2D2_cleantimes = {1,48,48,25,25};
_l2d2server L2D2 = {0,0,0,0,30,24,1,4,20,'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',0,0};

/* variable for Signal handlers */
volatile sig_atomic_t ProcessCount = 0; /* number of workers */
volatile sig_atomic_t sig_depend = 0;
volatile sig_atomic_t sig_admin_Terminate = 0;
volatile sig_atomic_t sig_admin_AddWorker = 0;
volatile sig_atomic_t sig_recv = 0;
volatile sig_atomic_t sig_child = 0;

/* signal handler for sesion control not used for the moment  */
static void recv_handler ( int notused ) { sig_recv = 1; }

static void child_handler(int signo, siginfo_t *siginfo, void *context)
{
     wait(0);
     sig_child=1;
}
 
/* Signal handler for Dependency Manager not used now */
static void depMang_handler(int signo, siginfo_t *siginfo, void *context ) {
    sig_depend = siginfo->si_signo;
}

static void sig_admin(int signo, siginfo_t *siginfo, void *context) {
    
    switch (signo) {
        case SIGUSR1: 
	             sig_admin_Terminate = 1; 
		     break;
        case SIGUSR2:
		     sig_admin_AddWorker = 1;
		     break;
        default:
		     break;
    }
}

/**
   Routine which run as a Process for Verifying and 
   Sumbiting dependencies. This routine is concurrency
   safe meaning that a hcron script could be set to 
   manage dependency files.
*/
void DependencyManager (_l2d2server l2d2 ) {
     
     FILE *fp,*ft,*dmlg;
     static DIR *dp=NULL;
     struct dirent *pd;
     struct stat st;
     sigset_t  newmask, oldmask,zeromask;
     struct sigaction sa;
     struct _depParameters *depXp=NULL;
     time_t current_epoch, start_epoch, start_epoch_cln;
     glob_t g_LogFiles;
     size_t cnt;
     int  g_lres;
     int datestamp,nb,LoopNumber;
     char underline[2];
     char buf[1024];
     char cmd[2048];
     char listings[1024];
     char largs[128];
     char extension[256]; /* probably a malloc here */
     char ffilename[512], filename[256], linkname[1024],LoopName[64];
     char ControllerAlive[128];
     char DependencyMAlive[128];
     char rm_key[256];
     char rm_keyFile[1024];
     char Time[40],tlog[16];
     char *pleaf=NULL;
     char **p;
     int r, ret, running=0, _ZONE_ = 2, KILL_SERVER = FALSE;
     int fd,epid; 
         
     l2d2.depProcPid=getpid();
  
     /* redirect  streams 
        Note : dup2 should handle close and open , but it does not !!*/
     close(STDIN_FILENO);
     if (fd = open("/dev/null", O_RDONLY) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
              exit (1);
        }
     }
     close(STDOUT_FILENO);
     close(STDERR_FILENO);
     if (fd = open("/dev/null", O_WRONLY) != -1) {
        if(dup2(fd, STDOUT_FILENO) < 0) {
             exit (1);
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
             exit (1);
        }
      }
      if (fd > STDERR_FILENO) {
              if(close(fd) < 0) {
                   exit (1);
              }
      }

     /* open log files */
     get_time(Time,1);
     snprintf(tlog,sizeof(tlog),"%.8s",Time);
     snprintf(buf,sizeof(buf),"%s_%.8s_%d",l2d2.dmlog,Time,l2d2.depProcPid);
     if ( (dmlg=fopen(buf,"w+")) == NULL ) {
            fprintf(stdout,"Dependency Manager: Could not open dmlog stream:%s\n",buf);
	    exit(1);
     }
   
     /* DM will check if controller is alive */
     snprintf(ControllerAlive,sizeof(ControllerAlive),"%s/CTR_%d",l2d2.tmpdir,l2d2.pid);
     
     /* streams will be unbuffered */
     setvbuf(dmlg, NULL, _IONBF, 0);
    
     sa.sa_sigaction = &depMang_handler;
     sa.sa_flags = SA_SIGINFO;
     sigemptyset(&sa.sa_mask);

     /* register signals Note: they are not used for the moment */
     if ( sigaction(SIGUSR1,&sa,NULL)  != 0 )  fprintf(dmlg,"error in sigactions  SIGUSR1\n");
     if ( sigaction(SIGUSR2,&sa,NULL)  != 0 )  fprintf(dmlg,"error in sigactions  SIGUSR2\n");
     

     get_time(Time,2);
     fprintf(dmlg,"Dependency Manager starting at:%s pid=%d\n", Time, l2d2.depProcPid);

     /* check existence of web stat files  */
     if ( access(l2d2.web_dep,R_OK) == 0 || stat(l2d2.web_dep,&st) == 0 || st.st_size > 0 ) {
    	     fp = fopen(l2d2.web_dep,"w");
    	     fwrite(page_start_dep, 1, strlen(page_start_dep) , fp);
	     fwrite(page_end_dep, 1, strlen(page_end_dep) , fp);
	     fclose(fp);
     }
    
     /* for timer */
     time(&start_epoch);
     time(&start_epoch_cln);

     /* for heartbeat */
     snprintf(DependencyMAlive,sizeof(DependencyMAlive),"%s/DM_%d",l2d2.tmpdir,getpid());
     if ( (ft=fopen(DependencyMAlive,"w+")) != NULL ) {
             fclose(ft);
     }

     while (running == 0 ) {
         sleep(l2d2.pollfreq);
	 /* get current epoch */
	 time(&current_epoch);
	 if ( (dp=opendir(l2d2.dependencyPollDir)) == NULL ) { 
	          fprintf(dmlg,"Error Could not open polling directory:%s\n",l2d2.dependencyPollDir);
		  sleep(5);
		  /* possible infinite loop here */
	          continue ; 
	 }  
	 fp = fopen(l2d2.web_dep , "w");
	 fwrite(page_start_dep, 1, strlen(page_start_dep) , fp);
         
	 while ( pd=readdir(dp))
	 {
	    memset(listings,'\0',sizeof(listings));
	    memset(linkname,'\0',sizeof(linkname));
	    memset(LoopName,'\0',sizeof(LoopName));

	    snprintf(ffilename,sizeof(ffilename),"%s/%s",l2d2.dependencyPollDir,pd->d_name);
	    snprintf(filename,sizeof(filename),"%s",pd->d_name);

            /* stat will stat the file pointed to ... lstat will stat the link itself */
	    if ( stat(ffilename,&st) != 0 ) {
	                 get_time(Time,1);
			 r=readlink(ffilename,linkname,1023);
			 linkname[r] = '\0';
	                 fprintf(dmlg,"DependencyManager():%s inter-dependency file not there, removing link at:%s ... \n",linkname,Time);
			 ret=unlink(ffilename); 
	                 continue;
	    }
	    switch ( typeofFile(st.st_mode) ) {
                case 'r'    :
		             /* skip hidden files */
		             if ( filename[0] == '.' ) break; /* should examine if a left over file ... */
                             /* test format */
			     nb = sscanf(filename,"%14d%1[_]%s",&datestamp,underline,extension);
                             if ( nb == 3 ) {
			        /* ok get the file & parse */
				r=readlink(ffilename,linkname,1023);
                                memset(buf,'\0',sizeof(buf)); memset(cmd,'\0',sizeof(cmd));
				linkname[r] = '\0';
				if ( (depXp=ParseXmlDepFile( linkname, dmlg )) == NULL ) {
	                                get_time(Time,1);
	                                fprintf(dmlg,"DependencyManager(): %s Problem parsing xml file:%s\n",Time,linkname);
				} else {
				        /* Is dependant node still in waiting state? If not, do not submit. */
					if (l2d2_Util_isNodeXState (depXp->xpd_snode, depXp->xpd_slargs, depXp->xpd_sxpdate, depXp->xpd_sname, "waiting") == 0) {
					     fprintf(dmlg,"DependencyManager(): Removing dependency (waiting state of dependant gone) ffilename=%s ; linkname=%s\n",ffilename,linkname);
					     unlink(linkname);
					     unlink(ffilename);
					     break;
					}

                                        if ( strcmp(depXp->xpd_slargs,"") != 0 )  
					        snprintf(largs,sizeof(largs),"-l \"%s\"",depXp->xpd_slargs);
                                        else 
					        strcpy(largs,"");
                                        /* before trying to access lock file, see if dependency is still active */
					epoch_diff=(int)(current_epoch - atoi(depXp->xpd_regtimepoch))/3600; 
					if ( epoch_diff >= l2d2.dependencyTimeOut ) {
					            ret=unlink(linkname);
					            ret=unlink(ffilename);
	                                            fprintf(dmlg,"============= Dependency Timed Out ============\n");
	                                            fprintf(dmlg,"DependencyManager(): Dependency:%s Timed Out\n",filename);
	                                            fprintf(dmlg,"source     exp  name:%s\n",depXp->xpd_sname);
	                                            fprintf(dmlg,"dependency node name:%s\n",depXp->xpd_name);
	                                            fprintf(dmlg,"current_epoch=%d registred_epoch=%d epoch_diff(hours)=%lu\n",current_epoch,atoi(depXp->xpd_regtimepoch), epoch_diff);
						    fprintf(dmlg,"\n");
						    
						    /* log into exp. nodelog file */
						    get_time(Time,1);
						    snprintf(buf,sizeof(buf),"Dependency on exp:%s node:%s from exp:%s and node:%s Timed out. Removed by mserver at:%s",depXp->xpd_name, depXp->xpd_node, depXp->xpd_sname, depXp->xpd_snode, Time);
					            snprintf(cmd,sizeof(cmd),"exec >/dev/null 2>&1;%s; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; nodelogger -n %s %s -s info -m \"%s\" ",l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_sxpdate, depXp->xpd_snode, largs, buf);
					            ret=system(cmd);
						    
						    /* do an initnode */
                                                    memset(cmd,'\0',sizeof(cmd));
					            /* snprintf(cmd,sizeof(cmd),"exec >/users/dor/afsi/rol/tmp/l2d2server/log/v1.4.0/cmd_listing_init 2>&1; %s; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; maestro -s initnode -n %s %s",l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_sxpdate, depXp->xpd_snode, largs);*/
					            /* snprintf(cmd,sizeof(cmd),"exec >/dev/null 2>&1;%s; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; maestro -s initnode -n %s %s",l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_sxpdate, depXp->xpd_snode, largs); */
					            snprintf(cmd,sizeof(cmd),"%s; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; maestro -s initnode -n %s %s",l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_sxpdate, depXp->xpd_snode, largs);
					            ret=system(cmd);

					            /* Notify user by email : See if we need to add it 	    
                                                    memset(buf,'\0',sizeof(buf));
                                                    snprintf(buf,sizeof(buf),"Dependency on Experiment:%s and Node:%s",depXp->xpd_sname,depXp->xpd_name);
			                            ret=sendmail(l2d2.emailTO,l2d2.emailTO,l2d2.emailCC,"Dependency Removed",&buf[0],dmlg);
						    */
						    continue;
					} else if ( access(depXp->xpd_lock,R_OK) == 0 ) {
                                              get_time(Time,4); 
					      pleaf=(char *) getPathLeaf(depXp->xpd_snode);
					      /* where to put listing: xp/listings/server_host/datestamp/node_container/node_name_and_loop */
                                              memset(listings,'\0',sizeof(listings));
					      snprintf(listings,sizeof(listings),"%s/listings/%s%s",depXp->xpd_sname, l2d2.host, depXp->xpd_container);
					      if ( access(listings,R_OK) != 0 )  ret=r_mkdir(listings,1,dmlg);
					      if ( ret != 0 ) fprintf(dmlg,"DM:Could not create directory:%s\n",listings);
                                              memset(listings,'\0',sizeof(listings));
					      if ( strcmp(depXp->xpd_slargs,"") != 0 ) {
					              snprintf(listings,sizeof(listings),"%s/listings/%s/%s/%s_%s.submit.mserver.%s.%s",depXp->xpd_sname,l2d2.host, depXp->xpd_container,pleaf,depXp->xpd_slargs,depXp->xpd_sxpdate,Time);
                                              } else {
					              snprintf(listings,sizeof(listings),"%s/listings/%s/%s/%s.submit.mserver.%s.%s",depXp->xpd_sname,l2d2.host, depXp->xpd_container,pleaf,depXp->xpd_sxpdate,Time);
					      }
					      /* build command */
					      snprintf(cmd,sizeof(cmd),"%s >/dev/null 2>&1; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; maestro -s submit -n %s %s -f %s >%s 2>&1; nodetracer -n %s %s -d %s -type submission -i %s",
						       l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_sxpdate, depXp->xpd_snode, largs, depXp->xpd_flow, listings, depXp->xpd_snode, largs, depXp->xpd_sxpdate, listings);
					      fprintf(dmlg,"dependency submit cmd=%s\n",cmd); 
					      /* take account of concurrency here ie multiple dependency managers! */
					      snprintf(buf,sizeof(buf),"%s/.%s",l2d2.dependencyPollDir,filename);
					      ret=rename(ffilename,buf);
					      if ( ret == 0 ) {
					             ret=unlink(buf); 
						     ret=unlink(linkname);
					             ret=system(cmd); 
                                              }
					}
					
					/* write dependency to be accessed through web page */
					snprintf(buf,sizeof(buf),"<tr><td>%s</td>\n",depXp->xpd_regtimedate);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<td><table><tr><td><font color=\"red\">SRC_EXP</font></td><td>%s</td>\n",depXp->xpd_sname);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">SRC_NODE</font></td><td>%s</td>\n",depXp->xpd_snode);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">DEP_ON_EXP</font></td><td>%s</td>\n",depXp->xpd_name);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">Key</font></td><td>%s_%s</td>\n",depXp->xpd_xpdate,depXp->xpd_key);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">LOCK</font></td><td>%s</td></table></td>\n",depXp->xpd_lock);
					fwrite(buf, 1 ,strlen(buf), fp );
					free(depXp);depXp=NULL;
				}
			     } 
		             break;
                default:
		    	     break;
            } /* end switch */
	 } /* end while readdir */
	
	 /* check controller */
	 if ( stat(ControllerAlive,&st) == 0 ) {
	        if ( (diff_t=abs(difftime(current_epoch,st.st_mtime))) > 20 ) {
	              if ( (ret=kill(l2d2.pid,0)) != 0 ) {
			      fprintf(dmlg,"Controller pid=%u is dead\n",l2d2.pid); 
			      KILL_SERVER=TRUE;
		      }
		}
	 } else {
	     fprintf(dmlg,"Could not stat controller heartbeat file, pid=%u\n",l2d2.pid);
	     if ( (ret=kill(l2d2.pid,0)) != 0 ) {
		      fprintf(dmlg,"Controller pid=%u is dead\n",l2d2.pid); 
		      KILL_SERVER=TRUE;
             }
         }

         if ( KILL_SERVER ) {
	       char ThreeChar[4];
	       snprintf(buf,sizeof(buf),"%s/EW_*",l2d2.tmpdir);
	       g_lres = glob(buf, GLOB_NOSORT , NULL, &g_LogFiles);
               if ( g_lres == 0 && g_LogFiles.gl_pathc > 0  ) {
	              for (p=g_LogFiles.gl_pathv , cnt=g_LogFiles.gl_pathc ; cnt ; p++, cnt--) {
			   if ( (ret=sscanf(*p, "%[^EW_]%[EW_]%d", buf, ThreeChar, &epid)) == 3 ) {
		                 fprintf(dmlg,"Killing Eternal worker having pid:%d from Dependency Manager Process\n",epid);
				 ret=kill(epid,9);
			   } else
		                 fprintf(dmlg,"Trying to kill Eternal Worker=%s, Parsing Error buf=%s pid=%d num=%d\n",*p,buf,epid,ret);
		      }
	       }
	       /* kill DM (self) */
	       exit(1); 
	 }

	 /* heartbeat & cascading log files :: each 2 minutes */
	 if ( (diff_t=difftime(current_epoch,start_epoch)) >= 120 ) {
	         start_epoch=current_epoch;
		 if ( (ret=utime(DependencyMAlive,NULL)) != 0 ) {
                   if ( (ft=fopen(DependencyMAlive,"w+")) != NULL ) fclose(ft);
		 }
	         /* cascade log file if time to do so */
	         get_time(Time,1);
	         if ( strncmp(tlog,Time,8) != 0 ) {
	                    snprintf(tlog,sizeof(tlog),"%.8s",Time);
	                    /* close old dmlg file */
	                    fprintf(dmlg,"Cascading log file at:%s\n", Time);
			    fclose(dmlg);
                            snprintf(buf,sizeof(buf),"%s_%.8s_%d",l2d2.dmlog,Time,l2d2.depProcPid);
                            if ( (dmlg=fopen(buf,"w+")) == NULL ) {
                                   /* write somewhere */ 
	                           exit(1); /* could we have another alt. */
                            }
                            setvbuf(dmlg, NULL, _IONBF, 0);
		 }
         }
	 

	 /* clean of log files : check each 30 min (1800s)  */
	 if ( l2d2.clean_times.clean_flag == 1 && (diff_t=difftime(current_epoch,start_epoch_cln)) >= 1800 ) {
	           start_epoch_cln=current_epoch;
	           snprintf(buf,sizeof(buf),"%s/meworker_*",l2d2.logdir);
		   g_lres = glob(buf, GLOB_NOSORT , NULL, &g_LogFiles);
                   if (  g_lres == 0 && g_LogFiles.gl_pathc > 0  ) {
		          for (p=g_LogFiles.gl_pathv , cnt=g_LogFiles.gl_pathc ; cnt ; p++, cnt--) {
				   if ( stat(*p, &st) == 0) {
					   epoch_diff=(int)(current_epoch - st.st_mtime)/3600; 
					   if ( epoch_diff >= l2d2.clean_times.eworker_clntime ) {
					          ret=unlink(*p);
					   }
				   }
			  }
			  globfree(&g_LogFiles);
		   }
	           snprintf(buf,sizeof(buf),"%s/mtworker_*",l2d2.logdir);
		   g_lres = glob(buf, GLOB_NOSORT , NULL, &g_LogFiles);
                   if (  g_lres == 0 && g_LogFiles.gl_pathc > 0  ) {
		          for (p=g_LogFiles.gl_pathv , cnt=g_LogFiles.gl_pathc ; cnt ; p++, cnt--) {
				   if ( stat(*p, &st) == 0) {
					   epoch_diff=(int)(current_epoch - st.st_mtime)/3600; 
					   if ( epoch_diff >= l2d2.clean_times.tworker_clntime ) {
					          ret=unlink(*p);
					   }
				   }
			  }
			  globfree(&g_LogFiles);
		   }
	           snprintf(buf,sizeof(buf),"%s/mdpmanager_*",l2d2.logdir);
		   g_lres = glob(buf, GLOB_NOSORT , NULL, &g_LogFiles);
                   if (  g_lres == 0 && g_LogFiles.gl_pathc > 0  ) {
		          for (p=g_LogFiles.gl_pathv , cnt=g_LogFiles.gl_pathc ; cnt ; p++, cnt--) {
				   if ( stat(*p, &st) == 0) {
					   epoch_diff=(int)(current_epoch - st.st_mtime)/3600; 
					   if ( epoch_diff >= l2d2.clean_times.dpmanager_clntime ) {
					          ret=unlink(*p);
					   }
				   }
			  }
			  globfree(&g_LogFiles);
		   }
         }

	 fwrite(page_end_dep, 1, strlen(page_end_dep) , fp);
	 fclose(fp);
	 closedir(dp);
     }
}

/*
 *  Worker Process : for locking & logging, can be Eternal or Transient.
 *  This process will handle a client Session with Multiplexing.
 */
static void l2d2SelectServlet( int listen_sd , TypeOfWorker tworker)
{
 
  FILE *fp, *mlog;
  fd_set master_set, working_set;
  int buflen,num,ret;
  int i,j,k,count,try;
  unsigned int pidSent;
  int _ZONE_ = 1, STOP = -1, SelecTimeOut;
  
  char buf[1024],buff[1024];
  char Astring[1024],inode[128], expName[256], expInode[64], hostname[128]; 
  char Bigstr[2048];
  char heartbeatFile[1024];
  char node[256], signal[256], username[256];
  char Stime[25],Etime[25], tlog[10];
  char m5[40];
  char filename[1024];

  _l2d2client l2d2client[1024]; /* the select can take 1024 max  */

  int  max_sd, new_sd;
  int  desc_ready, end_server = FALSE;
  int  rc, close_conn, got_lock;
  int  mode,ceiling=0;
  int  fd;           
  int sent;            
  struct stat stbuf; 
  char *ts;
  char trans;

  key_t log_key;
 
  struct sigaction ssa;
  struct timeval timeout;   /* Timeout for select */
  struct flock nlock,ilock; /* for Logging we are using fnctl() */
  glob_t g_AliveFiles;
  int g_result;
  time_t sig_sent,now;
  double delay;

  /* open log files */
  get_time(Stime,1);
  snprintf(tlog,sizeof(tlog),"%.8s",Stime);
  
  memset(buf,'\0',sizeof(buf));
  if ( tworker == ETERNAL ) 
      snprintf(buf,sizeof(buf),"%s/meworker_%.8s_%d",L2D2.logdir,Stime,getpid());
  else
      snprintf(buf,sizeof(buf),"%s/mtworker_%.8s_%d",L2D2.logdir,Stime,getpid());
  

  /* if logging directory was removed -> NO logs  for the 3 servers */ 
  if ( (mlog=fopen(buf,"w+")) != NULL ) {
         setvbuf(mlog, NULL, _IONBF, 0); /* streams will be unbuffered */
  }


  /* for heartbeat */ 
  if ( tworker == ETERNAL ) {
        snprintf(heartbeatFile,sizeof(heartbeatFile),"%s/EW_%d",L2D2.tmpdir,getpid());
  } else { 
	snprintf(heartbeatFile,sizeof(heartbeatFile),"%s/TRW_%d",L2D2.tmpdir,getpid());
  }
       
  /* create files */
  if ( (fp=fopen(heartbeatFile,"w+")) != NULL ) {
          fclose(fp);
  }
 
  /*
  === This could be used in future ====
  register SIGALRM signal for session control with each client,though it will need sig suspend 
  ssa.sa_handler = recv_handler;
  ssa.sa_flags = 0;
  sigemptyset(&ssa.sa_mask);
  if ( sigaction(SIGALRM,&ssa,NULL) == -1 ) fprintf(mlog,"Error registring signal in SelectServlet \n");
  */

  FD_ZERO(&master_set);
  max_sd = listen_sd;
  FD_SET(listen_sd, &master_set);

  /* get reference time to manage sending of signals to add workers to main server */
  time(&sig_sent);

  /* fix timeout for each type of workers */
  if ( tworker == ETERNAL ) {
          SelecTimeOut=ETERNAL_WORKER_STIMEOUT;
  } else {
          SelecTimeOut=TRANSIENT_WORKER_STIMEOUT;
  }

  /* Loop waiting for incoming connects or for incoming data on any of the connected sockets. */
  do
  {
      /* Copy the master fd_set over to the working fd_set. */
      memcpy(&working_set, &master_set, sizeof(master_set));
  
      /* set timeout for select SELECT_TIMEOUT minutes      */
      timeout.tv_sec = SelecTimeOut ;
      timeout.tv_usec = 0;

      /* Call select() with timeout                         */
      rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

      /* Check to see if the select call failed.            */
      if (rc < 0) {
	 if ( mlog != NULL ) fprintf(mlog,"select() failed: Worker end \n");
         break;
      }

      /* Check to see if select call timed out ... yes -> do admin. things */ 
      if (rc == 0) {
          if ( tworker != ETERNAL ) {
             ret=unlink(heartbeatFile);
	     get_time(Stime,2);
	     if ( mlog != NULL ) {
	           fprintf(mlog,"Transient worker process exited pid=%lu at:%s\n", (unsigned long) getpid(), Stime );
	           fclose(mlog);
	     }
	     exit(0); /* send SIGCHLD to controller */
          } else {
	     /* cascade log file if time to do so */
	     get_time(Stime,1);
	     if ( strncmp(tlog,Stime,8) != 0 && mlog != NULL ) {
	            fprintf(mlog,"Cascading log file at:%s\n", Stime);
	            snprintf(tlog,sizeof(tlog),"%.8s",Stime);
	            /* close old mlog file */
	            fclose(mlog);
                    snprintf(buf,sizeof(buf),"%s/meworker_%.8s_%d",L2D2.logdir,Stime,getpid());
	            if ( (mlog=fopen(buf,"w+")) == NULL ) {
	                      fprintf(stdout,"Worker: Could not open mlog stream\n"); /* doesn't go anywhere */
	            } else {
	                setvbuf(mlog, NULL, _IONBF, 0);
                    }
	     }
	  }
      }
      
      /* heartbeat */
      ret=utime(heartbeatFile,NULL); 

      /* One or more descriptors are readable. Need to           
         determine which ones they are.                         */
      desc_ready = rc;
      for (i=0; i <= max_sd  &&  desc_ready > 0; ++i)
      {
         /* Check to see if this descriptor is ready            */
         if (FD_ISSET(i, &working_set))
         {
            /* A descriptor was found that is readable - one     
               less has to be looked for.  This is being done     
               so that we can stop looking at the working set     
               once we have found all of the descriptors that     
               were ready. */
            
	    desc_ready -= 1;

            /* Check to see if this is the listening socket */
            if (i == listen_sd)
            {
               /* Accept all incoming connections that are        
                  queued up on the listening socket before we     
                  loop back and call select again. */
               do
               {
                  /* Accept each incoming connection.  If       
                     accept fails with EWOULDBLOCK, then we     
                     have accepted all of them.  Any other      
                     failure on accept will cause us to end the 
                     server.  */
                  new_sd = accept(listen_sd, NULL, NULL);
                  if (new_sd < 0) {
                     if (errno != EWOULDBLOCK) {
	                    if ( mlog != NULL ) fprintf(mlog,"accept() failed \n");
                            end_server = TRUE;
                     }
                     break;
                  }

                  /* Add the new incoming connection to the master read set. 
		   * And Examine maximum simulatenous connected Clients 
		   * Note, we do not reject accepts, but we rely on the main 
		   * server to spawn a new worker to lower the load and also 
		   * hoping that after 5 sec, the load is fairly distributed
		   * btw the workers. */
		  
		  /* put the socket in non-blocking mode 
		     I think there is no need for this , the socket will inherit the
		     state from the listening socket */
		  if  ( ! (fcntl(new_sd, F_GETFL) & O_NONBLOCK)  ) {
		        if  (fcntl(new_sd, F_SETFL, fcntl(new_sd, F_GETFL) | O_NONBLOCK) < 0) {
		            if ( mlog != NULL ) fprintf(mlog,"Could not put the socket in non-blocking mode\n");
		        }
		  }
                  
		  FD_SET(new_sd, &master_set);
                  if (new_sd > max_sd) max_sd = new_sd; /* keep track of the max */
		  
		  ceiling++;
		  if ( ceiling >= L2D2.maxClientPerProcess ) {
			time(&now);
			/* wait SPAWNING_DELAY_TIME secondes btw sending of signals to main server,this is for signal flood control 
			   Note : Only Eternal worker is able to send signal for adding workers */
			if ( (delay=difftime(now,sig_sent)) >= SPAWNING_DELAY_TIME ) {
		               get_time(Stime,3);
			       if ( tworker == ETERNAL ) {
		                     if ( mlog != NULL ) fprintf(mlog,"Signal sent at:%s to main server to add a worker\n",Stime);
		                     kill(L2D2.pid,SIGUSR2);
                               }
			       sig_sent=now;
			}
		  }

                 /* Loop back up and accept another incoming connection */
               } while (new_sd != -1);
            }

            /* This is not the listening socket, therefore an     
               existing connection must be readable             */
            else
            {
               close_conn = FALSE;
               /* Receive all incoming data on this socket        
                  before we loop back and call select again.    */
               memset(buff,'\0',sizeof(buff));
	       count=0;
               do
               {
		/* Receive data on this connection until the    
                   recv fails with EWOULDBLOCK.  If any other   
                   failure occurs, we will close the connection. */
		   
                   rc = recv(i, buff, sizeof(buff), 0);
                   if (rc < 0) {
                      if (errno != EWOULDBLOCK) {
                           get_time(Stime,3);
                           if ( mlog != NULL ) logZone(L2D2.dzone,L2D2.dzone,mlog,"recv() failed with Host:%s AT:%s Exp=%s Node:%s Signal:%s\n",l2d2client[i].host, Stime, l2d2client[i].xp, l2d2client[i].node, l2d2client[i].signal);
                           close_conn = TRUE;
                      }
                      break;
                    }

                    /* Check to see if the connection has been closed by the client  */
                   if (rc == 0) {
                        get_time(Stime,3);
                        if ( mlog != NULL ) logZone(_ZONE_,L2D2.dzone,mlog,"%s Closed:%s TrNum=%u\n",l2d2client[i].Open_str,Stime,l2d2client[i].trans);
                        close_conn = TRUE;
                        break;
                   }
                   
                   /* work on data (requests)  */
                   buff[rc > 0 ? rc : 0] = '\0';
		  
                   switch (buff[0]) {
	                       case 'A': /* test existence of file  */
			                memset(filename,'\0',sizeof(filename));
					ret = sscanf(&buff[2],"%s %d",filename, &mode);
	                                ret = access (filename , mode);
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'C': /*  create a Lock file  */
	                                ret = CreateLock ( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'D': /* mkdir  */
	                                ret = r_mkdir ( &buff[2] , 1, mlog);
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'F': /* test existence of file  */
	                                ret = isFileExists ( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'G': /* glob  */
			                ret = globPath (&buff[2], GLOB_NOSORT, 0, mlog );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'I': /* accept session */
			                 pidSent=0;
					 memset(expInode,'\0',sizeof(expInode));
					 memset(expName,'\0',sizeof(expName));
					 memset(node,'\0',sizeof(node));
					 memset(hostname,'\0',sizeof(hostname));
					 memset(username,'\0',sizeof(username));
					 memset(m5,'\0',sizeof(m5));
                                         ret=sscanf(&buff[2],"%d %s %s %s %s %s %s %s",&pidSent,expInode,expName,node,signal,hostname,username,m5);
                                         get_time(Stime,3);
                                         if ( ret != 8 ) {
	                                         send_reply(i,1);
                                                 if ( mlog != NULL ) fprintf (mlog,"Got wrong number of parameters at LOGIN, number=%d instead of 8 buff=>%s<\n",ret,buff);
	                                         /* close(i);same comment as for the S case below */
			                         ret=shutdown(i,SHUT_WR);
					         ceiling--;
                                                 snprintf(l2d2client[i].Open_str,sizeof(l2d2client[i].Open_str),"Session Refused with Host:%s AT:%s Exp=%s Node=%s Signal=%s ... Wrong number of arguments ",hostname , Stime, expName, node, signal);
                                         } else if ( pidTken == pidSent && strcmp(m5,L2D2.m5sum) == 0 ) {
	                                         send_reply(i,0);
						 snprintf(l2d2client[i].Open_str,sizeof(l2d2client[i].Open_str),"OpenConHost:%s At:%s Xp=%s Node=%s Signal=%s NumCon=%d ",hostname, Stime, expName, node ,signal, ceiling);
                                         } else {
	                                         send_reply(i,1);
	                                         /* close(i);same comment as for the S case below */
			                         ret=shutdown(i,SHUT_WR);
						 ceiling--;
                                                 snprintf(l2d2client[i].Open_str,sizeof(l2d2client[i].Open_str),"Session Refused with Host:%s AT:%s Exp=%s Node=%s Signal=%s pid_svr=%d pid_sent=%d m5_client=%s ",hostname , Stime, expName, node, signal, pidTken, pidSent, m5);
                                         }
                                         /* gather info for this client */
					 l2d2client[i].trans=0;
			                 strcpy(l2d2client[i].host,hostname);
			                 strcpy(l2d2client[i].xp,expName);
			                 strcpy(l2d2client[i].node,node);
			                 strcpy(l2d2client[i].signal,signal);
		                         break;
	                       case 'K': /* write Inter user dep file : Not used */
			                break;
	                       case 'L':/* Log the node under the proper experiment ie: grab a lock on a file  */ 
                                        /* Generate a key based on xp's Inode & user id. Same for all process ,we are not using ftok
		                           Note : For this release we discard using the datestamp in generating the key */

                                        memset(buf,'\0',sizeof(buf));

                                        /* regexp for checking existence of TRansient Workers . If there are any , we must use the
					   locking mechanism. if none (only the Eternel worker write directly in nodelog file */ 
	                                snprintf(buf,sizeof(buf),"%s/TRW_*",L2D2.tmpdir);
					g_result = glob(buf, 0, NULL, &g_AliveFiles);

                                        if ( tworker == TRANSIENT || ( g_result == 0 && g_AliveFiles.gl_pathc > 0 ) ) {

					       globfree(&g_AliveFiles);
                                               log_key = (getuid() & 0xff ) << 24 | ( atoi(expInode) & 0xffff) << 16;
                                               memset(buf,'\0',sizeof(buf));
	                                       snprintf(buf,sizeof(buf),"%s/NodeLogLock_0x%x",L2D2.tmpdir,log_key);
					       /* Open a file descriptor to the file.  what if +1 open at the same time? */
					       if ( (fd = open (buf, O_WRONLY|O_CREAT, S_IRWXU)) < 0 ) {
                                                      if ( mlog != NULL ) fprintf(mlog,"ERROR Opening NodeLogLock file\n");
			                              send_reply(i,1);
					              break;
					       }
					       memset (&nlock, 0, sizeof(nlock));
					       nlock.l_type = F_WRLCK;
					       /* NOTE: F_SETLKW -> fnc suspend until lock be placed, 
					                F_SETLK  -> fnc returns immedialtly if lock cannot be placed */
                                               got_lock = FALSE;

					       /* try for (MAX_LOCK_TRY - 1) * 0.25 sec ie= 2.5 sec */
                                               for (  try = 0; try < MAX_LOCK_TRY; try++ ) {
					           if (fcntl(fd, F_SETLK, &nlock) == -1) {
					                usleep(250000);
					           } else { 
					                got_lock = TRUE;
					                break;
					           }
					       }

                                               if ( got_lock == FALSE ) {
					              fcntl(fd, F_GETLK, &ilock);
					              if ( mlog != NULL ) fprintf(mlog,"ERROR Cannot get lock, owned by pid:%d exp=%s node=%s\n", ilock.l_pid, expName, node);
                                                      close(fd);
			                              send_reply(i,1);
					              break;
					       }

	                                       ret = NodeLogr( &buff[2] , getpid(), mlog );
			                       send_reply(i,ret);
                         
                                               nlock.l_type = F_UNLCK;
                                               if (fcntl(fd, F_SETLK, &nlock) == -1) {
                                                      if ( mlog != NULL ) fprintf(mlog,"ERROR unlocking NodeLogLock file\n");
					       }
                                               close(fd);
                                        } else {  
					       /* one Serial worker ie Eternel worker */ 
	                                       ret = NodeLogr( &buff[2] , getpid(), mlog );
			                       send_reply(i,ret);
					}
			  
					l2d2client[i].trans++;
			                break;
	                       case 'N': /* grab a lock for End state */
		                        ret = lock( &buff[2] ,   L2D2 ,expName, node, mlog ); 
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'P': /* unlock End state */ 
		                        ret = unlock( &buff[2] , L2D2 ,expName, node, mlog ); 
			                send_reply(i,ret);
					l2d2client[i].trans++;
		                        break;
	                       case 'R': /* Remove file on local xp */
	                                ret = removeFile( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'S': /* Client has sent a Stop. OK Close this connection 
			                Note: if we use close(i) here , the rcv() call will fail, we need to close
					the server write side and let the client close his side, the server will then
					report a closed socket */
			                ret=shutdown(i,SHUT_WR);
			                STOP = TRUE; /* var STOP not used for the moment */
			                break;
	                       case 'T': /* Touch a Lock file on local xp */
	                                ret = touch ( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'W': /* write Node Wait file  under dependent-ON xp */
			                ret = writeNodeWaitedFile ( &buff[2] , mlog );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'X': /* server shutdown */
		                        kill(L2D2.pid,SIGUSR1);
			                ret=shutdown(i,SHUT_WR);
			                STOP = TRUE;
			                break;
	                       case 'Y': /* server is alive need to return more here? */
		                        get_time(Stime,1);
                                        time(&now);
                                        memset(buf,'\0',sizeof(buf));
	                                snprintf(buf,sizeof(buf),"%s/DM_*",L2D2.tmpdir);
					g_result = glob(buf, 0, NULL, &g_AliveFiles);
                                        if (  g_result == 0 && g_AliveFiles.gl_pathc == 1  ) {
				                 if ( stat(*(g_AliveFiles.gl_pathv), &stbuf) == 0) {
					                epoch_diff=abs(now - stbuf.st_mtime); 
							/* 120 sec for DM heartbeat */
					                if ( epoch_diff >=  180  ) {
	                                                        snprintf(buf,sizeof(buf),"0 Problems with Dependency Manager: heartbeat \0");
					                        ret=write(i,buf,strlen(buf));
			                                        break;
					                }
						 }
                                        } else if (g_AliveFiles.gl_pathc > 1 ) {
	                                       snprintf(buf,sizeof(buf),"0 Problems with Dependency Manager: Multiple instances\0");
					       ret=write(i,buf,strlen(buf));
			                       break;
					} else {
					       /* issue a kill here before sending message */
	                                       snprintf(buf,sizeof(buf),"0 Problems with Dependency Manager: process dead \0");
					       ret=write(i,buf,strlen(buf));
			                       break;
					}

			                if ( tworker != ETERNAL ) {
                                             time(&now);
                                             memset(buf,'\0',sizeof(buf));
	                                     snprintf(buf,sizeof(buf),"%s/DM_*",L2D2.tmpdir);
					     g_result = glob(buf, 0, NULL, &g_AliveFiles);
                                             if (  g_result == 0 && g_AliveFiles.gl_pathc == 1  ) {
				                      if ( stat(*(g_AliveFiles.gl_pathv), &stbuf) == 0) {
					                     epoch_diff=(int)(now - stbuf.st_mtime); 
							     /* 60 sec for EW heartbeat if no coonections  */
					                     if ( epoch_diff >= 100 ) {
	                                                             snprintf(buf,sizeof(buf),"0 Problems with Eternal worker: heartbeat\0");
					                             ret=write(i,buf,strlen(buf));
			                                             break;
					                     }
						      }
                                             } else if (g_AliveFiles.gl_pathc > 1 ) {
	                                            snprintf(buf,sizeof(buf),"0 Problems with Eternal worker: Multiple instances\0");
					            ret=write(i,buf,strlen(buf));
			                            break;
					     } else {
					       /* issue a kill here before sending message */
	                                       snprintf(buf,sizeof(buf),"0 Problems with Eternal worker: process dead \0");
					       ret=write(i,buf,strlen(buf));
			                       break;
					     }
					} 

			                /* send_reply(i,0); */
	                                snprintf(buf,sizeof(buf),"0 Server is Alive on host=%s version=%s, Dependency Manager ok, Eworker ok \0",L2D2.host, L2D2.mversion);
					ret=write(i,buf,strlen(buf));
			                break;
	                       case 'Z':/* download waited file to client */
		                        ret = SendFile( &buff[2] , i, mlog ); 
					/* if waited file not there really , client will abort
					   connection will eventualy be closed */
					l2d2client[i].trans++;
		              	        break;
                               default :
	                                if ( mlog != NULL ) fprintf(mlog,"Unrecognized Token>%s< from Host=%s Exp=%s node=%s signal=%s \n",buff,l2d2client[i].host,l2d2client[i].xp,l2d2client[i].node,l2d2client[i].signal);
			                send_reply(i,1); 
			                break;
                       }
		       count++;
               } while (TRUE); 

               /* If the close_conn flag was turned on, we need   
                  to clean up this active connection.  This       
                  clean up process includes removing the          
                  descriptor from the master set and              
                  determining the new maximum descriptor value    
                  based on the bits that are still turned ON in   
                  the master set.                               */

               if (close_conn) {
                     close(i);
                     FD_CLR(i, &master_set);
                     if (i == max_sd) {
                        while (FD_ISSET(max_sd, &master_set) == FALSE) max_sd -= 1;
                     }
		     /* dont think that this re-initializing is mandatory */
		     l2d2client[i].trans=0;
		     memset(l2d2client[i].host,'\0',sizeof(l2d2client[i].host));
		     memset(l2d2client[i].xp,'\0',sizeof(l2d2client[i].xp));
		     memset(l2d2client[i].node,'\0',sizeof(l2d2client[i].node));
		     memset(l2d2client[i].signal,'\0',sizeof(l2d2client[i].signal));
		     memset(l2d2client[i].Open_str,'\0',sizeof(l2d2client[i].Open_str));
		     memset(l2d2client[i].Close_str,'\0',sizeof(l2d2client[i].Close_str));
		     ceiling--;
               }
            } /* End of existing connection is readable */
         } /* End of if (FD_ISSET(i, &working_set)) */
      } /* End of loop through selectable descriptors */
   } while (end_server == FALSE);

   /* Cleanup all of the sockets that are open                  */
   for (i=0; i <= max_sd; ++i) {
      if (FD_ISSET(i, &master_set)) close(i);
   }
}


void maestro_l2d2_main_process_server (int fserver)
{
  FILE *smlog, *fp;
  pid_t pid_eworker, kpid;  /* pid_eworker : pid of eternal worker */
  int ret,status,i=0,j,ew_regenerated=1,dm_regenerated=1;
  char *m5sum=NULL, *Auth_token=NULL;
  char authorization_file[1024], filename[1024], isAliveFile[128];
  const char message[256];
  time_t current_epoch, epoch_cln ;
  struct sigaction adm;
  struct stat st;
  char Time[40], tlog[16];
  glob_t g_LogFiles;
  size_t cnt;
  int  fd, g_lres;
  char **p;
  struct passwd *passwdEnt = getpwuid(getuid());

  bzero(&adm, sizeof(adm));
  adm.sa_sigaction = &sig_admin;
  adm.sa_flags = SA_SIGINFO;
  
  /* Open log files */
  get_time(Time,1);
  snprintf(tlog,sizeof(tlog),"%.8s",Time);
  snprintf(filename,sizeof(filename),"%s_%.8s_%d",L2D2.mlog,Time,L2D2.pid);
  if ( (smlog=fopen(filename,"w+")) == NULL ) {
            fprintf(stderr,"Main server: Could not open mlog stream\n");
            unlink(L2D2.auth);
	    exit(1);
  } else {
	    get_time(Time,2);
            fprintf(smlog,"Main server: starting at:%s pid=%d host:%s \n", Time, L2D2.pid, L2D2.host);
  }
 
  /* redirect streams : same comment as in Dependency manager */
  close(STDIN_FILENO);
  if (fd = open("/dev/null", O_RDONLY, 0) != -1) {
        if(dup2(fd, STDIN_FILENO) < 0) {
              perror("dup2 stdin");
             exit (1);
        }
  }
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  if (fd = open("/dev/null", O_RDWR, 0) != -1) {
        if(dup2(fd, STDOUT_FILENO) < 0) {
             perror("dup2 stdout");
             exit (1);
        }
        if(dup2(fd, STDERR_FILENO) < 0) {
             perror("dup2 stderr");
             exit (1);
        }
  }
  if (fd > STDERR_FILENO) {
        if(close(fd) < 0) {
             perror("close");
             exit (1);
        }
  }

  /* Streams will be unbuffered */
  setvbuf(smlog, NULL, _IONBF, 0);

  /* Register signal for admin purposes */
  if ( sigaction(SIGUSR1, &adm, NULL) != 0 ) fprintf(smlog,"ERROR in sigaction signal :USR1");
  if ( sigaction(SIGUSR2, &adm, NULL) != 0 ) fprintf(smlog,"ERROR in sigaction signal :USR2");
  if ( sigaction(SIGHUP,  &adm, NULL) != 0 ) fprintf(smlog,"ERROR in sigaction signal :HUP");

  /* Generate Eternal worker */
  if ( (pid_eworker = fork()) == 0 ) { 
      fclose(smlog);
      l2d2SelectServlet( fserver , ETERNAL );
  } else if ( pid_eworker > 0 ) {      
      fprintf(smlog,"Main server: creating the first Worker pid=%d\n", pid_eworker);
  } else {                        
      fprintf(smlog,"Failed to fork the First Eternal Worker ... mserver exiting\n");
      close(fserver); 
      kill(L2D2.depProcPid,9);
      l2d2server_shutdown(pid_eworker,smlog);
  }

  /* Init clean epoch */
  epoch_cln = time(NULL);
  
  /* Generate file to track if controller is alive */
  snprintf(isAliveFile,sizeof(isAliveFile),"%s/CTR_%d",L2D2.tmpdir,getpid());
  if ( (fp=fopen(isAliveFile,"w+")) != NULL ) {
          fclose(fp);
  }

  /* Go into the main admin loop */
  for (;;)
  {
        /* I Am alive */
	if ( (ret=utime(isAliveFile,NULL)) != 0 ) {
              if ( (fp=fopen(isAliveFile,"w+")) != NULL ) {
                      fclose(fp);
              }
	}

	if ( sig_admin_AddWorker == 1 ) {
	      sig_admin_AddWorker=0;
	      get_time(Time,3);
	      fprintf(smlog,"Received signal USR2 at:%s from worker\n",Time);
	      
              if ( ProcessCount < L2D2.maxNumOfProcess && ProcessCount < MAX_PROCESS ) {
                 if ( (ChildPids[ProcessCount] = fork()) == 0 ) { 
		      fclose(smlog);
                      l2d2SelectServlet( fserver , TRANSIENT );
		      exit(0); /* never reached */
                 } else if ( ChildPids[ProcessCount] > 0 ) {        
                      ProcessCount++;
	              fprintf(smlog,"One worker generated with pid=%u at:%s Actual ProcessCount=%d\n",ChildPids[ProcessCount],Time,ProcessCount);
                 } else                      
                      fprintf(smlog,"fork() Worker failed\n");
              } else {
	            fprintf(smlog,"cannot add more worker, already reached maximum:%d\n",ProcessCount);
              }
	}
	

        /* sigchild :: a Child has exited ... dont know who is */
	if ( sig_child == 1 ) {
	     sig_child=0;
	     /* Check Eternal worker */
	     if ( (ret=kill(pid_eworker,0)) != 0 ) {
	           get_time(Time,1);
                   if ( ew_regenerated == 3 ) {
	                 fprintf(smlog,"Eternal Worker has been Re-generated 2 times already .. exiting at :%s\n",Time);
	                 close(fserver);
                         for ( j=0; j < MAX_PROCESS ; j++ ) {
	                       ChildPids[j] == 0 ? : kill (ChildPids[j],9);
                         }
                         kill(L2D2.depProcPid,9);
			 /* send email notice */
                         snprintf(message,sizeof(message),"maestro server (pid=%u) died (at:%s) after being re-started 2 times, Please check",L2D2.pid,Time);
			 ret=sendmail(L2D2.emailTO,L2D2.emailTO,L2D2.emailCC,"maestro server (mserver) failure",&message[0],smlog);
	                 l2d2server_shutdown(pid_eworker,smlog);
		   } else {
	                 get_time(Time,2);
	                 fprintf(smlog,"Eternal Worker is dead (pid=%d) ... trying to spawn a new one (count=%d) at:%s\n",pid_eworker,ew_regenerated,Time);
	                 snprintf(filename,sizeof(filename),"%s/EW_%d",L2D2.tmpdir,pid_eworker);
			 ret=unlink(filename);
                         if ( (pid_eworker = fork()) == 0 ) { 
		                    fclose(smlog);
                                    l2d2SelectServlet( fserver , ETERNAL );
		                    exit(0); /* never reached */
                         } else if ( pid_eworker > 0 ) {      
                                    fprintf(smlog,"Main server: creating a Eworker pid=%d at:%s\n", pid_eworker, Time);
			            ew_regenerated++;
                         } else {
			      fprintf(smlog,"Not able to fork The Eternal Worker"); 
			      /* send email */
			      exit(1);
			 } 
                   }
	     } 
	     
	     /* Check Dependency Manager */
	     if ( (ret=kill(L2D2.depProcPid,0)) != 0 ) {
	           get_time(Time,1);
	           if ( dm_regenerated == 2 ) {
		        fprintf(smlog,"Dependency manager has been Re-generated once already .. exiting at:%s\n",Time);
	                kill(pid_eworker,9);
                        for ( j=0; j < MAX_PROCESS ; j++ ) {
	                       ChildPids[j] == 0 ? : kill (ChildPids[j],9);
                        }
                        snprintf(message,sizeof(message),"Dependency manager (pid=%u) died (at:%s) after being re-started, mserver exiting, Please check",L2D2.depProcPid,Time);
			ret=sendmail(L2D2.emailTO,L2D2.emailTO,L2D2.emailCC,"maestro server (mserver) failure",&message[0],smlog);
	                l2d2server_shutdown(pid_eworker,smlog);
                   } else {
	                fprintf(smlog,"Dependency Manager is dead (pid=%d) ... trying to spawn a new one at:%s\n",L2D2.depProcPid,Time);
	                snprintf(filename,sizeof(filename),"%s/DM_%d",L2D2.tmpdir, L2D2.depProcPid);
			ret=unlink(filename);
                        if ( (L2D2.depProcPid=fork()) == 0 ) {
                              /*  this is a child, Note: will inherite signals */
                               close(fserver);
		               fclose(smlog);
                               DependencyManager (L2D2) ;
                               exit(0); /* never reached ! */
                        } else if ( L2D2.depProcPid > 0 ) {
	                       /* in parent do nothing */
			       dm_regenerated++;
	                } else {
			     fprintf(smlog,"Not able to fork a Dependency Manager\n");
			     /* send email */
			     exit(1);
                        }
                   }
             }

	     /* Check Transient worker, update variables  */
             for ( j=0; j < MAX_PROCESS; j++ ) {
		 if ( ChildPids[j] != 0 && (ret=kill(ChildPids[j],0)) != 0 ) {
	                     get_time(Time,2);
                             ProcessCount = (ProcessCount < 0) ? 0 : ProcessCount-1;
	                     fprintf(smlog,"Worker process pid:%u has terminated time=%s ProcessCount=%d\n", ChildPids[j], Time, ProcessCount);
                             ChildPids[j]=0;
		 }
	     }
	}
        
	/* Heartbeat: 
	   both of [E|T]Worker and DManager update a file on tmpdir, This section examine the
	   modif time of the file to see if processes are still alive, this redundency is
	   added in case we miss signals 
	   Note : the Files TW_* (transient worker) are not examined */

	current_epoch = time(NULL);
	/* Eternal worker       */
	snprintf(filename,sizeof(filename),"%s/EW_%d",L2D2.tmpdir,pid_eworker);
	if ( stat(filename,&st) == 0 ) {
	        if ( (diff_t=abs(difftime(current_epoch,st.st_mtime))) > NOTIF_TIME_INTVAL_EW ) {
	              if ( (ret=kill(pid_eworker,0)) != 0 ) {
		                   sig_child=1;
				   ret=unlink(filename);
	                           fprintf(smlog,"Eternal Worker is dead (no heartbeat) ...\n");
		      }
	        } 
        } else if ( (ret=kill(pid_eworker,0)) != 0 ) {
		            sig_child=1;
			    ret=unlink(filename);
	                    fprintf(smlog,"Eternal worker is dead (no heartbeat) ...\n");
	}

	/* Dependency Manager  */
	 snprintf(filename,sizeof(filename),"%s/DM_%d",L2D2.tmpdir,L2D2.depProcPid);
	 if ( stat(filename,&st) == 0 ) {
	            if ( (diff_t=abs(difftime(current_epoch,st.st_mtime))) > NOTIF_TIME_INTVAL_DM ) {
	                if ( (ret=kill(L2D2.depProcPid,0)) != 0 ) {
		                   sig_child=1;
				   ret=unlink(filename);
	                           fprintf(smlog,"Dependency manager is dead (no heartbeat) ...\n");
			}
                    } 
         } else if ( (ret=kill(L2D2.depProcPid,0)) != 0 ) {
		           sig_child=1;
			   ret=unlink(filename);
	                   fprintf(smlog,"Dependency manager is dead (no heartbeat) ...\n");
	 }

        /* Shut down server */
        if ( sig_admin_Terminate == 1 ) {
	     close(fserver);
             sleep (2);
             for ( j=0; j < MAX_PROCESS; j++ ) {
	           ChildPids[j] == 0 ? : kill (ChildPids[j],9);
             }
	     kill(pid_eworker,9);
             kill(L2D2.depProcPid,9);
	     l2d2server_shutdown(pid_eworker,smlog);
	}

	/* Test to see if user has started a new mserver */
        snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",L2D2.mversion);
        Auth_token=get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);
	if ( Auth_token != NULL ) {
	   if ( strcmp(m5sum,L2D2.m5sum) != 0 ) {
	       get_time(Time,1);
	       fprintf(smlog,"mserver::Error md5sum has changed File=%s new=%s old=%s ... at:%s killing server\n",L2D2.auth,m5sum,L2D2.m5sum,Time);
               for ( j=0; j < MAX_PROCESS; j++ ) {
	           ChildPids[j] == 0 ? : kill (ChildPids[j],9);
               }
	       kill(pid_eworker,9);
	       l2d2server_remove(smlog);
	   } else {
	           free(Auth_token); 
	           free(m5sum); 
	   }
	}

	/* Wait on Un-caught childs  */
	if ( (kpid=waitpid(-1,NULL,WNOHANG)) > 0 ) { 
               ProcessCount = (ProcessCount < 0) ? 0 : ProcessCount-1;
	       fprintf(smlog,"mserver::Waited on child with pid:%lu\n",(unsigned long ) kpid);
        }

        /* cascading and clean  : check each hour */
	if ( L2D2.clean_times.clean_flag == 1 && (diff_t=difftime(current_epoch,epoch_cln)) >= 3600 ) {
	         epoch_cln=current_epoch;
	         get_time(Time,1);
	         if ( strncmp(tlog,Time,8) != 0 ) {
	                    snprintf(tlog,sizeof(tlog),"%.8s",Time);
	                    fprintf(smlog,"Cascading log file at:%s\n", Time);
			    fclose(smlog);
                            snprintf(filename,sizeof(filename),"%s_%.8s_%d",L2D2.mlog,Time,L2D2.pid);
                            if ( (smlog=fopen(filename,"w+")) == NULL ) {
                                   /* write somewhere */ 
	                           exit(1); /* could we have another alt. */
                            }
                            setvbuf(smlog, NULL, _IONBF, 0);
	                    /* clean */ 
		            snprintf(filename,sizeof(filename),"%s/mcontroller_*",L2D2.logdir);
		            g_lres = glob(filename, GLOB_NOSORT , NULL, &g_LogFiles);
                            if (  g_lres == 0 && g_LogFiles.gl_pathc > 0  ) {
		                   for (p=g_LogFiles.gl_pathv , cnt=g_LogFiles.gl_pathc ; cnt ; p++, cnt--) {
				            if ( stat(*p, &st) == 0) {
					            epoch_diff=(int)(current_epoch - st.st_mtime)/3600; 
					            if ( epoch_diff >= L2D2.clean_times.controller_clntime ) {
					                   ret=unlink(*p);
					            }
				            }
			           }
			           globfree(&g_LogFiles);
		            }
		 }
	}
        
	/* check to see if # of Transient worker alive file is consistent with ProcessCount variable 
	   This glitch could happen sometime 
	snprintf(filename,sizeof(filename),"%s/TRW_*",L2D2.tmpdir);
	g_lres = glob(filename, GLOB_NOSORT , NULL, &g_LogFiles);
        if (  g_lres == 0 && g_LogFiles.gl_pathc != ProcessCount ) {
	}
	*/

        

	sleep(5);  /* yield */              
  }

}

int main ( int argc , char * argv[] ) 
{

  
  struct stat st;
  char *Auth_token=NULL, *m5sum=NULL;
  struct passwd *passwdEnt = getpwuid(getuid());
  
  char authorization_file[256];
  char hostname[128], hostTken[32], ipTken[32];
  char buf[1024];
  
  int fserver;
  int server_port;
  int portTken;
  int ret,status,i;

  char *home = NULL;
  char *ip=NULL;

  /* default values for L2D2.clean_times */
  L2D2.clean_times=L2D2_cleantimes;
  
  /* get maestro current version and shortcut */
  if (  (L2D2.mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
            fprintf(stderr, "maestro_server(),Could not get maestro current version. do a proper ssmuse \n");
            exit(1);
  }

  if (  (L2D2.mshortcut=getenv("SEQ_MAESTRO_SHORTCUT")) == NULL ) {
            fprintf(stderr, "maestro_server(),Could not get maestro current version. do a proper ssmuse \n");
            exit(1);
  }

  /* first of all, create ~user/.suites if not there */
  snprintf(buf,sizeof(buf),"%s/.suites",passwdEnt->pw_dir);
  strcpy(L2D2.home,buf);
  if ( access(buf,R_OK) != 0 ) {
         status=mkdir( buf , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH); 
  }
  
  /* check if directories for dependencies are there */
  snprintf(buf,sizeof(buf),"%s/.suites/maestrod/dependencies/polling/v%s",passwdEnt->pw_dir,L2D2.mversion);
  strcpy(L2D2.dependencyPollDir,buf);
  if ( access(buf,R_OK) != 0 ) {
         if ( (status=r_mkdir(buf ,1,stderr))  != 0 ) { 
                fprintf(stderr, "maestro_server(),Could not create dependencies directory:%s\n",buf);
		/* what to do next? */
	 }
	 
  }

  /* get authorization param */
  snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",L2D2.mversion);
  snprintf(L2D2.auth,sizeof(L2D2.auth),"%s/.suites/%s",passwdEnt->pw_dir,authorization_file);
  Auth_token=get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);
  if ( Auth_token != NULL) {
      fprintf(stderr, "maestro_server(),found .maestro_server_%s file, removing configuration file\n",L2D2.mversion);
      status=unlink(L2D2.auth);
      free(Auth_token);
      free(m5sum);
  }

  /* detach from current terminal */
  if ( fork() > 0 ) {
      fprintf(stdout, "maestro_server(), exiting from parent process \n");
      exit(0);  /* parent exits now */
  }
  
  status=chdir("/");
  if ( status != 0 ) {
      fprintf(stderr, "maestro_server(),could not change directory to / ... exiting");
      exit(1);
  }

  /* get hostname */
  gethostname(hostname, sizeof(hostname));
  strcpy(L2D2.host,hostname);


  /* acquire a socket */
  fserver = get_socket_net();

  /* set buffer sizes for socket */
  set_socket_opt (fserver);
  
  /* bind to a free port, get port number */
  server_port = bind_sock_to_port (fserver);
  
  /* parent writes host:port and exits after disconnecting stdin, stdout, and stderr */
  ip = get_own_ip_address();  /* get own IP address as 32 bit integer */


  unsigned int mypid=getpid();

  /* get into another process group */
  setpgrp(); 

  L2D2.pid  = mypid;
  L2D2.user = passwdEnt->pw_name;
  L2D2.sock = fserver;
  L2D2.port = server_port;
  strcpy(L2D2.ip,ip);

  /* create local common tmp directory where to synchronize locks */
  fprintf(stdout, "Host ip=%s Server port=%d user=%s pid=%d\n", ip, server_port, L2D2.user, L2D2.pid);
  sprintf(buf,"/tmp/%s/%d",L2D2.user,L2D2.pid);
  if ( (ret=r_mkdir(buf,1,stderr)) != 0 ) {
          fprintf(stdout,"Cannot create server working tmpdir directory:%s ... exiting\n",buf);
	  exit(1);
  }

  /* put in global struct */
  if ( (L2D2.tmpdir=malloc((1+strlen(buf))*sizeof(char))) == NULL ) {
          fprintf(stdout,"Cannot malloc for tmpdir ... exiting\n");
	  exit(1);
  }
  strcpy(L2D2.tmpdir,buf);
  

  /* Set authorization file */
  set_Authorization (mypid,hostname,ip,server_port,authorization_file,passwdEnt->pw_name,&L2D2.m5sum);
  Auth_token = get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);
  sscanf(Auth_token, "seqpid=%u seqhost=%s seqip=%s seqport=%d", &pidTken, hostTken, ipTken, &portTken);
  fprintf(stdout, "Set maestro_server pid key to:%u file md5=%s\n",pidTken,L2D2.m5sum);
  snprintf(buf,sizeof(buf),"%s/.suites/.maestro_server_%s",passwdEnt->pw_dir,L2D2.mversion);
  strcpy(L2D2.lock_server_pid_file,buf);
  free(Auth_token);

  /* Parse config file. In which directory should this file reside ? */
  snprintf(buf,sizeof(buf),"%s/.suites/mconfig.xml",passwdEnt->pw_dir);
  ret = ParseXmlConfigFile(buf, &L2D2 );

  /* register handler for sigchild signal */
  struct sigaction chd;
  bzero(&chd, sizeof(chd));
  chd.sa_sigaction = &child_handler;
  chd.sa_flags = SA_SIGINFO;
  sigemptyset (&chd.sa_mask);
  if ( sigaction(SIGCHLD, &chd, 0) != 0 ) fprintf(stderr,"ERROR sigaction for SIGCHLD");

  

  /* fork a Dependency Manager Process (DM)  */
  if ( (L2D2.depProcPid=fork()) == 0 ) {
         /*  this is a child */
         close(fserver);
         DependencyManager (L2D2) ;
         exit(0); /* never reached ! */
  } else if ( L2D2.depProcPid < 0 ) { 
         fprintf(stderr,"forking a Dependency Manager failed");
	 exit(1);
  }

  /* set socket in non blocking mode , This is needed by select sys call */
  int on=1;
  if ( (ret=ioctl(fserver, FIONBIO, (char *)&on)) < 0 ) {
        fprintf(stderr,"setting the socket non blocking failed\n");
        close(fserver);
	exit(-1);
  }
  

  /* Back to parent */
  if ( listen(fserver, 3000) < 0 ) {
         fprintf(stderr, "maestro_server(), Could not listen on port:fserver \n");
         kill(L2D2.depProcPid,9); /* should kill the DMP */
         exit(1);  /* not reachable ??? */

  }

  /* Ok start the Server ie go into main loop */
  maestro_l2d2_main_process_server(fserver);

  return (0);
}

/**
 * shutdown the l2d2server process
 * this gets called when shutting down the server
 */

static void l2d2server_shutdown(pid_t pid , FILE *fp)
{
    int ret;
    char buf[1024];

    fprintf(fp,"Shutting down maestro server ... \n");
    
    fprintf(fp,"Removing .maestro_server file:%s\n",L2D2.auth);
    unlink(L2D2.auth);
   
    snprintf(buf,sizeof(buf),"%s/DM_%d",L2D2.tmpdir,L2D2.depProcPid);
    ret=unlink(buf);
    
    snprintf(buf,sizeof(buf),"%s/EW_%d",L2D2.tmpdir,pid);
    ret=unlink(buf);
    
    snprintf(buf,sizeof(buf),"%s/CTR_%d",L2D2.tmpdir,L2D2.pid);
    ret=unlink(buf);
    
    snprintf(buf,sizeof(buf),"%s/END_TASK_LOCK",L2D2.tmpdir);
    ret=unlink(buf);

    /* TRW_* ??? */

    ret=rmdir(L2D2.tmpdir);
    ret == 0 ?  fprintf(fp,"tmp directory removed ... \n") : fprintf(fp,"tmp directory not removed ... \n") ;

    fclose(fp);

    free(L2D2.tmpdir);
    free(L2D2.m5sum);

    exit(1);
}

/**
 * remove old running mserver
 *
 */
static void l2d2server_remove(FILE *fp) 
{
     fprintf(fp,"Stopping previous dependency process pid=%d...\n",L2D2.depProcPid);
     kill(L2D2.depProcPid,9);
     
     fprintf(fp,"Removing previous maestro_server processes\n");
     exit(1);

}

