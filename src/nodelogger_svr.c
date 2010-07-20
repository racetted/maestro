 /*-------------------------------------------------------------------------------\
 |                      Inclusion des librairies                                  |
 \-------------------------------------------------------------------------------*/
#include  "nodelogger_svr.h"

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    : update_svr_file    ()                                              |
|                                                                                  |
|  Creation    : Francois , Patrick Andre (28 jul 2004)                            |
|                                                                                  |
|  Description : write the pid to a file which will be used to send                |
|                a NOHUP signal to cascade the logfiles                            |
|                                                                                  |
|  Parametre(s): Aucun                                                             |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void update_svr_file (int port) {
   int this_pid, pidf;
   char buf[MAX_LINE];
   char clientfile[MAX_LINE];
   char svrfile[MAX_LINE];
   char hostname[255];
   char truehost[255];
   
   memset(clientfile, '\0', sizeof(clientfile));
   memset(svrfile, '\0', sizeof(svrfile));
   memset(hostname, '\0', sizeof(hostname));
   memset(truehost, '\0', sizeof(truehost));

   gethostname(hostname, sizeof(hostname));
   /*
   if ( getenv("TRUE_HOST") != NULL ) {
      strcpy(truehost, getenv("TRUE_HOST"));
   } else {
      strcpy(truehost, hostname);
   }
   */
   strcpy(truehost, hostname);
   sprintf(svrfile, "%s/.suites/.nodelogger_svr_%s", getenv("HOME"), hostname);
   sprintf(clientfile, "%s/.suites/.nodelogger_client", getenv("HOME"));
   this_pid = getpid();
   if ( (pidf = open(svrfile, O_WRONLY|O_TRUNC|O_CREAT, 00664)) < 1 ) {
      sprintf(buf, "server: can't open file %s", svrfile);
      perror(buf);
      exit(1);
   }
   /* write server file, specific to host machine where svr is started */
   chmod(svrfile, 00664);
   sprintf( buf, "seqpid=%d seqport=%d", this_pid, port);
   write(pidf, buf, strlen(buf));
   close(pidf);

   /* write client file, should be the same, host contains TRUE_HOST value */
   if ( (pidf = open(clientfile, O_WRONLY|O_TRUNC|O_CREAT, 00664)) < 1 ) {
      sprintf(buf, "server: can't open file %s", clientfile);
      perror(buf);
      exit(1);
   }
   chmod(clientfile, 00664);
   sprintf( buf, "seqhost=%s seqport=%d", truehost, port);
   write(pidf, buf, strlen(buf));
   close(pidf);
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  process_cmds          ()                                         |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 jul 2004)                           |
|                                                                                  |
|  Description :  execution de la commande                                         |
|                                                                                  |
|  Parametre(s):                                                                   |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void process_cmds () {

   char   buf[MAX_LINE],firsin[MAX_CH];
   struct sockaddr_in server,server_eff;
   int    sizeserver = sizeof server;
   int    sizeserver_eff = sizeof server_eff;
   int    port,sock,sockz,logfile;

   SetInteger (socket(AF_INET, SOCK_STREAM, 0),'s');
   sock   = GetInteger ('s');
   printf ("sock = %d \n",sock);
   if (sock  < 0) {
      printf("Cannot open stream socket");
      exit (1);
   }

   server.sin_family= AF_INET;
   server.sin_port= htons(0);
   server.sin_addr.s_addr= INADDR_ANY;
   
   if (bind(sock,(struct  sockaddr *)&server,sizeserver)<0) {
      printf("Bind failed, abnormal termination. errno=%d\n",errno);
      exit(1);
   }

   getsockname(sock, (struct  sockaddr *)&server_eff, (unsigned int*) &sizeserver_eff);
   port =  ntohs(server_eff.sin_port);
   printf("process_cmds: port=%d SOMAXCONN=%d\n",port,SOMAXCONN);

   update_svr_file( port );
  (void) listen(sock,SOMAXCONN);

  while(1) {
    if ((sockz = accept(sock, (struct  sockaddr *)&server,(unsigned int*)  &sizeserver)) < 0){
      if (errno == EINTR)
        continue;
      else {
        printf("Accept failed, abnormal termination.");
        exit(1);
      }
    } else {
      memset(buf,'\0', sizeof buf);
      while ( read(sockz, buf, sizeof buf) > 0 ) {
        Set_buf_message (buf,firsin);
        if ((logfile = open(firsin, O_WRONLY|O_APPEND|O_CREAT, 00666)) != -1 ){
           write(logfile,buf, strlen(buf));
           close(logfile);
           memset(buf,'\0', sizeof buf);
        } else {
           printf("process_cmds failed: can't open firsin logfile:%s\n", firsin);
        }
      }
      close(sockz);
    }
  }
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  termine          ()                                              |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 jul 2004)                           |
|                                                                                  |
|  Description :                                                                   |
|                                                                                  |
|  Parametre(s):                                                                   |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void termine () {
  shutdown(GetInteger('s'),2);
  exit(0);
}

/*--------------------------------------------------------------------------------*\
|                                                                                  |
|  Fonction    :  reapchild         ()                                             |
|                                                                                  |
|  Creation    :  Francois , Patrick Andre (28 juil 2004)                          |
|                                                                                  |
|  Description :  Formater le message du buffer                                    |
|                                                                                  |
|  Parametre(s):                                                                   |
|                                                                                  |
|----------------------------------------------------------------------------------|
|                                                                                  |
|  Modification:                                                                   |
|                                                                                  |
|     Nom        :                                                                 |
|     Date       :                                                                 |
|     Description:                                                                 |
|                                                                                  |
\*--------------------------------------------------------------------------------*/
void reapchild() {
  int status;

  wait(&status);
  signal(SIGCLD, (void(*)()) reapchild);
}

main (argc, argv)
int argc;
char *argv[];
{
   umask(000);
   signal(SIGINT, (void(*)()) termine);
   signal(SIGQUIT, (void(*)()) termine);
   signal(SIGTERM, (void(*)()) termine);
   signal(SIGCLD, (void(*)()) reapchild);

   process_cmds();
}

