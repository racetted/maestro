/* l2d2_admin.c - Server administrative tool, part of the Maestro sequencer software package.
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
#include <fcntl.h>
#include <signal.h>
#include <glob.h>
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
#include "l2d2_roxml.h"
#include "l2d2_server.h"
#include "l2d2_lists.h"

extern char *get_Authorization(char *, char * ,char **);
extern dpnode *getDependencyFiles(char *ddep , char *xp, FILE *fp , const char *deptype);

/* The name of this program.  */
const  char* program_name;

typedef enum {
      SHUT_DOWN_SERVER,
      LIST_DEPENDENCIES,
      REMOVE_DEPENDENCIES,
      CHANGE_TIME_STEP,
      CHANGE_DEBUG_ZONE,
      RELOAD_CONFIG,
      IS_ALIVE,
      NONE
} ServerActions;

typedef enum {
      DEP_KEY,
      DEP_EXP,
      DPE_EXP,
      DEP_ALL
} DepOption;

/* pointer to linked list of dependencies */
dpnode *PRT_listdep=NULL;


char typeofFile(mode_t mode);
static void alarm_handler() { /* nothing */ }
/* static int globerr(const char *path, int eerrno); */

/* 
 * Prints usage information for this program to STREAM (typically
 * stdout or stderr), and exit the program with EXIT_CODE.  Does not
 * return.
*/

void print_usage (FILE* stream , int exit_code)
{
  fprintf (stream, "Usage:  %s options [ args ... ]\n", program_name);
  fprintf (stream,
           "  -h                      Display usage information.\n"
           "  -c   config.xml         re-load maestro server configuration file.\n"
           "  -l   xp_name            xp_name: list current registered dependencies for this xp_name\n"
	   "                          none: list current registered dependencies of defined SEQ_EXP_HOME\n" 
	   " \n"
           "  -r   key|xp_name|all    key: remove current registered dependency identified by the given key\n"
	   "                          xp_name: remove current registered dependencies for this xp_name\n"
	   "                          all: remove all current registered dependencies for this user\n"
	   "                          none: remove current registered dependencies of defined SEQ_EXP_HOME\n"
           "  -t   xp_name            list xp who are depending on this xp_name \n"
           "                          none: list xp who are depending on defined SEQ_EXP_HOME \n"
           "  -s                      Shutdown maestro server \n" 
           "  -i                      Inquire if maestro server is alive \n" 
	   "-----------------------------------------------------------------\n"
	   "xp_name    :refers to a valid experiment name\n"
	   "all        :string \"all\"\n"
	   "key        :key given by list command (-l)\n"
	   "\n");

  exit (exit_code);
}

char linkname[1024];
ssize_t r;
/* struct _depParameters *depXp=NULL; */
char filename[256];

