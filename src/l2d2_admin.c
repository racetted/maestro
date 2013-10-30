#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
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
#include "l2d2_roxml.h"
#include "l2d2_server.h"

#define MAXBUF 1024
extern char *get_Authorization (char *, char * ,char **);

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

char typeofFile(mode_t mode);
static void alarm_handler() { /* nothing */ }

/* 
 * Prints usage information for this program to STREAM (typically
 * stdout or stderr), and exit the program with EXIT_CODE.  Does not
 * return.
*/

void print_usage (FILE* stream , int existe_code)
{
  fprintf (stream, "Usage:  %s options [ args ... ]\n", program_name);
  fprintf (stream,
           "  -h  --help                        Display usage information.\n"
           "  -c  --confile  config.xml         Load maestro server configuration file.\n"
           "  -l  --listd                       List Registered Dependencies \n" 
           "  -r  --rmdep   token|xp_name       Remove a dependency args is dependency_token or experiment_name \n" 
           "  -s  --shutdown                    Shut Down maestro server \n" 
           "  -i  --isalive                     Inquire if maestro server is alive \n" 
           "  -v  --verbose                     Print verbose messages.\n");
  exit (existe_code);
}


int main (int argc, char* argv[])
{
  int next_option;
  int i,status=0;
  ServerActions whatAction;
  
  int sock,bytes_read, bytes_sent;
  unsigned int pid;
  int port;

  struct sockaddr_in server;
  struct hostent *hostentry = NULL;
  struct in_addr ip;
  int answer,ret;

  struct passwd *passwdEnt = getpwuid(getuid());

  char buffer[MAXBUF];
  char buffer1[MAXBUF];
  static char ipserver[20];
  static char htserver[20];
  static char host[100];
  static char node[100];
  static char seq_exp_home[512];
  static char signal[10]="madmin";
  static DIR *dp = NULL;
  struct dirent *d;
  struct stat st;
  ssize_t r;
  struct sigaction act;
  struct _depParameters *depXp=NULL;

  char dpkey[120];
  char buf[256];
  int *datestamp;
  char extension[256];
  char underline[2];
  char linkname[1024];
  char ffilename[512];
  char filename[256];
  char *mversion = NULL;
  char *mshorcut = NULL;
  char *m5sum=NULL;
  char authorization_file[256];
  

  /* A string listing valid short options letters.  */
  static const char* const short_options = "ielshcbr:t:?";

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
    case '?':   /* The user specified an invalid option.  */
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
                break;

    case 's':   /* -s or --shutdown */
                whatAction=SHUT_DOWN_SERVER;
                break;
    case 'r':   /* -r or --remdep */
                whatAction=REMOVE_DEPENDENCIES;
		strcpy(dpkey,optarg);
                break;
    case 't':   /* -t or --timestep */
                verbose = 1;
                whatAction=CHANGE_TIME_STEP;
                break;

    case -1:   /* Done with options.  */
                break; 

    default:    /* Something else: unexpected.  */
                fprintf(stderr,"invalide\n");
                abort ();
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
           fprintf(stderr,"Cannot get maestro version do a proper ssmuse\n");
           exit(1);
  }
  snprintf(authorization_file,sizeof(authorization_file),".maestro_server_%s",mversion);
  char *Auth_token = get_Authorization (authorization_file, passwdEnt->pw_name, &m5sum);
  
  if ( Auth_token == NULL) {
            fprintf(stderr, "Found No maestro_server_%s parameteres file\n",mversion);
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
  snprintf(seq_exp_home,sizeof(seq_exp_home),"%s",buffer);

  answer = do_Login(sock, pid, node, seq_exp_home, signal, passwdEnt->pw_name, &m5sum);
  

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
           snprintf(buf,sizeof(buf),"%s/.suites/maestrod/dependencies/polling/v%s",passwdEnt->pw_dir,mversion);
           if ( (dp=opendir(buf)) == NULL ) { 
                    fprintf(stderr,"Failed to open polling directory:%s \n",buf);
	            exit(1); 
	   }
           while ( d=readdir(dp))
           {
	        memset(linkname,'\0',sizeof(linkname));
                snprintf(ffilename,sizeof(ffilename),"%s/%s",buf,d->d_name);
                snprintf(filename,sizeof(filename),"%s",d->d_name);

                if ( stat(ffilename,&st) != 0 ) continue; 
                switch ( typeofFile(st.st_mode) ) {
                    case 'r'    :
                            /* skip hidden files */
                             if ( filename[0] == '.' ) continue;
                             /* test format */
                             i = sscanf(filename,"%14d%1[_]%s",&datestamp,underline,extension);
                             if ( i == 3 ) {
			             r=readlink(ffilename,linkname,1023);
				     linkname[r] = '\0';
                                     /* ok parse file and test dependency  */
                                     if (  (depXp=(struct _depParameters *) ParseXmlDepFile(linkname)) == NULL ) {
                                             fprintf(stderr,"SubmitDepProc(): Problem parsing xml file:%s\n",filename);
                                     } else {
                                             fprintf(stdout,"xpd_name        :%s\n",depXp->xpd_name);
                                             fprintf(stdout,"xpd_node        :%s\n",depXp->xpd_node);
                                             fprintf(stdout,"xpd_indx        :%s\n",depXp->xpd_indx);
                                             fprintf(stdout,"xpd_xpdate      :%s\n",depXp->xpd_xpdate);
                                             fprintf(stdout,"xpd_status      :%s\n",depXp->xpd_status);
                                             fprintf(stdout,"xpd_largs       :%s\n",depXp->xpd_largs);
                                             fprintf(stdout,"xpd_susr        :%s\n",depXp->xpd_susr);
                                             fprintf(stdout,"xpd_sname       :%s\n",depXp->xpd_sname);
                                             fprintf(stdout,"xpd_snode       :%s\n",depXp->xpd_snode);
                                             fprintf(stdout,"xpd_sxpdate     :%s\n",depXp->xpd_sxpdate);
                                             fprintf(stdout,"xpd_slargs      :%s\n",depXp->xpd_slargs);
                                             fprintf(stdout,"xpd_sub         :%s\n",depXp->xpd_sub);
                                             fprintf(stdout,"xpd_lock        :%s\n",depXp->xpd_lock);
                                             fprintf(stdout,"xpd_mversion    :%s\n",depXp->xpd_mversion);
                                             fprintf(stdout,"xpd_mdomain     :%s\n",depXp->xpd_mdomain);
                                             fprintf(stdout,"xpd_regtimedate :%s\n",depXp->xpd_regtimedate);
                                             fprintf(stdout,"xpd_regtimepoch :%s\n",depXp->xpd_regtimepoch);
                                             fprintf(stdout,"xpd_key         : %s\n",filename);
                                             fprintf(stdout,"++++++++++++++++++++++++++++++++++++++++++++++++++\n");
                                     }
			     }
		   	     break;
                    default:
                             break;
                }
           }
           closedir(dp);
           break;
      case REMOVE_DEPENDENCIES:
           snprintf(buf,sizeof(buf),"%s/.suites/maestrod/dependencies/polling/v%s/%s",passwdEnt->pw_dir,mversion,dpkey);
           r=readlink(buf,linkname,1023);
	   linkname[r] = '\0'; 
	   answer=unlink(linkname);
	   if ( answer == 0 ) 
	      fprintf(stdout,"dependency removed\n");
           else
	      fprintf(stdout,"failed to remove dependency\n");

           break;
      case CHANGE_TIME_STEP:
           break;
      case RELOAD_CONFIG: /* has to be reviewed */
           sprintf(buffer,"I %s",input_file);
	   alarm(5);
           bytes_sent=send(sock, buffer , sizeof(buffer) , 0);
	   alarm(0);
	   if ( bytes_sent <= 0 ) {
	          fprintf(stderr,"Could not send to mserver. Timed out... bytes_sent=%d\n",bytes_sent);
		  break;
           }
	  
	   memset(buffer,'\0',sizeof(buffer));
	   alarm(5);
	   bytes_read=read(sock, buffer, sizeof(buffer));
	   alarm(0);
	   
	   if ( bytes_read <= 0 ) {
	          fprintf(stderr,"Could not read from mserver. Timed out... bytes_read=%d\n",bytes_read);
		  break;
           }
	   if ( buffer[0] == '0' )
	        fprintf(stderr,"Configuration file Reloaded \n");
           else
	        fprintf(stderr,"Problems Reloading Configuration file\n");
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

	   if ( buffer[0] == '0' )
	        fprintf(stderr,"Server is Alive \n");
           else
	        fprintf(stderr,"Server Not responding Correctly!, got:%s\n",buffer);


           break;

  }
  /* end session */
  alarm(5);
  bytes_sent=write(sock, "S ", 2);
  alarm(0);
  if ( bytes_sent <= 0 ) fprintf(stderr,"Could not sent to mserver. Timed out ... bytes_sent=%d\n",bytes_sent);

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
