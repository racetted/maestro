#define _REENTRANT
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <errno.h>
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

#define SERVER_SOCK_TIMEOUT 5
#define MAX_PROCESS 12

#define TRUE 1
#define FALSE 0


/* forward functions declarations */
extern int  _sleep (double sleep_time);
extern void logZone(int , int , int , char * , ...);
extern int initsem(key_t key, int nsems);
extern char *page_start_dep , *page_end_dep;
extern char *page_start_blocked , *page_end_blocked;

/* globals vars */
unsigned int pidTken = 0;
unsigned int ChildPids[MAX_PROCESS]={0};
int ProcessCount = 0;

/* global default data for l2d2server */
_l2d2server L2D2 = {0,0,0,0,30,24,1,4,'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};

/* variable for Signal handlers */
sig_atomic_t sig_depend_USR1 = 0;
sig_atomic_t sig_depend_USR2 = 0;
sig_atomic_t sig_admin_Terminate = 0;
sig_atomic_t sig_recv = 0;

static void recv_handler ( int notused ) { sig_recv = 1; }
static void sig_child(int sig)
{
     wait(0);
     ProcessCount--;
 }
 

static void sig_depend(int signo) {
    switch (signo) {
        case SIGUSR1: 
                     sig_depend_USR1 = 1;
		     break;
        case SIGUSR2:
                     sig_depend_USR2 = 1;
                     fprintf(stderr,"received sig 2\n");
		     break;
        case SIGHUP:
                     fprintf(stderr,"received SIGHUP\n");
		     break;
        default:
                     fprintf(stderr,"default\n");
		     break;
    }
}

static void sig_admin(int signo, siginfo_t *siginfo, void *context) {
    
    switch (signo) {
        case SIGUSR1: 
	             sig_admin_Terminate = 1; 
		     break;
        case SIGUSR2:
                     fprintf(stderr,"received sig 2\n");
		     break;
        case SIGHUP:
                     fprintf(stderr,"received SIGHUP\n");
		     break;
        default:
                     fprintf(stderr,"default\n");
		     break;
    }
}

/**
   Routine which run as a Process for Verifying and 
   Sumbiting dependencies. This routine is concurrency
   safe meaning that a hcron script could be set to 
   manage dependency files in the same way we do here
   w/o clashing with it. If  needed for Operational
   mode. 
   
*/
void DependencyManager (_l2d2server l2d2 ) {
     
     FILE *fp;
     static DIR *dp = NULL;
     struct dirent *d;
     struct stat st;
     struct _depParameters *depXp=NULL;
     time_t current_epoch;
     unsigned long int epoch_diff;
     int datestamp,nb,LoopNumber;
     char underline[2];
     char buf[1024];
     char cmd[2048];
     char listings[1024];
     char largs[128];
     char extension[256]; /* probably a malloc here */
     char ffilename[512], filename[256], linkname[1024],LoopName[40];
     char rm_key[256];
     char rm_keyFile[1024];
     char Time[40];
     char *pleaf=NULL;
     int Dret,running = 0;
     int r, _ZONE_ = 2;
     

     /* check existence of web stat files  */
     if ( access(l2d2.web_dep,R_OK) == 0 || stat(l2d2.web_dep,&st) == 0 || st.st_size > 0 ) {
    	     fp = fopen(l2d2.web_dep,"w");
    	     fwrite(page_start_dep, 1, strlen(page_start_dep) , fp);
	     fwrite(page_end_dep, 1, strlen(page_end_dep) , fp);
	     fclose(fp);
     }
     /* Not used now 
     if ( access(l2d2.web_blk,R_OK) == 0 || stat(l2d2.web_blk,&st) == 0 || st.st_size > 0 ) {
	     fp = fopen(l2d2.web_blk,"w");
	     fwrite(page_start_blocked, 1, strlen(page_start_blocked) , fp);
	     fwrite(page_end_blocked, 1, strlen(page_end_blocked) , fp);
	     fclose(fp);
     }
     */

     while (running == 0 ) {
         _sleep(l2d2.pollfreq);
	 /* get current epoch */
	 current_epoch = time(NULL);
	 if ( (dp=opendir(l2d2.dependencyPollDir)) == NULL ) { 
	          fprintf(stderr,"Error Could not pen polling directory:%s\n",l2d2.dependencyPollDir);
		  _sleep(3);
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
	                 fprintf(stderr,"DependencyManager(): inter dependency file removed:%s\n",filename);
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
				if ( (depXp=ParseXmlDepFile(linkname)) == NULL ) {
	                                fprintf(stderr,"DependencyManager(): Problem parsing xml file:%s\n",linkname);
				} else {
                                        if ( strcmp(depXp->xpd_slargs,"") != 0 )  
					        snprintf(largs,sizeof(largs),"-l \"%s\"",depXp->xpd_slargs);
                                        else 
					        strcpy(largs,"");

                                        if ( access(depXp->xpd_lock,R_OK) == 0 ) {
                                              get_time(Time,4); 
					      pleaf=(char *) getPathLeaf(depXp->xpd_snode);
					      /* where to put listing :xp/listings/server_host/datestamp/node_container/nonde_name and loop */
					      snprintf(listings,sizeof(listings),"%s/listings/%s/%s%s",depXp->xpd_sname, l2d2.host, depXp->xpd_xpdate, depXp->xpd_container);
					      if ( access(listings,R_OK) != 0 )  Dret=r_mkdir(listings,1);
					      if ( Dret != 1 ) fprintf(stderr,"DM :: could not create dirctory:%s\n",listings);
                                              memset(listings,'\0',sizeof(listings));
					      if ( strcmp(depXp->xpd_slargs,"") != 0 ) {
					              snprintf(listings,sizeof(listings),"%s/listings/%s/%s/%s/%s_%s.submit.mserver.%s.%s",depXp->xpd_sname,l2d2.host, depXp->xpd_xpdate, depXp->xpd_container,pleaf,depXp->xpd_slargs,depXp->xpd_xpdate,Time);
                                              } else {
					              snprintf(listings,sizeof(listings),"%s/listings/%s/%s/%s/%s.submit.mserver.%s.%s",depXp->xpd_sname,l2d2.host, depXp->xpd_xpdate, depXp->xpd_container,pleaf,depXp->xpd_xpdate,Time);
					      }
					      /* build command */
					      snprintf(cmd,sizeof(cmd),"%s; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s; maestro -s submit -n %s %s -f continue >%s 2>&1",l2d2.mshortcut, depXp->xpd_sname, depXp->xpd_xpdate, depXp->xpd_snode, largs, listings);
					      fprintf(stderr,"dependency submit cmd=%s\n",cmd); 
					      /* take account of concurrency here ie multiple dependency managers! */
					      snprintf(buf,sizeof(buf),"%s/.%s",l2d2.dependencyPollDir,filename);
					      Dret=rename(ffilename,buf);
					      if ( Dret == 0 ) {
					             unlink(buf);
					             unlink(linkname);
					             Dret=system(cmd); 
                                              }
					} else {
					      epoch_diff=(int)(current_epoch - atoi(depXp->xpd_regtimepoch))/3600;
					      if ( epoch_diff > L2D2.dependencyTimeOut ) {
					            unlink(linkname);
					            unlink(ffilename);
	                                            fprintf(stderr,"============= Dependency Timed Out ============\n");
	                                            fprintf(stderr,"DependencyManager(): Dependency:%s Timed Out\n",filename);
	                                            fprintf(stderr,"source name:%s\n",depXp->xpd_sname);
	                                            fprintf(stderr,"name       :%s\n",depXp->xpd_name);
	                                            fprintf(stderr,"current_epoch=%u registred_epoch=%d registred_epoch_str=%s epoch_diff=%lu\n",(unsigned int)current_epoch,atoi(depXp->xpd_regtimepoch),depXp->xpd_regtimepoch, epoch_diff);
						    fprintf(stderr,"\n");
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
				}
			     } 
		             break;
                default:
		    	     break;
            }
	 }
	 fwrite(page_end_dep, 1, strlen(page_end_dep) , fp);
	 fclose(fp);
	 closedir(dp);
     }
}

/*
*  Worker Process/Thread : locking, logging 
*  This process will handle a client Session with Multiplexing.
*/
static void l2d2SelectServlet( int listen_sd )
{
  
  fd_set master_set, working_set;
  int buflen,num,Sret=0;
  int i,j,k,count;
  unsigned int pidSent;
  int _ZONE_ = 1, STOP = -1;
  
  char buf[1024],buff[1024];
  char Astring[1024],inode[128], expName[256], expInode[64], hostname[128]; 
  char Bigstr[2048];
  char node[256], signal[256], username[256];
  char Stime[25],Etime[25];
  char m5[40];

  int    max_sd, new_sd;
  int    desc_ready, end_server = FALSE;
  int    rc,close_conn,len;
  char *ts;
  char trans;

  key_t log_key, lock_key, acct_key;
  int semid_log, semid_lock, semid_acct;
  struct sembuf sem_log, sem_lock, sem_acct;
  struct sigaction ssa;

  sem_acct.sem_num = 0;
  sem_acct.sem_op = -1;  /* set to allocate resource */
  sem_acct.sem_flg = SEM_UNDO;
  acct_key=0x0f0f0f;

  ssa.sa_handler = recv_handler;
  ssa.sa_flags = 0;
  sigemptyset(&ssa.sa_mask);

  /* register SIGALRM signal for session control with each client */
  if ( sigaction(SIGALRM,&ssa,NULL) == -1 ) fprintf(stderr,"Error registring signal in SelectServlet \n");

  FD_ZERO(&master_set);
  max_sd = listen_sd;
  FD_SET(listen_sd, &master_set);

  /* Loop waiting for incoming connects or for incoming data   
      on any of the connected sockets. */
  do
  {
      /* Copy the master fd_set over to the working fd_set.     */
      memcpy(&working_set, &master_set, sizeof(master_set));

      /* Call select() with no timeout.   */
      rc = select(max_sd + 1, &working_set, NULL, NULL, NULL);

      /* Check to see if the select call failed.                */
      if (rc < 0) {
         perror("  select() failed");
         break;
      }

      /* Check to see if the 5 minute time out expired. In case of time out 
      if (rc == 0) {
         fprintf(stderr,"  select() timed out.  End program.\n");
         break;
      }
      */

      /* One or more descriptors are readable.  Need to           
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
               /* fprintf(stdout,"Listening socket is readable\n"); */
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
                        perror("  accept() failed");
                        end_server = TRUE;
                     }
                     break;
                  }

                  /* Add the new incoming connection to the  
                     master read set if Client validate */

                  if ( (buflen=recv_socket(new_sd,buf,sizeof(buf),SERVER_SOCK_TIMEOUT)) <= 0 ) {
                         fprintf (stderr,"read in accept loop Timed out\n");
	                 close(new_sd);
                         continue;
                  }

                  buf[buflen > 0 ? buflen : 0] = '\0';
                  Sret=sscanf(buf,"%c %d %s %s %s %s %s %s %s",&trans,&pidSent,expInode,expName,node,signal,hostname,username,m5);
                  if ( Sret != 9 ) {
                             fprintf (stderr,"Got wrong number of parameters at LOGIN, number=%d ,should be 9 others->trans=%c,pidSent=%d expInode=%s \
		             expName=%s node=%s signal=%s hostname=%s username=%s m5=%s\n",Sret,trans,pidSent,expInode,expName,node,signal,hostname,username,m5);
		             close(new_sd);
		             continue;
                  }

                  get_time(Stime,3);
                  if ( pidTken == pidSent && strcmp(m5,L2D2.m5sum) == 0 ) {
	                  send_reply(new_sd,0);
                          logZone(_ZONE_,L2D2.dzone,CONSOLE_OUT,"Open Session with Host:%s AT:%s Exp=%s Node=%s Signal=%s XpInode=%s User=%s M5=%s PID=%d\n",hostname, Stime, expName, node ,signal, expInode, username, m5, getpid());
                  } else {
	                 send_reply(new_sd,1);
	                 close(new_sd);
                         logZone(_ZONE_,L2D2.dzone,CONSOLE_ERR,"Session Refused with Host:%s AT:%s Exp=%s Node=%s Signal=%s XpInode=%s User=%s PID=%d\n",hostname , Stime, expName, node, signal, expInode, username, getpid());
                         continue;
                  }

                  FD_SET(new_sd, &master_set);
                  if (new_sd > max_sd) max_sd = new_sd; /* keep track of the max */
                  
		  /* put the socket in non-blocking mode */
		  if  ( ! (fcntl(new_sd, F_GETFL) & O_NONBLOCK)  ) {
		        if  (fcntl(new_sd, F_SETFL, fcntl(new_sd, F_GETFL) | O_NONBLOCK) < 0) {
		                fprintf(stderr,"Could not put the socket in non-blocking mode\n");
		        }
		  }

                  /* Loop back up and accept another incoming connection */
               } while (new_sd != -1);
            }

            /* This is not the listening socket, therefore an     
               existing connection must be readable             */
            else
            {
               get_time(Stime,3);
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
                         fprintf(stderr," recv failed on i=%d\n",i); 
                         close_conn = TRUE;
                      }
                      break;
                    }

                    /* Check to see if the connection has been    
                       closed by the client  */
                   if (rc == 0) {
                       get_time(Stime,3);
                       fprintf(stdout,"Connection closed:%d Time:%s\n",i,Stime);
                       close_conn = TRUE;
                       break;
                   }

                   /* Data was received  */
                   len = rc;

                   /* work on data      */
                   /* Generate a semaphore key based on xp's Inode for LOGGING same for all process, 
		      we are not using ftok
		      Need a datestamp here */
                   log_key = (getuid() & 0xff ) << 24 | ( atoi(expInode) & 0xffff) << 16;
     
                   /* initialize a LOGGING semaphore for this Xp */
                   sem_log.sem_num = 0;
                   sem_log.sem_op = -1;  /* set to allocate resource */
                   sem_log.sem_flg = SEM_UNDO;
                   if ((semid_log = initsem(log_key, 1)) == -1) {
                        fprintf(stderr,"ERROR initsem lock\n");
	                /* what to do next ??? */
                   }

                   buff[rc > 0 ? rc : 0] = '\0';
                   switch (buff[0]) {
	                       case 'A': /* test existence of lock file on local xp */
	                                Sret = access (&buff[2], R_OK);
			                send_reply(i,Sret);
			                break;
	                       case 'C': /*  create a Lock file on local xp */
	                                Sret = CreateLock ( &buff[2] );
			                send_reply(i,Sret);
			                break;
	                       case 'D': /* mkdir on local xp */
	                                Sret = r_mkdir ( &buff[2] , 1);
			                send_reply(i,Sret);
			                break;
	                       case 'F': /* test existence of lock file on local xp */
	                                Sret = isFileExists ( &buff[2] );
			                send_reply(i,Sret);
			                break;
	                       case 'G': /* glob local xp */
			                Sret = globPath (&buff[2], GLOB_NOSORT, 0 );
			                send_reply(i,Sret);
			                break;
	                       case 'H': /* Remove file on remote xp */
			                break;
	                       case 'I': 
		                        Sret=ParseXmlConfigFile(&buff[2] , &L2D2 );
			                send_reply(i, Sret);
		                        break;
	                       case 'K': /* write Inter user dep file */
		                        Sret = sscanf(&buff[2],"%d %s",&num,Astring);
		                        switch (num) {
		                          case 1:
		                                 strcpy(Bigstr,&buff[4]);
		                                 break;
		                          case 2:
		                                 strcat(&Bigstr[strlen(Bigstr)],&buff[4]);
			                         Sret = writeInterUserDepFile (Bigstr);
			                         break;
			                  default:
			                         break;
			                }

			                /* Sret = writeInterUserDepFile (buff, i); */
			                send_reply(i,Sret);
			                break;
	                       case 'L': /* Log the node under the proper experiment grab the semaphore set created */ 
			                if (semop(semid_log, &sem_log, 1) == -1) {
			                        fprintf(stderr,"ERROR grabing semop log\n");
			                        /* send_reply(i,1); */
				                break;
		                        }
                          

	                                Sret = NodeLogr( &buff[2] , getpid());
			                send_reply(i,Sret);
                          
			  
		                        sem_log.sem_op = 1;  
			                if (semop(semid_log, &sem_log, 1) == -1) {
			                        fprintf(stderr,"ERROR release semop log\n");
			                }
			  
			                break;
	                       case 'N': /* lock */
		                        Sret = lock( &buff[2] , L2D2 ,expName, node ); 
			                send_reply(i,Sret);
			                break;
	                       case 'P': /* unlock */ 
		                        Sret = unlock( &buff[2] , L2D2 ,expName, node ); 
			                send_reply(i,Sret);
		                        break;
	                       case 'R': /* Remove file on local xp */
	                                Sret = removeFile( &buff[2] );
			                send_reply(i,Sret);
			                break;
	                       case 'S': /* Client has sent a Stop, OK Close this connection */
			                Sret=shutdown (i,SHUT_WR);
			                STOP = TRUE;
			                break;
	                       case 'T': /* Touch a Lock file on local xp */
	                                Sret = touch ( &buff[2] );
			                send_reply(i,Sret);
			                break;
	                       case 'W': /* write Node Wait file  under dependent-ON xp */
			                Sret = writeNodeWaitedFile ( &buff[2] );
			                send_reply(i,Sret);
			                break;
	                       case 'X': /* server shutdown */
		                        kill(L2D2.pid,SIGUSR1);
			                Sret=shutdown (i,SHUT_WR);
			                STOP = TRUE;
			                break;
	                       case 'Y': /* server is alive do a uptime */
			                send_reply(i,0);
			                break;
	                       case 'Z': /* download waited file to client */
		                        Sret = SendFile( &buff[2] , i);
		              	        break;
                               default :
	                                fprintf(stderr,"Unrecognized Token>%s< LEN=%d from Experiment=%s node=%s from_host=%s signal=%s \n",buff,rc,expName,node,hostname,signal);
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
                        while (FD_ISSET(max_sd, &master_set) == FALSE)
                           max_sd -= 1;
                     }
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
   pid_t pid, kpid;
   int status,i;
   char *md5sum=NULL, *Auth_token=NULL;
   struct passwd *passwdEnt = getpwuid(getuid());
   char authorization_file[1024];
   struct sigaction adm;


   /* create requested number of process */
  for (i=0 ; i< L2D2.numProcessAtstart; i++ )
  {
           if ( (ChildPids[i] = fork()) == 0 ) 
                l2d2SelectServlet(fserver);
           else if ( ChildPids[i] > 0 )       
                ProcessCount++;
           else                         
                perror("fork() failed");
  }
  
  bzero(&adm, sizeof(adm));
  adm.sa_sigaction = &sig_admin;
  adm.sa_flags = SA_SIGINFO;
  
  if ( sigaction(SIGUSR1, &adm, NULL) != 0 ) fprintf(stderr,"ERROR sigaction for adminstration purposes:USR1");
  if ( sigaction(SIGUSR2, &adm, NULL) != 0 ) fprintf(stderr,"ERROR sigaction for adminstration purposes:USR2");
  if ( sigaction(SIGHUP,  &adm, NULL) != 0 ) fprintf(stderr,"ERROR sigaction for adminstration purposes:HUP");

  /* try alws to adjust the number of servlet */
  for (;;)
  {
	sleep(3);              

        if ( sig_admin_Terminate == 1 ) {
	     close(fserver);
             sleep (2);
             for ( i=0; i < L2D2.numProcessAtstart ; i++ ) {
	           ChildPids[i] == 0 ? : kill (ChildPids[i],9);
		   /* if ( (kpid=waitpid(ChildPids[i], &status, WNOHANG)) > 0 )  fprintf(stderr,"Child %ld terminated\n", kpid); */
             }
	     l2d2server_shutdown();
	}
	/* test to see if user has started a new mserver */
        snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",L2D2.mversion);
	
        Auth_token=get_Authorization (authorization_file, passwdEnt->pw_name, &md5sum);
	if ( Auth_token != NULL ) {
	   if ( strcmp(md5sum,L2D2.m5sum) != 0 ) {
	       fprintf(stderr,"mserver::Error md5sum has changed auth=%s new=%s old=%s ... killing server\n",L2D2.auth,md5sum,L2D2.m5sum);
               for ( i=0; i < L2D2.numProcessAtstart ; i++ ) {
	           ChildPids[i] == 0 ? : kill (ChildPids[i],9);
		   /* if ( (kpid=waitpid(ChildPids[i], &status, WNOHANG)) > 0 )  fprintf(stderr,"Child %ld terminated\n", kpid); */
               }
	       l2d2server_remove();
	   } else free(Auth_token); 
	}
  }

}