/* int madmin (int argc, char* argv[]) */
int main (int argc, char* argv[])
{
  int i,next_option,answer,ret,status=0;
  int sock,bytes_read, bytes_sent, port, datestamp ;
  
  ServerActions whatAction;
  DepOption     Doption;

  unsigned int pid;

  struct sockaddr_in server;
  struct hostent *hostentry = NULL;
  struct in_addr ip;
  struct passwd *passwdEnt = getpwuid(getuid());

  static char buffer[1024];
  static char ipserver[32];
  static char htserver[32];
  static char host[128];
  static char node[256];
  static char exp_home[1024];
  static char xp[1024];
  static char depdir[1024];
  static char *SeqExpHome=NULL;
  static char signal[16]="madmin";
  static DIR *dp = NULL;
  struct dirent *d;
  struct stat st;
  struct sigaction act;

  char dpkey[128];
  char cmdBuf[1024];
  char extension[256];
  char underline[2];
  char ffilename[512];
  char Time[40];
  char lpargs[512];
  char *mversion = NULL;
  char *mshortcut = NULL;
  char *m5sum=NULL;
  char *buf2=NULL;
  char authorization_file[256];
  dpnode *LP=NULL; 
  FILE * fp = stdout;
  

  /* A string listing valid short options letters. */
  static const char* const short_options = ":ieshcbl:r:t:?";

  /* The name of the file to receive program output, or NULL for
     standard output.  */
  const char* input_file = NULL;

  /* Whether to display verbose messages.  */
  int verbose = 0;

  /* Remember the name of the program, to incorporate in messages.
     The name is stored in argv[0].  */

  program_name = argv[0];
  whatAction=NONE;

  do {
    next_option = getopt (argc, argv, short_options);
    switch (next_option)
    {
    case 'b':   /*  */
                break;
    case 'e':   /*  */
                break;
    case 'h':   /* -h or --help */
                /* User has requested usage information.  Print it to standard
                   output, and exit with exit code zero (normal termination).  */
                print_usage (stdout , 1);
                break;
    case 'i':   /* -i or --isalive */
                whatAction=IS_ALIVE;
                break;

    case 'c':   /* -i or --confile */
                /* This option takes an argument, the name of the directive input file xml format.  */
                input_file = optarg;
		whatAction=RELOAD_CONFIG;
                break;
    case 'l':   /* -l or --list */
                whatAction=LIST_DEPENDENCIES;
		if ( strcmp(optarg,"all") == 0 ) {
		       Doption=DEP_ALL;
                } else {
		       strcpy(xp,optarg);
		       Doption=DEP_EXP;
		}
                break;
    case 't':   /* -t or --list dependee */
                whatAction=LIST_DEPENDENCIES;
		strcpy(xp,optarg);
		Doption=DPE_EXP;
                break;

    case 's':   /* -s or --shutdown */
                whatAction=SHUT_DOWN_SERVER;
                break;
    case 'r':   /* -r or --remdep */
                whatAction=REMOVE_DEPENDENCIES;
		
		if ( strcmp(optarg,"all") == 0 ) {
		       Doption=DEP_ALL;
                } else if (optarg[0] == '2' ) {
		       strcpy(dpkey,optarg); 
		       Doption=DEP_KEY;
                } else {
		       Doption=DEP_EXP;
		       strcpy(xp,optarg); 
                }
                break;
    /* case 't':    -t or --timestep 
                verbose = 1;
                whatAction=CHANGE_TIME_STEP;
                break;
    */
    case ':':   /* The user specified an invalid option.  */
                SeqExpHome=getenv("SEQ_EXP_HOME");
                switch (optopt) {
		    case 'l':
                             whatAction=LIST_DEPENDENCIES;
                             if ( SeqExpHome == NULL ) {
                                      fprintf(stderr,"Cannot get SEQ_EXP_HOME, please define\n");
                                      exit(1);
                             } else {
		                   strcpy(xp,SeqExpHome);
		                   Doption=DEP_EXP;
		             }
			     break;
                    case 't':
                             whatAction=LIST_DEPENDENCIES;
                             if ( SeqExpHome == NULL ) {
                                      fprintf(stderr,"Cannot get SEQ_EXP_HOME, please define\n");
                                      exit(1);
                             } else {
		                   strcpy(xp,SeqExpHome);
		                   Doption=DPE_EXP;
			     }
			     break;
                    case 'r':
                             whatAction=REMOVE_DEPENDENCIES;
                             if ( SeqExpHome == NULL ) {
                                      fprintf(stderr,"Cannot get SEQ_EXP_HOME, please define\n");
                                      exit(1);
                             }
			     break;
                    default:
		             fprintf(stderr,"IN default\n");
                             print_usage (stdout , 1);
		}
                break;
    case -1:   /* Done with options.  */
                break; 

    default:    /* Something else: unexpected.  */
                fprintf(stderr,"invalid arguments \n");
                exit(1);
    }
  } while (next_option != -1);

  /* Done with options.  OPTIND points to first non-option argument.
     For demonstration purposes, print them if the verbose option was
     specified.  */
  if (verbose) {
           for (i = optind; i < argc; ++i) printf ("Argument: %s\n", argv[i]);
  }

  /* print usage if no args */
  if ( whatAction== NONE ) {
          print_usage (stdout , 1);
	  exit(1);
  }

  /* The main program goes here.  */
  if ( (mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
           fprintf(stderr,"Cannot get maestro version, please do a proper ssmuse\n");
           exit(1);
  }
  
  if (  (mshortcut=getenv("SEQ_MAESTRO_SHORTCUT")) == NULL ) {
           fprintf(stderr, "maestro_server(),Could not get maestro current version. Please do a proper ssmuse \n");
           exit(1);
  }

  snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",mversion);
  char *Auth_token = get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);
  
  if ( Auth_token == NULL) {
            fprintf(stderr, "Found No maestro_server_%s parameters file\n",mversion);
            exit(1);
  } else {
            int nscan = sscanf(Auth_token, "seqpid=%u seqhost=%s seqip=%s seqport=%u", &pid, htserver, ipserver, &port);
  }
  
  if ( (sock=connect_to_host_port_by_ip (ipserver,port))  < 1 ) {
            fprintf(stderr,"Cannot connect to host:%s and port:%d  ... exiting\n",htserver,port);
            exit(1);
  }

  gethostname(host, sizeof(host));
    
  /*
      do Login and get response
      Note : if the exp is not valid , the server will not log the client !!!!  
  */

  /* first set a handler for timeouts */
  act.sa_handler = &alarm_handler;
  act.sa_flags = 0;
  sigemptyset (&act.sa_mask);
  ret = sigaction (SIGALRM, &act, NULL);
  if (ret < 0) perror (__func__);

  strcpy(node,passwdEnt->pw_dir);
  sprintf(buffer,"%s/.suites",node);
  if ( access(buffer,R_OK) != 0 )  status = mkdir( buffer , S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  snprintf(exp_home,sizeof(exp_home),"%s",buffer);

  /* inter user dep. directory */
  snprintf(depdir,sizeof(depdir),"%s/maestrod/dependencies/polling/v%s",buffer,mversion);

  answer = do_Login(sock, pid, node, exp_home, signal, passwdEnt->pw_name, &m5sum);
  

  if ( answer != 0  ) {
           fprintf(stderr,"Failed Identification ... exiting\n");
           return(1);
  }


  memset(buffer,'\0',sizeof(buffer));
  switch ( whatAction ) 
  {
      case SHUT_DOWN_SERVER:
           alarm(5);
           bytes_sent=write(sock, "X ", 2);
           alarm(0);
	   if ( bytes_sent <= 0 ) {
	          fprintf(stderr,"Could not send to mserver. Timed out...bytes_sent=%d\n",bytes_sent);
		  break;
           }

           return(0);
      case CHANGE_DEBUG_ZONE:
	   kill (pid , SIGUSR2);
           break;
      case LIST_DEPENDENCIES:
           switch (Doption) 
	   {
	       /* add datestamp !! */
	       case DEP_ALL: /* insert list_ptr, DEPENDER xp, depender node, dependee xp name, dependee node, depender date, dependee date, depender loop args, dependee loop ars, file, link */
			    for ( LP=getDependencyFiles(depdir,"all",fp,"depender"); LP != NULL ; LP=LP->next ) {
	                          fprintf(stdout,"node:%s depend_on_exp:%s node:%s loop_args:%s key:%s \n",LP->snode,LP->depOnXp, LP->depOnNode, LP->depOnLargs, LP->key);
			          /* LP->waitfile */
			    }
			    free_list ( PRT_listdep );
                            break;
	       case DEP_EXP:
			   for ( LP=getDependencyFiles(depdir,xp,fp,"depender"); LP != NULL ; LP=LP->next ){
	                          fprintf(stdout,"node=%s depend_on_exp=%s node:%s loop_args:%s key:%s\n",LP->snode, LP->depOnXp, LP->depOnNode, LP->depOnLargs, LP->key);
	                          /* fprintf(stdout,"link=%s\n",LP->link); fprintf(stdout,"wfile=%s\n",LP->waitfile); */
			   }
			   free_list ( PRT_listdep );
			   break;
	       case DEP_KEY:
			   for ( LP= getDependencyFiles(depdir,xp,fp,"depender"); LP != NULL ; LP=LP->next ) {
	                          fprintf(stdout,"node=%s depend_on_exp=%s node=%s loop_args:%s key:%s\n",LP->snode, LP->depOnXp, LP->depOnNode, LP->depOnLargs, LP->key);
	                          /* fprintf(stdout,"link=%s\n",LP->link); fprintf(stdout,"wfile=%s\n",LP->waitfile); */
			   }
			   free_list ( PRT_listdep );
			   break;
	       case DPE_EXP: /* insert list_ptr, DEPENDEE xp, depender node, dependee xp name, dependee node, depender date, dependee date, depender loop args, dependee loop ars, file, link */
			    for ( LP=getDependencyFiles(depdir,xp,fp,"dependee"); LP != NULL ; LP=LP->next ) {
	                          fprintf(stdout,"node:%s exp:%s date:%s loop_args:%s key=%s\n",LP->snode, LP->depOnXp, LP->sdstmp, LP->slargs, LP->key);
			    }
			    free_list ( PRT_listdep );
	                 break;
	   }
	   break;
      case REMOVE_DEPENDENCIES:
           switch (Doption) 
	   {
	     case DEP_KEY: /* one dependency given by the key */
                         snprintf(cmdBuf,sizeof(cmdBuf),"%s/.suites/maestrod/dependencies/polling/v%s/%s",passwdEnt->pw_dir,mversion,dpkey);
                         r=readlink(cmdBuf,linkname,1023);
	                 linkname[r] = '\0'; 
	                 answer=unlink(linkname);
			 /* The server will remove the dangling link: we are doing this to have a trace in the server log  */
	                 if ( answer == 0 ) 
	                        fprintf(stdout,"dependency removed\n");
                         else
	                        fprintf(stdout,"failed to remove dependency\n");

                         break;
	     case DEP_EXP: /* dependencies for all this xp */
			 
			 for ( LP=getDependencyFiles(depdir,xp,fp,"depender"); LP != NULL ; LP=LP->next ) {
                                  memset(cmdBuf,'\0',sizeof(cmdBuf));

			          /* waitfile is the file in inter_depends (xml file), it is cleared by maestro when doing initnode.
				     We should probably erase Lp->link , but we will have no listing for this. this is
				     why we let the server remove (dangling link) and log action in mdpmanager_$date_$pid
				     answer=unlink(LP->waitfile); 
				     answer=unlink(LP->link); */

				  /* 
				    use system : no need to build linked list of loop args 
				  */

			          snprintf(cmdBuf,sizeof(cmdBuf),"SEQ_EXP_HOME=%s",xp);
			          if  ( (ret=putenv(cmdBuf)) != 0 ) fprintf(stderr,"Error adding variable seq_exp_home in environement \n"); 
				  
				  get_time(Time,1);
				  if ( (buf2=(char *) malloc (512*sizeof(char)) ) != NULL ) {
				             /* fprintf(stdout,"len=%d\n",strlen(buf2)); give 6 !!!! */
				             sprintf(buf2,"Inter-user dependency deleted (madmin) at:%s, will init node",Time);
                                  } else {
				             fprintf(stderr,"Could not malloc in remove dep\n");
                                             close(sock); free(buf2); exit(1);
                                  }

				  /* log the action to be done */ 
				  sprintf(lpargs,"%s","");
				  if ( strcmp(LP->slargs,"") != 0 ) {
				         sprintf(lpargs,"-l \"%s\"",LP->slargs);
				  } 
				         
				  /* wild card in loop ?? */
				  snprintf(cmdBuf,sizeof(cmdBuf),"%s >/dev/null 2>&1; export SEQ_EXP_HOME=%s; nodelogger -n %s -s \"initnode\" %s -m \"%s\" -d %s",mshortcut, LP->sxp, LP->snode, lpargs, buf2, LP->sdstmp);
				  ret=system(cmdBuf);

                                  /* build maestro init command */ 
                                  memset(cmdBuf,'\0',sizeof(cmdBuf));
				  snprintf(cmdBuf,sizeof(cmdBuf),"%s >/dev/null 2>&1; export SEQ_EXP_HOME=%s; export SEQ_DATE=%s;maestro -s initnode -n %s %s -f stop ",mshortcut, LP->sxp, LP->sdstmp, LP->snode, lpargs);
				  ret=system(cmdBuf);

				  free(buf2);
			 }
			 free_list ( PRT_listdep );
	                 break;
	     case DEP_ALL:
			 for ( LP= getDependencyFiles(depdir,"all",fp,"depender"); LP != NULL ; LP=LP->next ) {
	                          fprintf(stdout,"Removing dependency node:%s on exp:%s node:%s loop_args:%s\n",LP->snode,LP->depOnXp, LP->depOnNode, LP->depOnLargs);
			          answer=unlink(LP->waitfile); 
			 }
			 free_list ( PRT_listdep );
	                 break;
	     case DPE_EXP:
	                 break;
           }
           break;
      case CHANGE_TIME_STEP:
           break;
      case RELOAD_CONFIG: 
	   fprintf(stderr,"This Option is not supported now\n");
	   break;
      case IS_ALIVE:
           strcpy(buffer,"Y ");
	   alarm(5);
           bytes_sent=send(sock, buffer , sizeof(buffer) , 0);
	   alarm(0);
	   if ( bytes_sent <= 0 ) {
	          fprintf(stderr,"Could not send to mserver. Timed out... bytes_sent:%d\n",bytes_sent);
		  break;
           }

	   memset(buffer,'\0',sizeof(buffer));
	   alarm(5);
	   bytes_read=read(sock, buffer, sizeof(buffer));
	   alarm(0);
	   if ( bytes_read <= 0 ) {
	          fprintf(stderr,"Could not read from the mserver. Timed out... bytes_read=%d\n",bytes_read);
		  break;
           }

	   if ( buffer[0] == '0' ) fprintf(stdout,"%s\n",&buffer[2]);

           break;

  }
  /* end session */
  alarm(5);
  bytes_sent=write(sock, "S ", 2);
  alarm(0);
  if ( bytes_sent <= 0 ) fprintf(stderr,"Could not send to mserver. Timed out ... bytes_sent=%d\n",bytes_sent);

  close(sock);

  /* remove handler */
  act.sa_handler = SIG_DFL;
  act.sa_flags = 0;
  sigemptyset (&act.sa_mask);
  ret = sigaction (SIGALRM, &act, NULL);
  if (ret < 0) perror (__func__);

  free(m5sum);

  return 0;
}
