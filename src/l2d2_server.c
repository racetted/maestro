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

#define MAX_PROCESS 8
#define ETERNAL_WORKER_STIMEOUT 3*60    /* 3 minutes */
#define TRANSIENT_WORKER_STIMEOUT 5*60

#define SPAWNING_DELAY_TIME  5 /* seconds */

#define NOTIF_TIME_INTVAL_EW 6*60  
#define NOTIF_TIME_INTVAL_DM 3*60

#define TRUE 1
#define FALSE 0

#define MAX_LOCK_TRY 11

/* forward functions & vars declarations */
extern void logZone(int , int , FILE *fp , char * , ...);
extern int initsem(key_t key, int nsems);
extern char *page_start_dep , *page_end_dep;
extern char *page_start_blocked , *page_end_blocked;

/* globals vars */
unsigned int pidTken = 0;
unsigned int ChildPids[MAX_PROCESS]={0};

/* global default data for l2d2server */
_l2d2server L2D2 = {0,0,0,0,30,24,1,4,20,'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};

/* variable for Signal handlers */
volatile sig_atomic_t ProcessCount = 0;
volatile sig_atomic_t sig_depend = 0;
volatile sig_atomic_t sig_admin_Terminate = 0;
volatile sig_atomic_t sig_admin_AddWorker = 0;
volatile sig_atomic_t sig_recv = 0;
volatile sig_atomic_t sig_child = 0;

/* signal handler for alarm */
static void recv_handler ( int notused ) { sig_recv = 1; }

/* Signal handler for Exiting childs  */
static void child_handler(int signo, siginfo_t *siginfo, void *context)
{
     wait(0);
     sig_child=1;
}
 
/* Signal handler for Dependency Manager */
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
     static DIR *dp = NULL;
     struct dirent *d;
     struct stat st;
     sigset_t  newmask, oldmask,zeromask;
     struct sigaction sa;
     struct _depParameters *depXp=NULL;
     time_t current_epoch,start_epoch;
     unsigned long int epoch_diff;
     double diff;
     int datestamp,nb,LoopNumber;
     char underline[2];
     char buf[1024];
     char cmd[2048];
     char listings[1024];
     char largs[128];
     char extension[256]; /* probably a malloc here */
     char ffilename[512], filename[256], linkname[1024],LoopName[64];
     char rm_key[256];
     char rm_keyFile[1024];
     char Time[40];
     char *pleaf=NULL;
     int ret,running = 0;
     int r, _ZONE_ = 2;
     
         
     l2d2.depProcPid=getpid();
     umask(0);


     /* open log files */
     get_time(Time,1);
     snprintf(buf,sizeof(buf),"%s_%.8s_%d",l2d2.dmlog,Time,l2d2.depProcPid);
     if ( (dmlg=fopen(buf,"w+")) == NULL ) {
            fprintf(stdout,"Dependency Manager: Could not open dmlog stream\n");
     }
    
     /* close 0,1,2 streams */
     close(0);
     close(1);
     close(2);
     
     /* streams will be unbuffered */
     setvbuf(dmlg, NULL, _IONBF, 0);

     sa.sa_sigaction = &depMang_handler;
     sa.sa_flags = SA_SIGINFO;
     sigemptyset(&sa.sa_mask);

     /* register signals Note : they are not used for the moment */
     if ( sigaction(SIGUSR1,&sa,NULL)  != 0 )  fprintf(dmlg,"error in sigactions  SIGUSR1\n");
     if ( sigaction(SIGUSR2,&sa,NULL)  != 0 )  fprintf(dmlg,"error in sigactions  SIGUSR2\n");
     

     fprintf(dmlg,"Dependency Manager starting ... pid=%d\n",l2d2.depProcPid);

     /* check existence of web stat files  */
     if ( access(l2d2.web_dep,R_OK) == 0 || stat(l2d2.web_dep,&st) == 0 || st.st_size > 0 ) {
    	     fp = fopen(l2d2.web_dep,"w");
    	     fwrite(page_start_dep, 1, strlen(page_start_dep) , fp);
	     fwrite(page_end_dep, 1, strlen(page_end_dep) , fp);
	     fclose(fp);
     }
    
     /* for heartbeat */
     time(&start_epoch);
     snprintf(buf,sizeof(buf),"%s/DM_%d",l2d2.tmpdir,getpid());
     if ( (ft=fopen(buf,"w+")) != NULL ) {
             fclose(ft);
     }

     while (running == 0 ) {
         sleep(l2d2.pollfreq);
	 /* get current epoch */
	 time(&current_epoch);
	 if ( (dp=opendir(l2d2.dependencyPollDir)) == NULL ) { 
	          fprintf(dmlg,"Error Could not open polling directory:%s\n",l2d2.dependencyPollDir);
		  sleep(2);
	          continue ; 
	 }  
	 fp = fopen(l2d2.web_dep , "w");
	 fwrite(page_start_dep, 1, strlen(page_start_dep) , fp);
	 while ( d=readdir(dp))
	 {
	    memset(buf,'\0',sizeof(buf));
	    memset(cmd,'\0',sizeof(cmd));
	    memset(listings,'\0',sizeof(listings));
	    memset(linkname,'\0',sizeof(linkname));
	    memset(LoopName,'\0',sizeof(LoopName));

	    snprintf(ffilename,sizeof(ffilename),"%s/%s",l2d2.dependencyPollDir,d->d_name);
	    snprintf(filename,sizeof(filename),"%s",d->d_name);

            /* stat will stat the file pointed to ... lstat will stat the link itself */
	    if ( stat(ffilename,&st) != 0 ) {
	       get_time(Time,1);
	       fprintf(dmlg,"DependencyManager(): %s inter-dependency file not there, removing link ... \n",Time,filename);
			 unlink(ffilename); 
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
			
				   linkname[r] = '\0';
				   if ( (depXp=ParseXmlDepFile( linkname, dmlg, dmlg )) == NULL ) {
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

                  if ( access(depXp->xpd_lock,R_OK) == 0 ) {
                     get_time(Time,4); 
					      pleaf=(char *) getPathLeaf(depXp->xpd_snode);
					      /* where to put listing :xp/listings/server_host/datestamp/node_container/nonde_name and loop */
					      snprintf(listings,sizeof(listings),"%s/listings/%s%s",depXp->xpd_sname, l2d2.host, depXp->xpd_container);
					      if ( access(listings,R_OK) != 0 )  ret=r_mkdir(listings,1);
					      if ( ret != 0 ) fprintf(dmlg,"DM:: Could not create directory:%s\n",listings);
                     memset(listings,'\0',sizeof(listings));
					      if ( strcmp(depXp->xpd_slargs,"") != 0 ) {
					              snprintf(listings,sizeof(listings),"%s/listings/%s/%s/%s_%s.submit.mserver.%s.%s",depXp->xpd_sname,l2d2.host, depXp->xpd_container,pleaf,depXp->xpd_slargs,depXp->xpd_sxpdate,Time);
                     } else {
					              snprintf(listings,sizeof(listings),"%s/listings/%s/%s/%s.submit.mserver.%s.%s",depXp->xpd_sname,l2d2.host, depXp->xpd_container,pleaf,depXp->xpd_sxpdate,Time);
					      }
					      /* build command */
					      snprintf(cmd,sizeof(cmd),"%s; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; maestro -s submit -n %s %s -f continue >%s 2>&1",l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_sxpdate, depXp->xpd_snode, largs, listings);
					      fprintf(dmlg,"dependency submit cmd=%s\n",cmd); 
					      /* take account of concurrency here ie multiple dependency managers! */
					      snprintf(buf,sizeof(buf),"%s/.%s",l2d2.dependencyPollDir,filename);
					      ret=rename(ffilename,buf);
					      if ( ret == 0 ) {
					             unlink(buf);
					             unlink(linkname);
					             ret=system(cmd); 
                     }
					} else {
					      epoch_diff=(int)(current_epoch - atoi(depXp->xpd_regtimepoch))/3600; 
					      if ( epoch_diff > l2d2.dependencyTimeOut ) {
					         unlink(linkname);
					         unlink(ffilename);
	                     fprintf(dmlg,"============= Dependency Timed Out ============\n");
	                     fprintf(dmlg,"DependencyManager(): Dependency:%s Timed Out\n",filename);
	                     fprintf(dmlg,"source name:%s\n",depXp->xpd_sname);
	                     fprintf(dmlg,"name       :%s\n",depXp->xpd_name);
	                     fprintf(dmlg,"current_epoch=%d registred_epoch=%d registred_epoch_str=%s epoch_diff=%d\n",current_epoch,atoi(depXp->xpd_regtimepoch),depXp->xpd_regtimepoch, epoch_diff);
						      fprintf(dmlg,"\n");
					      }
					}
					/* register dependency in web page */
					snprintf(buf,sizeof(buf),"<tr><td>%s</td>",depXp->xpd_regtimedate);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<td><table><tr><td><font color=\"red\">SRC_EXP</font></td><td>%s</td>",depXp->xpd_sname);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">SRC_NODE</font></td><td>%s</td>",depXp->xpd_snode);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">DEP_ON_EXP</font></td><td>%s</td>",depXp->xpd_name);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">Key</font></td><td>%s_%s</td>",depXp->xpd_xpdate,depXp->xpd_key);
					fwrite(buf, 1 ,strlen(buf), fp );
					snprintf(buf,sizeof(buf),"<tr><td><font color=\"red\">LOCK</font></td><td>%s</td></table></td>",depXp->xpd_lock);
					fwrite(buf, 1 ,strlen(buf), fp );
					free(depXp);depXp=NULL;
				}
			     } 
		             break;
                default:
		    	     break;
            } /* end switch */
	 } /* end while readdir */
	 
	 /* heart beat :: each 2 minutes */
	 if ( (diff=difftime(current_epoch,start_epoch)) >= 120 ) {
	         start_epoch=current_epoch;
	         snprintf(buf,sizeof(buf),"%s/DM_%d",l2d2.tmpdir,getpid());
		 ret=utime(buf,NULL);
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
  char mlogName[1024];
  char node[256], signal[256], username[256];
  char Stime[25],Etime[25], tlog[10];
  char m5[40];

  _l2d2client l2d2client[1024]; /* the select can take 1024 max  */

  int  max_sd, new_sd;
  int  desc_ready, end_server = FALSE;
  int  rc, close_conn, len, got_lock;
  int  ceiling=0;
  int  fd;           
  int sent;            
  struct stat stat_buf; 
  char *ts;
  char trans;

  key_t log_key;
 
  struct sigaction ssa;
  struct timeval timeout;  /* Timeout for select */
  struct flock nlock,ilock; /* for Logging we are using fnctl() */
  glob_t g_AliveFiles;
  int g_result;
  time_t sig_sent,now;
  double delay;

  /* open log files */
  get_time(Stime,1);
  snprintf(tlog,sizeof(tlog),"%.8s",Stime);
  snprintf(buf,sizeof(buf),"%s_%.8s_%d",L2D2.mlog,Stime,getpid());
  if ( (mlog=fopen(buf,"w+")) == NULL ) {
            fprintf(stdout,"Worker: Could not open mlog stream\n");
  }

  memset(buf,'\0',sizeof(buf));

  /* for heartbeat */ 
  if ( tworker == ETERNAL ) {
        snprintf(buf,sizeof(buf),"%s/EW_%d",L2D2.tmpdir,getpid());
        if ( (fp=fopen(buf,"w+")) != NULL ) {
             fclose(fp);
        }
  } else { /* need this for logging */
	snprintf(buf,sizeof(buf),"%s/TRW_%d",L2D2.tmpdir,getpid());
        if ( (fp=fopen(buf,"w+")) != NULL ) {
             fclose(fp);
        }
  }
  
  /* streams will be unbuffered */
  setvbuf(mlog, NULL, _IONBF, 0);

  /* register SIGALRM signal for session control with each client, 
  This could be used in future
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

  /* fix timeout for workers */
  if ( tworker == ETERNAL ) {
          SelecTimeOut=ETERNAL_WORKER_STIMEOUT;
  } else {
          SelecTimeOut=TRANSIENT_WORKER_STIMEOUT;
  }

  /* Loop waiting for incoming connects or for incoming data   
      on any of the connected sockets. */
  do
  {
      /* Copy the master fd_set over to the working fd_set.     */
      memcpy(&working_set, &master_set, sizeof(master_set));
  
      /* set timeout for select SELECT_TIMEOUT minutes */
      timeout.tv_sec = SelecTimeOut ;
      timeout.tv_usec = 0;

      /* Call select() with timeout */
      rc = select(max_sd + 1, &working_set, NULL, NULL, &timeout);

      /* Check to see if the select call failed.                */
      if (rc < 0) {
         perror("  select() failed");
         break;
      }

      /* Check to see if select call timed out ... yes -> do other things */ 
      if (rc == 0) {
          if ( tworker != ETERNAL ) {
	     snprintf(buf,sizeof(buf),"%s/TRW_%d",L2D2.tmpdir,getpid());
             ret=unlink(buf);
	     get_time(Stime,3);
	     fprintf(mlog,"TRansient worker exits pid=%lu at:%s\n", (unsigned long) getpid(), Stime );
	     fclose(mlog);
	     exit(0);
          } else {
	     /* heart beat */
	     snprintf(buf,sizeof(buf),"%s/EW_%d",L2D2.tmpdir,getpid());
             ret=utime(buf,NULL); 
	     /* cascade log file if time to do so */
	     get_time(Stime,1);
	     if ( strncmp(tlog,Stime,8) != 0 ) {
	            fprintf(mlog,"Cascading log file at:%s\n", Stime);
	            snprintf(tlog,sizeof(tlog),"%.8s",Stime);
	            /* close old mlog file */
	            fclose(mlog);
	            snprintf(buf,sizeof(buf),"%s_%.8s_%d",L2D2.mlog,Stime,getpid());
	            if ( (mlog=fopen(buf,"w+")) == NULL ) {
	                      fprintf(stdout,"Worker: Could not open mlog stream\n");
	            }
	            setvbuf(mlog, NULL, _IONBF, 0);
	     }
	  }
      }
      

      /* One or more descriptors are readable. Need to           
         determine which ones they are.                         */
      desc_ready = rc;
      for (i=0; i <= max_sd  &&  desc_ready > 0; ++i)
      {
         /* Check to see if this descriptor is ready            */
         if (FD_ISSET(i, &working_set))
         {
            /* A descriptor was found that was readable - one     
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
                        perror(" accept() failed");
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
		  
		  /* put the socket in non-blocking mode */
		  if  ( ! (fcntl(new_sd, F_GETFL) & O_NONBLOCK)  ) {
		        if  (fcntl(new_sd, F_SETFL, fcntl(new_sd, F_GETFL) | O_NONBLOCK) < 0) {
		            fprintf(mlog,"Could not put the socket in non-blocking mode\n");
		        }
		  }
                  
		  FD_SET(new_sd, &master_set);
                  if (new_sd > max_sd) max_sd = new_sd; /* keep track of the max */
		  
		  ceiling++;
		  if ( ceiling >= L2D2.maxClientPerProcess ) {
			time(&now);
			/* wait SPAWNING_DELAY_TIME secondes btw sending of signals to main server,this is for signal flood control */
			if ( (delay=difftime(now,sig_sent)) >= SPAWNING_DELAY_TIME ) {
		               get_time(Stime,3);
			       if ( tworker == ETERNAL ) {
		                     fprintf(mlog,"Signal sent at:%s to main server to add a worker\n",Stime);
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
                         perror("recv() failed");
                         get_time(Stime,3);
                         logZone(L2D2.dzone,L2D2.dzone,mlog,"recv failed with Host:%s AT:%s Exp=%s Node:%s Signal:%s\n",l2d2client[i].host, Stime, l2d2client[i].xp, l2d2client[i].node, l2d2client[i].signal);
                         close_conn = TRUE;
                      }
                      break;
                    }

                    /* Check to see if the connection has been closed by the client  */
                   if (rc == 0) {
                       get_time(Stime,3);
                       logZone(_ZONE_,L2D2.dzone,mlog,"%s Closed AT:%s TransNumber=%u\n",l2d2client[i].Open_str,Stime,l2d2client[i].trans);
                       close_conn = TRUE;
                       break;
                   }

                   /* Data was received  */
                   len = rc;
                   
                   /* work on data      */
                   buff[rc > 0 ? rc : 0] = '\0';
                   switch (buff[0]) {
	                       case 'A': /* test existence of lock file on local xp */
	                                ret = access (&buff[2], R_OK);
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'C': /*  create a Lock file on local xp */
	                                ret = CreateLock ( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'D': /* mkdir on local xp */
	                                ret = r_mkdir ( &buff[2] , 1);
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'F': /* test existence of lock file on local xp */
	                                ret = isFileExists ( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'G': /* glob local xp */
			                ret = globPath (&buff[2], GLOB_NOSORT, 0 );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'I': 
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
                                                 fprintf (mlog,"Got wrong number of parameters at LOGIN, number=%d instead of 8 buff=>%s<\n",buff);
	                                         /* close(i);same comment as for the S case below */
			                         ret=shutdown(i,SHUT_WR);
					         ceiling--;
                                                 snprintf(l2d2client[i].Open_str,sizeof(l2d2client[i].Open_str),"Session Refused with Host:%s AT:%s Exp=%s Node=%s Signal=%s ... Wrong number of arguments ",hostname , Stime, expName, node, signal);
                                         } else if ( pidTken == pidSent && strcmp(m5,L2D2.m5sum) == 0 ) {
	                                         send_reply(i,0);
						 snprintf(l2d2client[i].Open_str,sizeof(l2d2client[i].Open_str),"Open Session Host:%s AT:%s Exp=%s Node=%s Signal=%s pid=%d NumberOfConn=%d ",hostname, Stime, expName, node ,signal, pidSent, ceiling);
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
	                       case 'K': /* write Inter user dep file */
                                        get_time(Stime,3);
		                        ret = sscanf(&buff[2],"%d %s",&num,Astring);
		                        switch (num) {
		                          case 1:
		                                 strcpy(Bigstr,&buff[4]);
		                                 break;
		                          case 2:
		                                 strcat(&Bigstr[strlen(Bigstr)],&buff[4]);
			                         ret = writeInterUserDepFile (Bigstr, mlog );
			                         break;
			                  default:
			                         break;
			                }
                                         

			                /* ret = writeInterUserDepFile_svr_2 (&buff[2], i, mlog); */ 
			                send_reply(i,ret); 
					l2d2client[i].trans++;
			                break;
	                       case 'L':/* Log the node under the proper experiment grab the semaphore set created */ 
                                        /* Generate a  key based on xp's Inode for LOGGING. Same for all process ,we are not using ftok
		                            Note : Need a datestamp here ? */

                                        memset(buf,'\0',sizeof(buf));
                                        /* regexp for checking existence of TRansient Workers */ 
	                                snprintf(buf,sizeof(buf),"%s/TRW_*",L2D2.tmpdir);
					g_result = glob(buf, 0, NULL, &g_AliveFiles);

                                        if ( tworker == TRANSIENT || ( g_result == 0 && g_AliveFiles.gl_pathc > 0 ) ) {

					       globfree(&g_AliveFiles);
                                               log_key = (getuid() & 0xff ) << 24 | ( atoi(expInode) & 0xffff) << 16;
                                               memset(buf,'\0',sizeof(buf));
	                                       snprintf(buf,sizeof(buf),"%s/NodeLogLock_0x%x",L2D2.tmpdir,log_key);
					       /* Open a file descriptor to the file.  what if +1 open at the same time  ? */
					       if ( (fd = open (buf, O_WRONLY|O_CREAT, S_IRWXU)) < 0 ) {
                                                      fprintf(mlog,"ERROR Opening NodeLogLock file\n");
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
					              fprintf(mlog,"ERROR Cannot get lock, owned by pid:%d exp=%s node=%s\n", ilock.l_pid, expName, node);
                                                      close(fd);
			                              send_reply(i,1);
					              break;
					       }

	                                       ret = NodeLogr( &buff[2] , getpid(), mlog );
			                       send_reply(i,ret);
                         
                                               nlock.l_type = F_UNLCK;
                                               if (fcntl(fd, F_SETLK, &nlock) == -1) {
                                                      fprintf(mlog,"ERROR unlocking NodeLogLock file\n");
					       }
                                               close(fd);
                                        } else {  
					       /* one Serial worker ie Eternel worker */ 
	                                       ret = NodeLogr( &buff[2] , getpid(), mlog );
			                       send_reply(i,ret);
					}
			  
					l2d2client[i].trans++;
			                break;
	                       case 'N': /* lock */
		                        ret = lock( &buff[2] , L2D2 ,expName, node, mlog ); 
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'P': /* unlock */ 
		                        ret = unlock( &buff[2] , L2D2 ,expName, node, mlog ); 
			                send_reply(i,ret);
					l2d2client[i].trans++;
		                        break;
	                       case 'R': /* Remove file on local xp */
	                                ret = removeFile( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'S': /* Client has sent a Stop, OK Close this connection 
			                Note: if we use close(i) here , the rcv() call will fail, we need to close
					the server write side and let the client close his side, the server will then
					report a closed socket */
			                ret=shutdown(i,SHUT_WR);
			                STOP = TRUE;
			                break;
	                       case 'T': /* Touch a Lock file on local xp */
	                                ret = touch ( &buff[2] );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'W': /* write Node Wait file  under dependent-ON xp */
                                        get_time(Stime,3);
			                ret = writeNodeWaitedFile ( &buff[2] , mlog );
			                send_reply(i,ret);
					l2d2client[i].trans++;
			                break;
	                       case 'X': /* server shutdown */
		                        kill(L2D2.pid,SIGUSR1);
			                ret=shutdown(i,SHUT_WR);
			                STOP = TRUE;
			                break;
	                       case 'Y': /* server is alive do a uptime */
			                send_reply(i,0);
			                break;
	                       case 'Z':/* download waited file to client */
		                        ret = SendFile( &buff[2] , i, mlog ); 
					l2d2client[i].trans++;
		              	        break;
                               default :
	                                fprintf(mlog,"Unrecognized Token>%s< from Host=%s Exp=%s node=%s signal=%s \n",buff,l2d2client[i].host,l2d2client[i].xp,l2d2client[i].node,l2d2client[i].signal);
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
                  based on the bits that are still turned on in   
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
      if (FD_ISSET(i, &master_set))
         close(i);
   }
}


void maestro_l2d2_main_process_server (int fserver)
{
  FILE *smlog;
  pid_t pid_eworker, kpid;  /* pid_eworker : pid of eternal worker */
  int ret,status,i=0,j;
  char *m5sum=NULL, *Auth_token=NULL;
  struct passwd *passwdEnt = getpwuid(getuid());
  char authorization_file[1024],filename[1024];
  time_t current_epoch;
  struct sigaction adm;
  struct stat st;
  double diff_t;
  char Time[40];

  bzero(&adm, sizeof(adm));
  adm.sa_sigaction = &sig_admin;
  adm.sa_flags = SA_SIGINFO;
  
  /* open log files */
  snprintf(filename,sizeof(filename),"%s_%d",L2D2.mlog,L2D2.pid);
  if ( (smlog=fopen(filename,"w+")) == NULL ) {
            fprintf(stderr,"Main server: Could not open mlog stream\n");
  } else {
            fprintf(smlog,"Main server: starting .... PID=%d\n",L2D2.pid);
  }
  
  /* close 0,1,2 streams */
  close(0);
  close(1);
  close(2);

  /* streams will be unbuffered */
  setvbuf(smlog, NULL, _IONBF, 0);

  /* register signal for admin purposes */
  if ( sigaction(SIGUSR1, &adm, NULL) != 0 ) fprintf(smlog,"ERROR in sigaction signal :USR1");
  if ( sigaction(SIGUSR2, &adm, NULL) != 0 ) fprintf(smlog,"ERROR in sigaction signal :USR2");
  if ( sigaction(SIGHUP,  &adm, NULL) != 0 ) fprintf(smlog,"ERROR in sigaction signal :HUP");

  /* Generate the Eternal worker */
  if ( (pid_eworker = fork()) == 0 ) { 
      fclose(smlog);
      l2d2SelectServlet( fserver , ETERNAL );
  } else if ( pid_eworker > 0 ) {      
      ProcessCount++;
  } else                         
      perror("fork() Eternal Worker failed");

  /* go into the main admin loop */
  for (;;)
  {
	if ( sig_admin_AddWorker == 1 ) {
	      sig_admin_AddWorker=0;
	      get_time(Time,3);
	      fprintf(smlog,"Received signal USR2 at:%s from worker ProcessCount=%d\n",Time,ProcessCount);
              if ( ProcessCount < L2D2.maxNumOfProcess ) {
                 if ( (ChildPids[i] = fork()) == 0 ) { 
		      fclose(smlog);
                      l2d2SelectServlet( fserver , TRANSIENT );
                 } else if ( ChildPids[i] > 0 ) {        
                      ProcessCount++;
	              fprintf(smlog,"One worker generated with pid=%d at:%s\n",ChildPids[i],Time);
		      i++;
                 } else                      
                      perror("fork() Worker failed");
              } else {
	            fprintf(smlog,"cannot add more worker, already reached maximum number:%d\n",ProcessCount);
              }
	}
	

        /* sigchild :: a Child has exited ... dont know who is */
	if ( sig_child == 1 ) {
	     sig_child=0;
	     /* Check Eternal worker */
	     if ( (ret=kill(pid_eworker,0)) != 0 ) {
	           fprintf(smlog,"Eternal Worker is dead (pid=%d) ... trying to spawn a new one ret=%d\n",pid_eworker,ret);
                   ProcessCount--;
		   /* glitch here with signals */
		   if ( ProcessCount < 0 ) ProcessCount=0;

                   if ( (pid_eworker = fork()) == 0 ) { 
		              fclose(smlog);
                              l2d2SelectServlet( fserver , ETERNAL );
                   } else if ( pid_eworker > 0 ) {      
                              ProcessCount++;
                   } else                         
                          perror("fork() Eternal Worker failed");
	     } 
	     
	     /* Check Dependency Manager */
	     if ( (ret=kill(L2D2.depProcPid,0)) != 0 ) {
	           fprintf(smlog,"Dependency Manager is dead (pid=%d) ... trying to spawn a new one ret=%d\n",L2D2.depProcPid,ret);
                   if ( (L2D2.depProcPid=fork()) == 0 ) {
                         /*  this is a child, Note: will inherite signals */
                          close(fserver);
		          fclose(smlog);
                          DependencyManager (L2D2) ;
                          exit(0); /* never reached ! */
                   } else if ( L2D2.depProcPid > 0 ) {
	                  /* in parent do nothing */
	           } else 
                          perror("fork() Dependenct Manager failed");
             }
	     /* Check Transient worker, only for decrementing the ProcessCount variable */
             for ( j=0; j < i ; j++ ) {
                 if ( ChildPids[j] != 0 && (ret=kill(ChildPids[j],0)) != 0 ) {
	                     fprintf(smlog,"Worker process pid:%u has terminated\n",ChildPids[j]);
                             ChildPids[j]=0;
                             ProcessCount--;
			     /* glitch here with signals */
			     if ( ProcessCount < 0 ) ProcessCount=0;
		 }
	     }
	}
        
	/* hearbeat */
	current_epoch = time(NULL);
	/* must examine if Eternal worker or Dependency Manager has delivered this signal */
	snprintf(filename,sizeof(filename),"%s/EW_%d",L2D2.tmpdir,pid_eworker);
	if ( stat(filename,&st) == 0 ) {
	        if ( (diff_t=abs(difftime(current_epoch,st.st_mtime))) > NOTIF_TIME_INTVAL_EW ) {
	              if ( (ret=kill(pid_eworker,0)) != 0 ) {
	                   fprintf(smlog,"Eternal Worker is dead (no heartbeat) ...  spawning a new one\n");
		     }
	        } 
         } else if ( (ret=kill(pid_eworker,0)) != 0 ) {
	                fprintf(smlog,"Eternal worker is dead (no heartbeat) ... spawning a new one\n");
	        }

	 snprintf(filename,sizeof(filename),"%s/DM_%d",L2D2.tmpdir,L2D2.depProcPid);
	 if ( stat(filename,&st) == 0 ) {
	            if ( (diff_t=abs(difftime(current_epoch,st.st_mtime))) > NOTIF_TIME_INTVAL_DM ) {
	                if ( (ret=kill(L2D2.depProcPid,0)) != 0 ) {
	                   fprintf(smlog,"Dependency manager is dead (no heartbeat) ...  spawning a new one\n");
			}
                    } 
         } else if ( (ret=kill(L2D2.depProcPid,0)) != 0 ) {
	                fprintf(smlog,"Dependency manager is dead (no heartbeat) ... spawning a new one\n");
	 }

        /* shut down server */
        if ( sig_admin_Terminate == 1 ) {
	     close(fserver);
             sleep (2);
             for ( j=0; j < i ; j++ ) {
	           ChildPids[j] == 0 ? : kill (ChildPids[j],9);
             }
	     kill(pid_eworker,9);
             kill(L2D2.depProcPid,9);
	     l2d2server_shutdown(pid_eworker,smlog);
	}

	/* test to see if user has started a new mserver */
        snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",L2D2.mversion);
        Auth_token=get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);
	if ( Auth_token != NULL ) {
	   if ( strcmp(m5sum,L2D2.m5sum) != 0 ) {
	       fprintf(smlog,"mserver::Error md5sum has changed File=%s new=%s old=%s ... killing server\n",L2D2.auth,m5sum,L2D2.m5sum);
               for ( j=0; j < i ; j++ ) {
	           ChildPids[j] == 0 ? : kill (ChildPids[j],9);
               }
	       kill(pid_eworker,9);
	       l2d2server_remove(smlog);
	   } else {
	           free(Auth_token); 
	           free(m5sum); 
	   }
	}
        
	/* wait on Un-caught childs  */
	if ( (kpid=waitpid(-1,NULL,WNOHANG)) > 0 ) { 
	       fprintf(smlog,"mserver::Waited on child with pid:%lu\n",(unsigned long ) kpid);
        }

	sleep(5);  /* yield  Note : what about server receiving signal here ? */              
  }

}

int main ( int argc , char * argv[] ) 
{

  char buf[1024];
  
  struct stat st;
  char authorization_file[256];
  char hostname[128], hostTken[20], ipTken[20];
  char *Auth_token=NULL, *m5sum=NULL;
  struct passwd *passwdEnt = getpwuid(getuid());
  
  int fserver;
  int server_port;
  int portTken;
  int ret,status,i;

  char *home = NULL;
  char *ip=NULL;

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
         if ( (status=r_mkdir(buf , 1))  != 0 ) { 
                fprintf(stderr, "maestro_server(),Could not create dependencies directory:%s\n",buf);
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

  /* do we have to check for a running dependency process here ? */


  /* detach from current terminal */
  if ( fork() > 0 ) 
  {
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

  L2D2.pid = mypid;
  L2D2.user = passwdEnt->pw_name;
  L2D2.sock = fserver;
  L2D2.port = server_port;
  strcpy(L2D2.ip,ip);

  /* create local common tmp directory where to synchronize locks */
  fprintf(stdout, "Host ip=%s Server port=%d user=%s pid=%d\n", ip, server_port, L2D2.user, L2D2.pid);
  sprintf(buf,"/tmp/%s/%d",L2D2.user,L2D2.pid);
  
  ret=r_mkdir(buf,1);


  if ( (L2D2.tmpdir=malloc((1+strlen(buf))*sizeof(char))) == NULL ) {
          fprintf(stdout,"cannot malloc for tmpdir ... exiting\n");
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
         perror("fork failed");
	 exit(1);
  }

  /* set socket in non blocking mode , This is needed by select sys call */
  int on=1;
  if ( (ret=ioctl(fserver, FIONBIO, (char *)&on)) < 0 ) {
        perror("ioctl() failed");
        close(fserver);
	exit(-1);
  }
  

  /* Back to parent */
  if ( listen(fserver, 3000) < 0 ) {
         fprintf(stderr, "maestro_server(), Could not listen on port:fserver \n");
         kill(L2D2.depProcPid,9); /* should kill the DMP */
         exit(1);  /* not reachable ??? */

  }

 /* reopen streams  
  freopen(L2D2.mlog, "w+", stdout);
  freopen(L2D2.mlogerr, "w+", stderr);

  set NO BUFFERING for STDOUT and STDERR 
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  */

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
    
    snprintf(buf,sizeof(buf),"%s/end_task_lock",L2D2.tmpdir);
    ret=unlink(buf);

    ret=rmdir(L2D2.tmpdir);
    ret == 0 ?  fprintf(fp,"tmp directory removed ... \n") : fprintf(fp,"tmp directory not removed ... \n") ;

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