int main ( int argc , char * argv[] ) 
{

  int  dep_pid , buflen;
  char buf[1024];
  
  struct stat st;
  char authorization_file[256];
  char *Auth_token=NULL;
  struct passwd *passwdEnt = getpwuid(getuid());

  int status;

  struct _Thread_Data *ptr_ThData;
  pthread_t threadID;
  size_t stacksize;
  
  /* for bucket logs */
  struct _depList *depListHeadPrt=NULL;

  int fserver, client_sock;
  int server_port;
  int returnVal,ret,i;

  char hostname[128];
  char *home = NULL;
  char *ip=NULL;
  char hostTken[20],ipTken[20];
  int  portTken;

  /* get maestro current version and shortcut */
  if (  (L2D2.mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
            fprintf(stderr, "maestro_server(),Could not get maestro current version. do a proper ssmuse \n");
            exit(1);
  }
  
  if (  (L2D2.mshortcut=getenv("SEQ_MAESTRO_SHORTCUT")) == NULL ) {
            fprintf(stderr, "maestro_server(),Could not get maestro current shortcut. do a proper ssmuse \n");
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
         if ( (status=mkdir( buf , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))  != 0 ) { 
                fprintf(stderr, "maestro_server(),Could not create dependencies directory:%s\n",buf);
	 }
	 
  }

  /* get authorization param */
  snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",L2D2.mversion);
  snprintf(L2D2.auth,sizeof(L2D2.auth),"%s/.suites/%s",passwdEnt->pw_dir,authorization_file);
  Auth_token=get_Authorization (authorization_file, passwdEnt->pw_name, &L2D2.m5sum);
  if ( Auth_token != NULL) {
      fprintf(stderr, "maestro_server(),found .maestro_server_%s file, removing configuration file\n",L2D2.mversion);
      status=unlink(L2D2.auth);
      free(Auth_token);Auth_token=NULL;
  }

  /* check for a running dependency process */

  /* disconnect stdin, stdout, and stderr */
   close(0);   /* close and reopen STDOUT  */
   close(2);   /* close and reopen STDERR */

  /* detach from current terminal */
  if ( fork() > 0 ) 
  {
      fprintf(stdout, "maestro_server(), exiting from parent process \n");
      exit(0);  /* parent exits now */
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

  fprintf(stdout, "Host ip=%s Server port= %d\n", ip, server_port);

  unsigned int mypid=getpid();
  /* get into another process group */
  setpgrp(); 

  L2D2.pid = mypid;
  L2D2.user = passwdEnt->pw_name;
  L2D2.sock = fserver;
  L2D2.port = server_port;
  strcpy(L2D2.ip,ip);

  /* create local common tmp directory where to synchronize locks */
  fprintf(stdout,"user=%s pid=%d \n",L2D2.user,L2D2.pid);
  sprintf(buf,"/tmp/%s/%d",L2D2.user,L2D2.pid);
  
  ret=r_mkdir(buf,1);


  if ( (L2D2.tmpdir=malloc((1+strlen(buf))*sizeof(char))) == NULL ) {
          fprintf(stdout,"cannot malloc for tmpdir ... exiting\n");
	  exit(1);
  }
  strcpy(L2D2.tmpdir,buf);
  

  /* Set authorization file */
  set_Authorization (mypid,hostname,ip,server_port,authorization_file,passwdEnt->pw_name,&L2D2.m5sum);
  Auth_token = get_Authorization (authorization_file, passwdEnt->pw_name,&L2D2.m5sum);
  sscanf(Auth_token, "seqpid=%u seqhost=%s seqip=%s seqport=%d", &pidTken, hostTken, ipTken, &portTken);
  fprintf(stdout, "Set maestro_server pid key to:%u file md5=%s\n",pidTken,L2D2.m5sum);
  snprintf(buf,sizeof(buf),"%s/.suites/.maestro_server_%s",passwdEnt->pw_dir,L2D2.mversion);
  strcpy(L2D2.lock_server_pid_file,buf);

  free(Auth_token);

  /* Parse config file. In which directory should this file reside ? */
  snprintf(buf,sizeof(buf),"%s/.suites/mconfig.xml",passwdEnt->pw_dir);
  returnVal = ParseXmlConfigFile(buf, &L2D2 );

  /* fork a Dependency Manager Process (DMP) this is a child */
  if ( (L2D2.depProcPid=fork()) == 0 ) {
         sigset_t  newmask, oldmask,zeromask;
         struct sigaction sa;
         
	 L2D2.depProcPid=getpid();
         close(fserver);
	 umask(0);
         /* reopen stdout stderr */
         freopen(L2D2.dmlog,"w+",stdout); 
         freopen(L2D2.dmlogerr,"w+",stderr); 
         setvbuf(stdout, NULL, _IONBF, 0);
         setvbuf(stderr, NULL, _IONBF, 0);

         sa.sa_handler = sig_depend;
         sa.sa_flags = SA_RESTART;
         sigemptyset(&sa.sa_mask);

         /* register signals */
         if ( sigaction(SIGHUP,&sa,NULL)  != 0 )  fprintf(stderr,"error in sigactions  hup\n");
     
         DependencyManager (L2D2) ;
         exit(0);
  }

  /* set main socket in non blocking mode */
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

  struct sigaction chd;
  bzero(&chd, sizeof(chd));
  chd.sa_handler = sig_child;
  chd.sa_flags = SA_NOCLDSTOP | SA_RESTART;
  sigemptyset (&chd.sa_mask);
  if ( sigaction(SIGCHLD, &chd, 0) != 0 ) fprintf(stderr,"ERROR sigaction for SIGCHLD");
  

 /* register this handler to remove our pidfile at exit */
  atexit(l2d2server_atexit);
      

 /* Ok start the thread Server */
  fprintf(stdout,"starting the main Server Loop \n");

 /* open log files */
  freopen(L2D2.mlog, "w+", stdout);
  freopen(L2D2.mlogerr, "w+", stderr);

 /* set NO BUFFERING for STDOUT and STDERR */
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

 /* go into the main loop */
  maestro_l2d2_main_process_server(fserver);

  return (0);
}

/**
 * on fatal server shutdowns (if the exit() function is called)
 * remove the pid file of this process, terminate DMP. 
 */

static void l2d2server_atexit(void)
{
     fprintf(stderr,"Stopping dependency process ...\n");
     kill(L2D2.depProcPid,9);
}

/**
 * shutdown the l2d2server process
 * this gets called when  shuting down the server
 */

static void l2d2server_shutdown(void)
{
    fprintf(stderr,"Shutting down maestro server ... \n");
    fprintf(stderr,"Removing .maestro_server file:%s\n",L2D2.auth);
    unlink(L2D2.auth);
    exit(1); /* this will cal atexit */
}

/**
 * remove old running mserver
 *
 */
static void l2d2server_remove(void) 
{
     fprintf(stderr,"Removing previous maestro_server processes\n");
     fprintf(stderr,"Stopping previous dependency process ...\n");
     exit(1);

}

