#include <stdio.h>
#include <ctype.h>
#include <stdlib.h> /*do not remove the stdlib.h library cause 'atof' needs this library*/
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <libgen.h>
#include <limits.h>
#include <sys/param.h>
#include <pwd.h>
#include <unistd.h>
#include "runcontrollib.h"
#include "ocmjinfo.h"


/* name: ocmjinfo
 * 
 * date: January 1993
 *
 * author: Douglas Bender
 *
 * description: searches the ocm master files and displays
 *              all information  about an operational job
 *
 * revision: March 1993 Douglas Bender
 *                      - add hour extension to variables
 *                      - allow dependency hours of previous
 *                        runs
 *
 *           August 1993 Douglas Bender
 *                      - "catchup" field added to master files
 *
 *           September 1993 Douglas Bender
 *                      - '+' and '-' characters now accepted for
 *                        dependency hours
 *
 *           January 1994 Douglas Bender
 *                      - "-prefix" option added
 *
 *           September  1995 Douglas Bender
 *                      - "Event" dependancy code added
 *
 *           March 1996 Douglas Bender
 *                      - hardwired HOUR=18 for "ew" and "em" jobs
 *
 *           October 1996 Douglas Bender
 *                      - run series code added for ensemble forecast runs
 *
 *           November 1996 Douglas Bender
 *                      - bugfix for "em" and "ew" runs
 *
 *           February 1997 Douglas Bender
 *                      - "-p" parameter option to spit out results in
 *                        perl syntax
 *
 *           December 1997 Douglas Bender
 *                      - series elements added
 *
 *           December 1998 Ping-An Tan
 *                      - changed 'MMDDYYHH' datestamp into 'YYYYMMDDHH'
 *                        datestamp.
 *
 *           September 1999 Ping-An Tan
 *                      - Modified this module so it can be included into
 *                        the runcontrol software package.
 *           January 11th, 2000 Ping-An Tan
 *                      - Modified the 'gendtstmp' function so it would will
 *                        wait 5 secondes if the uspmadt file is empty. The uspmadt
 *                        is empty when it's being copied over.
 *           January 13th, 2000 Ping-An Tan
 *                      - enable serie jobs to have dependency.
 *           July 31st, 2000 Ping-An Tan
 *                      - add 'set' variable
 *           November 8th, 2000 Ping-An Tan
 *                      - add 'cpu' field into masterfiles
 *           May 14th, 2002 Ping-An Tan
 *                     - modify splitfield so it would accept HHMM as extension
 *                       for aj jobs
 *           Oct 30th, 2003 Ping-An Tan
 *                     - add 'wallclock' field into masterfiles
 *           Aug 18th, 2004 Ping-An Tan
 *                     - if $CMC_OCMPATH then
 *                           $CMC_OCMPATH/ocm/control/mastertable
 *                           $CMC_OCMPATH/ocm/control/aliastable
 *                           $CMC_OCMPATH/ocm/control/
 *                           $CMC_OCMPATH/datafiles/data/uspmadt
 *                        else
 *                           /home/binops/afsi/sio/ocm/control/mastertable
 *                           /home/binops/afsi/sio/ocm/control/aliastable
 *                           /home/binops/afsi/sio/ocm/control/
 *                           /usr/local/env/afsisio/datafiles/data/uspmadt
 *                        fi
 *
 *          Nov 1st, 2004 Ping-An Tan
 *                    - add 'loop' field into masterfiles
 *                    - change logic from series to loops.
 *          April, 2006 Ping-An Tan
 *                    - add $USER@$SUITE@$JOBNAME dependency
 *          July, 2006 Ping-An Tan
 *                    - modify dependency field so the $USER is optional
 *                    - [$USER]@$SUITE@$JOBNAME
 *                    - 
 *                         
 */

extern char *__loc1; /* needed for regex */
static FILE *jinfo_su;

static char jinfo_spma[JINFO_MAXLINE];
static char JINFO_MASTERTABLE[JINFO_MAXLINE];
static char JINFO_ALIASTABLE[JINFO_MAXLINE];
static char JINFO_MASTERBASE[JINFO_MAXLINE];

static char JINFO_DATESTAMP[JINFO_MAX_FIELD];
static char JINFO_LOOP_CURRENT[JINFO_MAX_FIELD];
static char JINFO_LOOP_START[JINFO_MAX_FIELD];
static char JINFO_LOOP_SET[JINFO_MAX_FIELD];
static char JINFO_LOOP_TOTAL[JINFO_MAX_FIELD];
static char JINFO_LOOP_COLOR[JINFO_MAX_FIELD];
static char JINFO_LOOP_REFERENCE[JINFO_MAX_FIELD];
static char JINFO_RUN[JINFO_MAX_FIELD];
static char JINFO_JOB[JINFO_MAX_FIELD];
static char JINFO_CATCHUP[JINFO_MAX_FIELD];
static char JINFO_CPU[JINFO_MAX_FIELD];
static char JINFO_WALLCLOCK[JINFO_MAX_FIELD];
static char JINFO_QUEUE[JINFO_MAX_FIELD];
static char JINFO_MACHINE_QUEUE[JINFO_MAX_FIELD];
static char JINFO_MACHINE[JINFO_MAX_FIELD];
static char JINFO_MEMORY[JINFO_MAX_FIELD];
static char JINFO_MPI[JINFO_MAX_FIELD];
static char JINFO_ABORT[JINFO_MAX_FIELD];
static char JINFO_SILENT[JINFO_MAX_FIELD];
static char JINFO_DEPEND[JINFO_MAX_FIELD];
static char JINFO_SUBMIT[JINFO_MAX_FIELD];
static char JINFO_HOUR[JINFO_MAX_FIELD];
static char JINFO_ALIAS_JOB[JINFO_MAX_FIELD];
static char JINFO_ARGS[JINFO_MAX_FIELD];
static char JINFO_MASTER[JINFO_MAXLINE];

static int JINFO_OFFSET, JINFO_FLAG_LOOPS;

static FILE  *jinfo_master, *jinfo_mastertable, *jinfo_aliastable;

static void message (char *msg1,char *msg2,char *var);
static int convert_tmphour(char plusorminus,char tmphour[5],char final_tmphour[5]);
static int getmaster(char *controlDir,char *suite,char *run,char **masterfile);
static void check4alias(char *job);
static int splitfield(char *var,char *field,char *result);
static void setfield(char *line,char *field);
static int findjob_setvariables(char *job);
static void getrunhourandjob(char *job);
static void getLoopFileInfos(char *run,char *loopRef);
static int getJobLoopInfos(char *controlDir,char *suite,char *job,int *loop_limit,int *set,int *start);

void insertItem(JLISTNODEPTR *, char *);
char *deleteItem(JLISTNODEPTR *);
int isListEmpty(JLISTNODEPTR sPtr);
void deleteWholeList(JLISTNODEPTR *sPtr);
void printList(JLISTNODEPTR);
void deallocateOcmjinfo(struct ocmjinfos **ptrocmjinfo);
struct ocmjinfos ocmjinfo(char *job);
void printOcmjinfo(struct ocmjinfos *ptrocmjinfo);

/********************************************************************************
* insertItem: Inserts an Item into the list
********************************************************************************/
void insertItem(JLISTNODEPTR *sPtr, char *chaine)
{
 JLISTNODEPTR newPtr=NULL, previousPtr=NULL, currentPtr=NULL;

 newPtr = malloc(sizeof(JLISTNODE));
 if (newPtr != NULL) { /* is space available */
    newPtr->data = (char *) malloc(strlen(chaine)+1);
    strcpy(newPtr->data,chaine);
    newPtr->nextPtr = NULL;
   
    previousPtr = NULL;
    currentPtr = *sPtr;

    while (currentPtr != NULL) {
       previousPtr = currentPtr; 
       currentPtr = currentPtr->nextPtr;
    }
     
    if (previousPtr == NULL) {
       newPtr->nextPtr=*sPtr;
       *sPtr = newPtr;
    } else {
       previousPtr->nextPtr=newPtr;
       newPtr->nextPtr=currentPtr;
    }
  } else {
      printf("%s not inserted. No memory available.\n",chaine);
  }
}

/********************************************************************************
* deleteItem: deletes an element from the list and return it.
********************************************************************************/
char *deleteItem(JLISTNODEPTR *sPtr)
{
JLISTNODEPTR tempPtr=NULL;
 char *data=NULL;

 tempPtr = *sPtr;
 data = (char *) malloc(strlen(tempPtr->data) + 1);

 strcpy(data,(*sPtr)->data);
 *sPtr = (*sPtr)->nextPtr;
 tempPtr->nextPtr=NULL;
 free(tempPtr->data);
 free(tempPtr);
 tempPtr=NULL;
 return(data);
}

/********************************************************************************
* deleteWholeList: deletes an element from the list and return it.
********************************************************************************/
void deleteWholeList(JLISTNODEPTR *sPtr)
{

 if ( *sPtr != NULL) {
   deleteWholeList(&(*sPtr)->nextPtr);
   free(*sPtr);
   *sPtr=NULL;
 }

}

/********************************************************************************
* isListEmpty: Returns 1 if the list is empty, 0 otherwise
********************************************************************************/
int isListEmpty(JLISTNODEPTR sPtr)
{
 if (sPtr == NULL) {
    return(1);
 } else {
    return(0);
 }
}


/********************************************************************************
*printList: Prints the contents of the list
********************************************************************************/
void printList(JLISTNODEPTR currentPtr)
{
 if (currentPtr == NULL) {
    printf("List is empty.\n");
  } else {
      printf("%s", currentPtr->data);
      currentPtr = currentPtr->nextPtr;
      while (currentPtr != NULL) {
         printf(" %s", currentPtr->data);
         currentPtr = currentPtr->nextPtr;
      }
 }
}

/********************************************************************************
*deallocateOcmjinfo: deallocate memory for the ocmjinfos struct
********************************************************************************/
void deallocateOcmjinfo(struct ocmjinfos **ptrocmjinfo)
{
 struct ocmjinfos *tempocmjinfo;

 tempocmjinfo = *ptrocmjinfo;

 if (tempocmjinfo->abort != NULL) {
     deleteWholeList(&(tempocmjinfo->abort));
 }

 if (tempocmjinfo->depend != NULL) {
    deleteWholeList(&(tempocmjinfo->depend));
 }

 if (tempocmjinfo->submit != NULL) {
    deleteWholeList(&(tempocmjinfo->submit));
 }

 if (tempocmjinfo->job != NULL) {
    free(tempocmjinfo->job);
    tempocmjinfo->job=NULL;
 }

 if (tempocmjinfo->queue != NULL) {
    free(tempocmjinfo->queue);
    tempocmjinfo->queue=NULL;
 }

 if (tempocmjinfo->machine_queue != NULL) {
    free(tempocmjinfo->machine_queue);
    tempocmjinfo->machine_queue=NULL;
 }

 if (tempocmjinfo->machine != NULL) {
    free(tempocmjinfo->machine);
    tempocmjinfo->machine=NULL;
 }

 if (tempocmjinfo->cpu != NULL) {
    free(tempocmjinfo->cpu);
    tempocmjinfo->cpu=NULL;
 }

 if (tempocmjinfo->memory != NULL) {
    free(tempocmjinfo->memory);
    tempocmjinfo->memory=NULL;
 }
 
 if (tempocmjinfo->run != NULL) {
    free(tempocmjinfo->run);
    tempocmjinfo->run=NULL;
 }

 if (tempocmjinfo->hour != NULL) {
    free(tempocmjinfo->hour);
    tempocmjinfo->hour=NULL;
 }

 if (tempocmjinfo->loop_current != NULL) {
    free(tempocmjinfo->loop_current);
    tempocmjinfo->loop_current=NULL;
 }

 if (tempocmjinfo->loop_start != NULL) {
    free(tempocmjinfo->loop_start);
    tempocmjinfo->loop_start=NULL;
 }

 if (tempocmjinfo->loop_set != NULL) {
    free(tempocmjinfo->loop_set);
    tempocmjinfo->loop_set=NULL;
 }
 
 if (tempocmjinfo->loop_total != NULL) {
    free(tempocmjinfo->loop_total);
    tempocmjinfo->loop_total=NULL;
 }
 
 if (tempocmjinfo->loop_color != NULL) {
    free(tempocmjinfo->loop_color);
    tempocmjinfo->loop_color=NULL;
 }

 if (tempocmjinfo->loop_reference != NULL) {
    free(tempocmjinfo->loop_reference);
    tempocmjinfo->loop_reference=NULL;
 } 

 if (tempocmjinfo->master != NULL) {
    free(tempocmjinfo->master);
    tempocmjinfo->master=NULL;
 }

 if (tempocmjinfo->alias_job != NULL) {
    free(tempocmjinfo->alias_job);
    tempocmjinfo->alias_job=NULL;
 }

 if (tempocmjinfo->args != NULL) {
    free(tempocmjinfo->args);
    tempocmjinfo->args=NULL;
 }

}

/********************************************************************************
*printOcmjinfo: Prints the contents of the structure ocmjinfos pointed by ptrocmjinfo
********************************************************************************/
void printOcmjinfo(struct ocmjinfos *ptrocmjinfo)
{

 if ( ptrocmjinfo->error == 0) {
    printf("ocmjinfo_job='%s'\n", ptrocmjinfo->job);
    printf("ocmjinfo_catchup='%d'\n", ptrocmjinfo->catchup);
    printf("ocmjinfo_cpu='%s'\n", ptrocmjinfo->cpu);
    printf("ocmjinfo_wallclock='%d'\n", ptrocmjinfo->wallclock);
    printf("ocmjinfo_mpi='%d'\n", ptrocmjinfo->mpi);
    printf("ocmjinfo_silent='%d'\n", ptrocmjinfo->silent);
    printf("ocmjinfo_machine_queue='%s'\n", ptrocmjinfo->machine_queue);
    printf("ocmjinfo_memory='%s'\n", ptrocmjinfo->memory);

    if (ptrocmjinfo->queue != NULL) {
       printf("ocmjinfo_queue='%s'\n", ptrocmjinfo->queue);
    }

    if (ptrocmjinfo->machine != NULL) {
       printf("ocmjinfo_machine='%s'\n", ptrocmjinfo->machine);
    }

    if (ptrocmjinfo->abort != NULL) {
       printf("ocmjinfo_abort='");
       printList(ptrocmjinfo->abort);
       printf("'\n");
    }   

    if (ptrocmjinfo->depend != NULL) {
       printf("ocmjinfo_depend='");
       printList(ptrocmjinfo->depend);
       printf("'\n");
    }

    if (ptrocmjinfo->submit != NULL) {
       printf("ocmjinfo_submit='");
       printList(ptrocmjinfo->submit);
       printf("'\n");
    }
    printf("ocmjinfo_run=%s\n", ptrocmjinfo->run);
    printf("ocmjinfo_hour=%s\n", ptrocmjinfo->hour);
    if (ptrocmjinfo->loop_current != NULL) {
       printf("ocmjinfo_loop_current=%s\n", ptrocmjinfo->loop_current);
    }
    if (ptrocmjinfo->loop_start != NULL) {
       printf("ocmjinfo_loop_start=%s\n", ptrocmjinfo->loop_start);
    }
    if (ptrocmjinfo->loop_set != NULL) {
       printf("ocmjinfo_loop_set=%s\n", ptrocmjinfo->loop_set);
    }
    if (ptrocmjinfo->loop_total != NULL) {
       printf("ocmjinfo_loop_total=%s\n", ptrocmjinfo->loop_total);
    }
    if (ptrocmjinfo->loop_color != NULL) {
       printf("ocmjinfo_loop_color=%s\n", ptrocmjinfo->loop_color);
    }
    if (ptrocmjinfo->loop_reference != NULL) {
       printf("ocmjinfo_loop_reference=%s\n", ptrocmjinfo->loop_reference);
    }
    printf("ocmjinfo_master=%s\n", ptrocmjinfo->master);
    printf("ocmjinfo_alias_job=%s\n", ptrocmjinfo->alias_job);
    printf("ocmjinfo_args='%s'\n", ptrocmjinfo->args);
 } else {
    fprintf(stderr,"%s\n",ptrocmjinfo->errormsg);
 }
}


/*****************************************************************************
* NAME: ocmjinfo
* PURPOSE: get information for a specified operational job.
* INPUT: job - jobname[_HH][_SSS]
*              where HH  - zulu [00,06,12,18]
*                    SSS - series
*              note: HH is only required for runs r[1-2], g[1-6]
* OUTPUT: the result is returned on a ocmjinfos structure.
* example: struct ocmjinfos *ocmjinfo_res;
*          ocmjinfo_res = malloc(sizeof(struct ocmjinfos));
*          *ocmjinfo_res = ocmjinfo("r1start_12");
*******************************************************************************/
struct ocmjinfos ocmjinfo(char *job)
{
 int getmaster_status;
 char tmp_result[JINFO_MAX_FIELD];
 struct ocmjinfos ocmjinfo_res;
 char tmp[JINFO_MAX_FIELD];
 char *tmpstrtok = NULL;
 char *cmc_ocmpath = NULL;
 char *masterfile = NULL;

 memset(jinfo_spma,'\0',sizeof jinfo_spma);
 memset(JINFO_MASTERTABLE,'\0',sizeof JINFO_MASTERTABLE);
 memset(JINFO_ALIASTABLE,'\0',sizeof JINFO_ALIASTABLE);
 memset(JINFO_MASTERBASE,'\0',sizeof JINFO_MASTERBASE);

 cmc_ocmpath=getenv("CMC_OCMPATH");
 if ( cmc_ocmpath != NULL ) {
    sprintf(jinfo_spma,"%s%s",cmc_ocmpath,"/datafiles/data/uspmadt");
    sprintf(JINFO_MASTERTABLE,"%s%s",cmc_ocmpath,"/ocm/control/mastertable");
    sprintf(JINFO_ALIASTABLE,"%s%s",cmc_ocmpath,"/ocm/control/aliastable");
    sprintf(JINFO_MASTERBASE,"%s%s",cmc_ocmpath,"/ocm/control/");
 } else {
    /* strcpy(jinfo_spma,"/usr/local/env/afsisio/datafiles/data/uspmadt"); */
    strcpy(jinfo_spma,"/home/binops/afsi/sio/datafiles/data/uspmadt");
    strcpy(JINFO_MASTERTABLE,"/home/binops/afsi/sio/ocm/control/mastertable");
    strcpy(JINFO_ALIASTABLE,"/home/binops/afsi/sio/ocm/control/aliastable");
    strcpy(JINFO_MASTERBASE,"/home/binops/afsi/sio/ocm/control/");
 }

 JINFO_OFFSET=0;
 JINFO_FLAG_LOOPS = 0;

 /*initialize the results first*/
 ocmjinfo_res.job = NULL;
 ocmjinfo_res.catchup = 0;
 ocmjinfo_res.cpu = NULL;
 ocmjinfo_res.wallclock = 0;
 ocmjinfo_res.queue = NULL;
 ocmjinfo_res.queue = (char *) malloc(strlen("null")+1);
 strcpy(ocmjinfo_res.queue,"null");
 ocmjinfo_res.machine_queue = NULL;
 ocmjinfo_res.machine = NULL;
 ocmjinfo_res.memory = NULL;
 ocmjinfo_res.mpi = 0;
 ocmjinfo_res.silent = 0;
 ocmjinfo_res.abort = NULL;
 ocmjinfo_res.depend = NULL;
 ocmjinfo_res.submit = NULL;
 ocmjinfo_res.run = NULL;
 ocmjinfo_res.hour = NULL;
 ocmjinfo_res.loop_current = NULL;
 ocmjinfo_res.loop_start = NULL;
 ocmjinfo_res.loop_set = NULL;
 ocmjinfo_res.loop_total = NULL;
 ocmjinfo_res.loop_color = NULL;
 ocmjinfo_res.loop_reference = NULL;
 ocmjinfo_res.master = NULL;
 ocmjinfo_res.alias_job = NULL;
 ocmjinfo_res.args = NULL;
 ocmjinfo_res.error = 0;

 memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
 memset(JINFO_DATESTAMP,'\0',sizeof JINFO_DATESTAMP);
 memset(JINFO_LOOP_CURRENT,'\0',sizeof JINFO_LOOP_CURRENT);
 memset(JINFO_LOOP_START,'\0',sizeof JINFO_LOOP_START);
 memset(JINFO_LOOP_SET,'\0',sizeof JINFO_LOOP_SET);
 memset(JINFO_LOOP_TOTAL,'\0',sizeof JINFO_LOOP_TOTAL);
 memset(JINFO_LOOP_COLOR,'\0',sizeof JINFO_LOOP_COLOR);
 memset(JINFO_LOOP_REFERENCE,'\0',sizeof JINFO_LOOP_REFERENCE);
 memset(JINFO_RUN,'\0',sizeof JINFO_RUN);
 memset(JINFO_JOB,'\0',sizeof JINFO_JOB);
 memset(JINFO_CATCHUP,'\0',sizeof JINFO_CATCHUP);
 memset(JINFO_CPU,'\0',sizeof JINFO_CPU);
 memset(JINFO_WALLCLOCK,'\0',sizeof JINFO_WALLCLOCK);
 memset(JINFO_QUEUE,'\0',sizeof JINFO_QUEUE);
 memset(JINFO_MACHINE_QUEUE,'\0',sizeof JINFO_MACHINE_QUEUE);
 memset(JINFO_MACHINE,'\0',sizeof JINFO_MACHINE);
 memset(JINFO_MEMORY,'\0',sizeof JINFO_MEMORY);
 memset(JINFO_MPI,'\0',sizeof JINFO_MPI);
 memset(JINFO_SILENT,'\0',sizeof JINFO_SILENT);
 memset(JINFO_ABORT,'\0',sizeof JINFO_ABORT);
 memset(JINFO_DEPEND,'\0',sizeof JINFO_DEPEND);
 memset(JINFO_SUBMIT,'\0',sizeof JINFO_SUBMIT);
 memset(JINFO_HOUR,'\0',sizeof JINFO_HOUR);
 memset(JINFO_ALIAS_JOB,'\0',sizeof JINFO_ALIAS_JOB);
 memset(JINFO_ARGS,'\0',sizeof JINFO_ARGS);
 memset(JINFO_MASTER,'\0',sizeof JINFO_MASTER);

 jinfo_mastertable = fopen(JINFO_MASTERTABLE, "r");
 
 if (jinfo_mastertable == NULL) {
    printf("mastertable:%s\n",JINFO_MASTERTABLE);
    ocmjinfo_res.error = 1;
    memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
    sprintf(ocmjinfo_res.errormsg,"ABORT: ocmjinfo: can't open %s\n",JINFO_MASTERTABLE);
    return ocmjinfo_res;
 }

 getrunhourandjob(job);

 /* get master filename from mastertable */
 /* masterfiletable is a file containing a list of valid */
 /* run names and their corresponding master file */
 /* format:                                       */
 /*        g100    .master                        */
 /*        g106    .master2                       */

 getmaster_status = getmaster("NONE","NONE",JINFO_RUN,&masterfile);

 if (getmaster_status == 1) {
    /* could not find the correct masterfile for JINFO_RUN */
    memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
    sprintf(ocmjinfo_res.errormsg,"ABORT: ocmjinfo: %s not in mastertable\n",JINFO_RUN);
    ocmjinfo_res.error = 3;
    return ocmjinfo_res; 
 }

 if (getmaster_status == 2) {
    /* could not find the correct masterfile for JINFO_RUN */
    memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
    sprintf(ocmjinfo_res.errormsg,"ABORT: ocmjinfo: %s requires an hour ie. %s00 %s06 %s12 %s18\n",JINFO_RUN,JINFO_RUN,JINFO_RUN,JINFO_RUN,JINFO_RUN);
    ocmjinfo_res.error = 3;
    return ocmjinfo_res;
 }

 strcpy(JINFO_MASTER,masterfile);

 jinfo_master= fopen(JINFO_MASTER, "r");
 if (jinfo_master== NULL) {
    printf("master:%s\n",JINFO_MASTER);
    memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
    sprintf(ocmjinfo_res.errormsg,"ABORT: ocmjinfo: can't open %s\n",JINFO_MASTER);
    ocmjinfo_res.error = 1;
    return ocmjinfo_res;
 }

 /* search the first field in the masterfile for the */
 /* specified job */

 if ( findjob_setvariables(JINFO_JOB) == 1 ) {
    /* could not find the job in the masterfile */
    memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
    sprintf(ocmjinfo_res.errormsg,"ABORT: ocmjinfo: could not find %s in %s\n",JINFO_JOB,JINFO_MASTER);
    ocmjinfo_res.error = 2;
    return ocmjinfo_res;
 }

 if ( JINFO_FLAG_LOOPS ) {
   /*set_series_elements();*/
    ocmjinfo_res.loop_current = (char *) malloc(strlen(JINFO_LOOP_CURRENT)+1);
    strcpy(ocmjinfo_res.loop_current,JINFO_LOOP_CURRENT);
 }

 ocmjinfo_res.loop_reference = (char *) malloc(strlen(JINFO_LOOP_REFERENCE)+1);
 strcpy(ocmjinfo_res.loop_reference,JINFO_LOOP_REFERENCE);

 if (strncmp(JINFO_LOOP_REFERENCE,"REG",3) != 0) {
    getLoopFileInfos(JINFO_RUN,JINFO_LOOP_REFERENCE);
    ocmjinfo_res.loop_start = (char *) malloc(strlen(JINFO_LOOP_START)+1);
    strcpy(ocmjinfo_res.loop_start,JINFO_LOOP_START);
    ocmjinfo_res.loop_set = (char *) malloc(strlen(JINFO_LOOP_SET)+1);
    strcpy(ocmjinfo_res.loop_set,JINFO_LOOP_SET);
    ocmjinfo_res.loop_total = (char *) malloc(strlen(JINFO_LOOP_TOTAL)+1);
    strcpy(ocmjinfo_res.loop_total,JINFO_LOOP_TOTAL);
    ocmjinfo_res.loop_color = (char *) malloc(strlen(JINFO_LOOP_COLOR)+1);
    strcpy(ocmjinfo_res.loop_color,JINFO_LOOP_COLOR);
 }

 /* check if the jobname is an alias for another */
 /* set ocmjinfo_alias_job and adjust JINFO_ARGS */

 jinfo_aliastable = fopen(JINFO_ALIASTABLE, "r");
 if (jinfo_aliastable == NULL) {
    memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
    sprintf(ocmjinfo_res.errormsg,"ABORT: ocmjinfo: can't open %s\n",JINFO_ALIASTABLE);
    ocmjinfo_res.error = 1;
    return ocmjinfo_res;        
 }

 check4alias(JINFO_JOB);

 /* break the argument into its components as defined */
 /* at the beginning of this program                  */

 if ( strcmp(JINFO_JOB, JINFO_NONE) != 0 ) {
    if (splitfield("job", JINFO_JOB,tmp_result) == 0) {
       ocmjinfo_res.job = (char *) malloc(strlen(tmp_result)+1);
       strcpy(ocmjinfo_res.job,tmp_result);
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field job=%s\n",JINFO_JOB);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;          
    }
 }

 if ( strcmp(JINFO_CATCHUP, JINFO_NONE) != 0 ) {
    if (splitfield("catchup", JINFO_CATCHUP,tmp_result) == 0) {
        ocmjinfo_res.catchup = atoi(tmp_result); 
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field catchup=%s\n",JINFO_CATCHUP);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;     
    }
 }

 if ( strcmp(JINFO_CPU, JINFO_NONE) != 0 ) {
    if (splitfield("cpu", JINFO_CPU,tmp_result) == 0) {
        ocmjinfo_res.cpu = (char *) malloc(strlen(tmp_result)+1);
        strcpy(ocmjinfo_res.cpu,tmp_result); 
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field cpu=%s\n",JINFO_CPU);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;     
    }
 }

 if ( strcmp(JINFO_MPI, JINFO_NONE) != 0 ) {
    if (splitfield("mpi", JINFO_MPI,tmp_result) == 0) {
        ocmjinfo_res.mpi = atoi(tmp_result); 
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field mpi=%s\n",JINFO_MPI);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;     
    }
 }

 if ( strcmp(JINFO_LOOP_REFERENCE, JINFO_NONE) != 0 ) {
    if (splitfield("loop", JINFO_LOOP_REFERENCE,tmp_result) == 0) {
        ocmjinfo_res.loop_reference = (char *) malloc(strlen(tmp_result)+1);
        strcpy(ocmjinfo_res.loop_reference,tmp_result);
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field loop reference=%s\n",JINFO_LOOP_REFERENCE);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;     
    }
 }

 if ( strcmp(JINFO_WALLCLOCK, JINFO_NONE) != 0 ) {
    if (splitfield("wallclock", JINFO_WALLCLOCK,tmp_result) == 0) {
        ocmjinfo_res.wallclock = atoi(tmp_result); 
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field wallclock=%s\n",JINFO_WALLCLOCK);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;     
    }
 }

 if ( strcmp(JINFO_MEMORY, JINFO_NONE) != 0 ) {
    if (splitfield("memory", JINFO_MEMORY,tmp_result) == 0) {
       ocmjinfo_res.memory = (char *) malloc(strlen(tmp_result)+1);
       strcpy(ocmjinfo_res.memory,tmp_result);
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field memory=%s\n",JINFO_MEMORY);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;
    }
 }

 if ( strcmp(JINFO_MACHINE_QUEUE, JINFO_NONE) != 0 ) {
    if (splitfield("machine_queue", JINFO_MACHINE_QUEUE,tmp_result) == 0) {
       ocmjinfo_res.machine_queue = (char *) malloc(strlen(tmp_result)+1);
       strcpy(ocmjinfo_res.machine_queue,tmp_result);
       tmpstrtok = strtok(tmp_result,":");
       ocmjinfo_res.machine = (char *) malloc(strlen(tmpstrtok)+1);
       strcpy(ocmjinfo_res.machine,tmpstrtok);
       tmpstrtok = strtok(NULL,":");
       if  (tmpstrtok != NULL) {
           ocmjinfo_res.queue = (char *) malloc(strlen(tmpstrtok)+1); 
           strcpy(ocmjinfo_res.queue,tmpstrtok);
       }
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field machine_queue=%s\n",JINFO_MACHINE_QUEUE);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res;
    }
 }

 if ( strcmp(JINFO_ABORT, JINFO_NONE) != 0 ) {
    if (splitfield("abort", JINFO_ABORT,tmp_result) == 0) {
       tmpstrtok = strtok(tmp_result," ");
       while ( tmpstrtok != NULL) {
          insertItem(&(ocmjinfo_res.abort),tmpstrtok);
          tmpstrtok = strtok(NULL," ");
       }
    ocmjinfo_res.silent = atoi(JINFO_SILENT); 
    } else {
       memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
       sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field abort=%s\n",JINFO_ABORT);
       ocmjinfo_res.error = 1;
       return ocmjinfo_res; 
   }
 }

 if ( strcmp(JINFO_DEPEND, JINFO_NONE) != 0 ) {

    if ( strcmp(JINFO_RUN,"em") == 0 || strcmp(JINFO_RUN,"ew") == 0 ) {
       strcpy(JINFO_HOUR,"18");
       if (splitfield("depend", JINFO_DEPEND,tmp_result) == 0) {
          tmpstrtok = strtok(tmp_result," ");
          while ( tmpstrtok != NULL) {
                insertItem(&(ocmjinfo_res.depend),tmpstrtok);
                tmpstrtok = strtok(NULL," ");
	  }
       } else {
          memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
          sprintf(ocmjinfo_res.errormsg,"Error: omcjinfo: on splitfield passing parameter field depend=%s\n",JINFO_DEPEND);
          ocmjinfo_res.error = 1;
          return ocmjinfo_res;
       }
       strcpy(JINFO_HOUR,"");
    } else {
       if (splitfield("depend", JINFO_DEPEND,tmp_result) == 0) {
          tmpstrtok = strtok(tmp_result," ");
          while ( tmpstrtok != NULL) {
             insertItem(&(ocmjinfo_res.depend),tmpstrtok);
             tmpstrtok = strtok(NULL," ");
          }
       } else {
          memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
          sprintf(ocmjinfo_res.errormsg,"Error: omcjinfo: on splitfield passing parameter field depend=%s\n",JINFO_DEPEND);
          ocmjinfo_res.error = 1;
          return ocmjinfo_res;
       }
    }
  }

 if ( strcmp(JINFO_SUBMIT, JINFO_NONE) != 0 ) {
    if (splitfield("submit", JINFO_SUBMIT,tmp_result) == 0) {
       tmpstrtok = strtok(tmp_result," ");
       while ( tmpstrtok != NULL) {
          insertItem(&(ocmjinfo_res.submit),tmpstrtok);
          tmpstrtok = strtok(NULL," ");
       }
     } else {
        memset(ocmjinfo_res.errormsg,'\0',sizeof ocmjinfo_res.errormsg);
        sprintf(ocmjinfo_res.errormsg,"Error: ocmjinfo: on splitfield passing parameter field submit=%s\n",JINFO_SUBMIT);
        ocmjinfo_res.error = 1;
        return ocmjinfo_res;
     }
 }

 ocmjinfo_res.run = (char *) malloc(strlen(JINFO_RUN)+1);
 strcpy(ocmjinfo_res.run,JINFO_RUN);

 ocmjinfo_res.hour = (char *) malloc(strlen(JINFO_HOUR)+1);
 strcpy(ocmjinfo_res.hour,JINFO_HOUR);

 ocmjinfo_res.master = (char *) malloc(strlen(JINFO_MASTER)+1);
 strcpy(ocmjinfo_res.master,JINFO_MASTER);

 ocmjinfo_res.alias_job = (char *) malloc(strlen(JINFO_ALIAS_JOB)+1);
 strcpy(ocmjinfo_res.alias_job,JINFO_ALIAS_JOB);

 memset(tmp, '\0', sizeof tmp);
 sprintf(tmp,"%s %s %s %s %s", JINFO_ARGS, JINFO_LOOP_CURRENT, JINFO_LOOP_TOTAL, JINFO_HOUR,JINFO_CPU);
 ocmjinfo_res.args = (char *) malloc(strlen(tmp)+1);
 strcpy(ocmjinfo_res.args,tmp);

 fclose(jinfo_mastertable);
 fclose(jinfo_master);
 fclose(jinfo_aliastable);

 /* return the structure */
 return ocmjinfo_res;
}


/*****************************************************************************
* NAME: getrunhourandjob
* PURPOSE: set the following global variables 
*          JINFO_JOB,JINFO_HOUR,JINFO_LOOP_CURRENT, JINFO_RUN and JINFO_FLAG_LOOPS
*          using the infos from 'job'
* INPUT: job - jobname[_HH][_LLL]
*              where HH  - zulu [00,06,12,18]
*                    LLL - loop
*              note: HH is only required for runs r[1-2], g[1-6]
* OUTPUT: none.
* example: getrunhourandjob("e1start_12_0");
*******************************************************************************/
static void getrunhourandjob(job)
char job[JINFO_MAX_FIELD];
{
 /* check the last three characters of the jobname */
 /* for "_[00,06,12,18]. Then set run=first two  */
 /* and the last two characters. ie g1sefmd_00....run=g100 */
 /* and set JINFO_JOB to the characters before the _?? */

 /* check if i-2 = '_' */
 /* if it does then assume that the last two characters are */
 /* one of 00,06,12,18 */

 sscanf(job,"%[^_]_%[^_]_%[^_]",JINFO_JOB,JINFO_HOUR,JINFO_LOOP_CURRENT);

 sprintf(JINFO_RUN,"%.2s%.2s",JINFO_JOB,JINFO_HOUR);

 if ( strcmp(JINFO_LOOP_CURRENT,"") !=0 ) {
    JINFO_FLAG_LOOPS = 1;
 }
}

/*****************************************************************************
* NAME: getLoopFileInfos
* PURPOSE: set the following global variables 
*          JINFO_LOOP_TOTAL,JINFO_LOOP_SET,JINFO_LOOP_START and JINFO_LOOP_COLOR
*
* INPUT: run - 4 characters (ex: R112)
*        loopRef - 3 characters (ex: L01)
* OUTPUT: none.
* example: getLoopFileInfos("r112","L03");
*******************************************************************************/
static void getLoopFileInfos(char *run, char *loopRef)
{
 char filename[JINFO_MAXLINE];
 int oktoread=0;
 static FILE *f=NULL;
 char line[JINFO_MAXLINE];

 memset(filename,'\0',sizeof filename);
 sprintf(filename,"%s%s%s%s",JINFO_MASTERBASE,"loops_",run,loopRef);
 
 f = fopen(filename,"r");
 if ( f != NULL) {
     while (!oktoread) {   
         memset(line,'\0',sizeof line);
         if (fgets(line,JINFO_MAXLINE,f) != NULL) {
            if (strncmp("break",line,5) == 0) {
               oktoread = 1;
               memset(line,'\0',sizeof line);
               if (fgets(line,JINFO_MAXLINE,f) == NULL) {
                   printf("ocmjinfo: WARNING loop file is invalid %s\n",filename);
               } else {
                  sscanf(line,"%s %s %s %s",JINFO_LOOP_TOTAL,JINFO_LOOP_SET,JINFO_LOOP_START,JINFO_LOOP_COLOR);
               }
            }
        }
     }
     fclose(f);
 } else {
    fprintf(stderr,"ocmjinfo: cannot open=%s\n",filename);
    exit(1);
 }
}

/*****************************************************************************
* NAME: findjob_setvariables
* PURPOSE: set the following global variables 
*          JINFO_JOB,JINFO_CATCHUP,JINFO_CPU,JINFO_LOOP_REFERENCE,JINFO_WALLCLOCK,
*          JINFO_QUEUE, JINFO_ABORT, JINFO_DEPEND and JINFO_SUBMIT
*          JINFO_MACHINE_QUEUE, JINFO_MACHINE, JINFO_MPI, JINFO_MEMORY
*
* INPUT: job - jobname (ex: r1start_12)
* OUTPUT: none.
* example: findjob_setvariables("r1start_12");
*******************************************************************************/
static int findjob_setvariables(job)
char *job;
{
 int jobfound=0;
 char line[JINFO_MAXLINE];
 char tmpjob[JINFO_MAX_TMP];

 memset(tmpjob,'\0',sizeof tmpjob);

 while (jobfound == 0) {
    memset(line,'\0',sizeof line);
    if (fgets(line,JINFO_MAXLINE,jinfo_master) == NULL ) return(1);

    /* check for "job" in the first JINFO_JOBSIZE characters */

    memset(tmpjob,'\0',sizeof tmpjob);
    strncpy(tmpjob,line,JINFO_JOBSIZE);
    if ( strcmp(job, tmpjob) == 0 ) jobfound = 1;
 }

 /* line[1000] contains all the information about "job" */

 /* set the variables JINFO_JOB, JINFO_QUEUE, JINFO_ABORT, JINFO_DEPEND and JINFO_SUBMIT */

 /* JINFO_QUEUE is the second field */
 /* read the first field ...then some spaces */

 setfield(line,JINFO_JOB);
 setfield(line,JINFO_CATCHUP);
 setfield(line,JINFO_CPU);
 setfield(line,JINFO_MPI);
 setfield(line,JINFO_LOOP_REFERENCE);
 setfield(line,JINFO_WALLCLOCK);
 setfield(line,JINFO_MEMORY);
 setfield(line,JINFO_MACHINE_QUEUE);
 setfield(line,JINFO_ABORT);
 sprintf(JINFO_SILENT,"%s","0");
 if (strncmp(JINFO_ABORT,"silent",6) == 0) {
    sprintf(JINFO_ABORT,"%s",JINFO_ABORT+6);
    sprintf(JINFO_SILENT,"%s","1");
 }
 setfield(line,JINFO_DEPEND);
 setfield(line,JINFO_SUBMIT);
 return(0);
}

/*****************************************************************************
* NAME: setfield
* PURPOSE: read 'line' and set the value unto 'field'.
*
* INPUT: line - a line from the master files.
*               ex:"dbbrch1 2 1    REG 1   nccs    rerun:cont          none                       dbbrch2"
*        field - global variable  
* OUTPUT: field contains the value and JINFO_OFFSET is set to the last postition of the field.
* example: setfield("dbbrch1 2 1    REG 1   nccs    rerun:cont          none                       dbbrch2",JINFO_JOB);
*******************************************************************************/
static void setfield(line,field)
char line[JINFO_MAXLINE];
char field[JINFO_MAX_FIELD];
{
 int i, t=0;

 for (i = JINFO_OFFSET; i < strlen(line); i++) {
     /* the field may contain several arguments       */
     /* separated by ":". ie r0start:r0delet:r0fire   */
     /* A valid arg must be composed of alpha-numeric */
     /* characters                                    */

     if ( isalnum(line[i]) || line[i] == ':' || line[i] == '[' ||
        line[i] == '-' || line[i] == '+' || line[i] == ']' || line[i] == '.' ||
        line[i] == '_' || line[i] == '*' || line[i] == '@' || line[i] == '%' )
	{
         field[t] = line[i];
         t++;
         /* valid_field=1;*/
     } else {
	if (strlen(field) > 0 ) {
	   JINFO_OFFSET=i;
	   i=strlen(line);
	   field[i]='\0';
	}
     }
 }

 if (strlen(field) > 0 ) {
    i=strlen(line);
    field[i]='\0';
 }
}


/*****************************************************************************
* NAME: splitfield
* PURPOSE: split the string 'field' and put the result into 'result'.
*          'var' will tell us what type of field it is.
* INPUT: var - type of field: job, catchup, cpu, loop, wallclock, machine_queue, abort, depend, submit.
*        field - a string 
*       
* OUTPUT: result - a string with the formated values seperated by blanks.
* example: splitfield("submit","h1deriv:h1derma",result);
*******************************************************************************/
static int splitfield(var, field, result)
char var[JINFO_MAX_FIELD];
char field[JINFO_MAX_FIELD];
char result[JINFO_MAX_FIELD];
{
 int i, t=0,pos,tmp_set,tmp_step;
 int invalidextension=0;
 int minus_int;
 int loop_limit,set,start;
 unsigned short done = 0;
 unsigned short counter = 9;
 unsigned short tmphourCounter = 0;
 char tmp[JINFO_MAX_TMP];
 char tmpfield[JINFO_MAX_TMP];
 char tmpjob[JINFO_MAX_TMP];
 char tmp_depend[JINFO_MAX_TMP];
 char tmphour[5];
 char final_tmphour[5];
 char plusorminus;
 char fmt_s_term[JINFO_MAX_FMT];
 char fmt_term[JINFO_MAX_FMT];
 char fmt_s8_term[JINFO_MAX_FMT];
 char fmt_s8[JINFO_MAX_FMT];
 char fmt_s[JINFO_MAX_FMT];
 char tmp_result[JINFO_MAX_FIELD];
 char branches[JINFO_MAX_FIELD];
 char tempo[JINFO_MAX_TMP];
 char minus_str[10];
 char loop_str[10];
 char end_job[10];
 char run[3];
 char filter[JINFO_MAXLINE];
 char filter2[JINFO_MAXLINE];
 char user[JINFO_MAX_TMP];
 char controlDir[JINFO_MAX_TMP];
 char suite[JINFO_MAX_TMP];
 struct passwd *pw=NULL;
 char remote_job[JINFO_MAX_TMP];
 char between_suite[JINFO_MAX_TMP];
 char *tmpstrtok = NULL;
 char *ocmpath = NULL;
 char *ocmpath2 = NULL;

 memset(result,'\0', sizeof result);
 memset(tmp_result, '\0', sizeof tmp_result);
 memset(tmp, '\0', sizeof tmp);
 memset(fmt_term,'\0', sizeof fmt_term);
 memset(fmt_s_term,'\0', sizeof fmt_s_term);
 memset(fmt_s8_term,'\0', sizeof fmt_s8_term);
 memset(fmt_s8,'\0', sizeof fmt_s8);
 memset(fmt_s,'\0', sizeof fmt_s);

 strcpy(fmt_s_term, "%s\'\n");
 strcpy(fmt_term, "\'\n");
 strcpy(fmt_s8_term, "%s.%10.10s\'\n");
 strcpy(fmt_s8, "%s.%10.10s ");
 strcpy(fmt_s, "%s ");
 
 if ( strcmp(var,"job") == 0) {
    if (strlen(field) > 0) {
       memset(tmp, '\0', sizeof tmp);
       strcpy(tmp,field);
       if (strlen(JINFO_HOUR) >= 2 && strlen(JINFO_HOUR) <= 4) {
	  strcat(tmp,"_");
	  strcat(tmp,JINFO_HOUR);
       }
       if ( JINFO_FLAG_LOOPS ) {
           strcat(tmp,"_");
           strcat(tmp,JINFO_LOOP_CURRENT);
       }
    }
    strcpy(result,tmp);
    return(0);
 } /* var=job */

 if ( strcmp(var,"catchup") == 0 || strcmp(var,"cpu") == 0  || strcmp(var,"mpi") == 0 || strcmp(var,"loop") == 0 || strcmp(var,"wallclock") == 0 || strcmp(var,"memory") == 0 || strcmp(var,"machine_queue") == 0) {
    if (strlen(field) > 0) {
       memset(tmp, '\0', sizeof tmp);
       strcpy(tmp,field);      
    }
    strcpy(result,tmp);
    return(0);
 } /*var=catchup|cpu|mpi|loop|wallclock|memory|machine_queue*/

 if ( strcmp(var,"abort") == 0) {
    memset(tmp, '\0', sizeof tmp);
    t=0;
    for (i = 0; i < strlen(field); i++) {
       /* the field may contain several arguments       
        * separated by ":". ie rerun:stop   
        */
      if ((isalnum(field[i]) || field[i] == '_')) {
         tmp[t] = field[i];
      } else if (field[i] == ':') {
         tmp[t] = ' ';
      }
      t++; 
    }
    strcpy(result,tmp);
    return(0);
 } /*var=abort*/

 if ( strcmp(var,"submit") == 0) {
    tmpstrtok = strtok(field,":");
    while ( tmpstrtok != NULL) {
          memset(tmpfield,'\0', sizeof tmpfield);
          memset(tmp, '\0', sizeof tmp);
          strcpy(tmpfield,tmpstrtok);
          strcpy(tmp,tmpfield);
  	  if (strlen(JINFO_HOUR) == 2) {
	     strcat(tmp,"_");
	     strcat(tmp,JINFO_HOUR);
          }
          if ( getJobLoopInfos("NONE","NONE",tmp,&loop_limit,&set,&start) == 1 ) {
              if (JINFO_FLAG_LOOPS) {
                 strcat(tmp,"_");
	         strcat(tmp,JINFO_LOOP_CURRENT);
                 memset(tempo, '\0', sizeof tempo);
                 sprintf(tempo,fmt_s, tmp);
                 strcat(tmp_result,tempo);
              } else {
                 memset(tmpjob,'\0',sizeof(tmpjob));
                 strcpy(tmpjob,tmp);
                 memset(tmp,'\0', sizeof tmp);
	         for (tmp_set=0; tmp_set<set; tmp_set++) {
                      tmp_step=start + tmp_set;
                      if (tmp_step <= loop_limit) {
                          sprintf(tmp,"%s_%d",tmpjob,tmp_step);
                          memset(tempo, '\0', sizeof tempo);
                          sprintf(tempo,fmt_s, tmp);
                          strcat(tmp_result,tempo);
		          memset(tmp, '\0', sizeof tmp);
                      }
	         } /* end for */
	       }
	    } else {
               memset(tempo, '\0', sizeof tempo);
               sprintf(tempo,fmt_s, tmp);
               strcat(tmp_result,tempo);
            }
         tmpstrtok = strtok(NULL,":");
    } /* while */
    strcpy(result,tmp_result);
    return(0);   
 } /* var=submit */

 return(1);
}

/*****************************************************************************
* NAME: check4alias
* PURPOSE: check if 'job' is an alias job. If it's an alias job than
*          copy the alias name into JINFO_ALIAS_JOB else copy 'NULL'.
* INPUT: job - jobname
*       
* OUTPUT: result - JINFO_ALIAS_JOB is set with alias name
* example: check4alias("g1so300_12");
*******************************************************************************/
static void check4alias(job)
char *job;
{
 int i,found=0;
 char line[JINFO_MAXLINE];
 char tmpjobname[JINFO_MAX_FIELD];

 strcpy(tmpjobname,"");
 while (found==0 && (fgets(line,JINFO_MAXLINE,jinfo_aliastable) != NULL )) {
    if ( strncmp(job,line,strlen(job)) == 0 ) {
       /* found job in the aliastable */
       /* now get the real jobname and its arguments */
       /* jobname starts in column 11 */
       /* args (if any) are the rest of the line */
       for ( i=10;i<=17;++i) {
	   if ( isalnum(line[i]) ) {
	      tmpjobname[i-10]=line[i];
	   } else {
	      /* end of line */
	      i=17;
	      tmpjobname[i-10]='\0';
	   }
       }
       found=1;
       strcpy(JINFO_ARGS,&line[17]);
       if (JINFO_ARGS[strlen(JINFO_ARGS) - 1] == '\n') JINFO_ARGS[strlen(JINFO_ARGS) - 1] = '\0';
    }
 }
 memset(JINFO_ALIAS_JOB, '\0', sizeof JINFO_ALIAS_JOB);
 strcpy(JINFO_ALIAS_JOB,tmpjobname);
}

/*****************************************************************************
* NAME: getmaster
* PURPOSE: find the master file which contains 'run'
* INPUT: run - the run 
*        
* OUTPUT: return 0 if sucessfull and put result into 'masterfile'
* example: getmaster("r112",&masterfile); 
*******************************************************************************/
static int getmaster(char *controlDir, char *suite, char *run,char **masterfile)
{
 /* read the mastertable file to find the correct masterfile */
 int i,found=0;
 int returnstatus=1;
 char line[JINFO_MAXLINE];
 char tmpmaster[JINFO_MAXLINE];
 /*
 static FILE *jinfo_tmp;
 */

 static FILE *jinfo_tmp=NULL;

 if ( strcmp("NONE",controlDir) == 0 && strcmp("NONE",suite) == 0) {
    jinfo_tmp=jinfo_mastertable;
    fseek(jinfo_tmp,0L,SEEK_SET);
 } else {
    memset(tmpmaster,'\0',sizeof tmpmaster);
    sprintf(tmpmaster,"%s%s",controlDir,"mastertable");

    jinfo_tmp=fopen(tmpmaster,"r");
    if (jinfo_tmp == NULL ) {
      printf("ocmjinfo: cannot open file=%s\n",tmpmaster);
      return(1);
    }
 }

 memset(tmpmaster,'\0',sizeof tmpmaster);
 while (found==0) {
    if (fgets(line,JINFO_MAXLINE,jinfo_tmp) == NULL ) return(returnstatus);
    if ( strncmp(run,line,strlen(run)) == 0 ) {
       /* found run in the mastertable */
       /* now get the corresponding masterfile */
       if ( line[strlen(run)] == ' ' ) {
	  for ( i=7;i<JINFO_MAXLINE;++i) {
	      if ( isalnum(line[i]) || ispunct(line[i]) ) {
		 tmpmaster[i-7]=line[i];
	      } else {
		 /* end of line */
		 i=JINFO_MAXLINE;
		 tmpmaster[i-7]='\0';
	      }
	  }
	  found=1;
       } else {
	  return(2);
       }
   }
 }
 
 *masterfile = (char *) malloc(JINFO_MAXLINE);
 if ( strcmp("NONE",controlDir) == 0 && strcmp("NONE",suite) == 0) {
     sprintf(*masterfile,"%s%s",JINFO_MASTERBASE,tmpmaster);
 } else {
   fclose(jinfo_tmp);
   sprintf(*masterfile,"%s%s",controlDir,tmpmaster);
 }

 return(0);
}

/*****************************************************************************
* NAME: getJobLoopInfos
* PURPOSE: check if 'job' is a loop job or not.
* INPUT: job - jobname
*       
* OUTPUT: return 1 if it's a loop job
*         loop_limit - the total loop iterations or -999 for none loop job
*         set - the number of set or -999 for none loop job
*         start - the starting loop or -999 for none loop job
* example: getJobLoopInfos("r1start_12",&loop_limit,&set,&start);
*******************************************************************************/
static int getJobLoopInfos(char *controlDir, char *suite, char *job,int *loop_limit,int *set,int *start)
{
 /* read the mastertable file to find the correct masterfile */
 int found=0,oktoread;
 static FILE *fmaster=NULL;
 static FILE *floop=NULL;
 char line[JINFO_MAXLINE], line2[JINFO_MAXLINE];
 char *masterfilename=NULL;
 char loopfilename[JINFO_MAXLINE];
 char jobname[20], junk1[JINFO_MAXLINE], junk2[JINFO_MAXLINE], loopRef[JINFO_MAXLINE], cpu[JINFO_MAXLINE];
 int catchup, mpi, resu;
 char color[JINFO_MAX_TMP];
 char run[5];

 if (strlen(job) < 10 ) {
    /* where job is not does not have hour as extension */
    return(0);
 } else {
    memset(run,'\0',sizeof run);
    strncpy(run,job,2);
    strncat(run,job+8,2);
 }
 
 resu=getmaster(controlDir,suite,run,&masterfilename);

 if ( resu != 0 ) {
   printf("ABORT ocmjinfo: job=%s not in masterfile\n",job);
   return(0);
 }

 fmaster=fopen(masterfilename, "r");
 
 if ( fmaster == NULL) {
   printf("ABORT: ocmjinfo: can't open %s\n",masterfilename);
   return(0);
 }

 found=0;
 memset(jobname,'\0',sizeof jobname);
 strncpy(jobname,job,7);

 *loop_limit = -999;
 *set = -999;
 *start = -999;
 while (found==0) {
    if (fgets(line,JINFO_MAXLINE,fmaster) == NULL ){
        fclose(fmaster);
        return(-1);
    }
    if (strncmp(line,jobname,7) == 0) {
       memset(junk1,'\0', sizeof junk1);
       memset(junk2,'\0', sizeof junk2);
       sscanf(line,"%s %d %s %d %s %s",junk1,&catchup,cpu,&mpi,loopRef,junk2);
       if (strncmp(loopRef,"REG",3) != 0) {
	  /* must be a loop job */
          memset(loopfilename,'\0', sizeof loopfilename);
          if ( strcmp("NONE",controlDir) == 0 && strcmp("NONE",suite) == 0) {
              sprintf(loopfilename,"%s%s%s%s",JINFO_MASTERBASE,"loops_",run,loopRef);
          } else {
            sprintf(loopfilename,"%s%s%s%s",controlDir,"loops_",run,loopRef);
          }

          floop=fopen(loopfilename,"r");
          if (floop != NULL) {
             oktoread = 0;
             while (!oktoread) {   
                 memset(line2,'\0', sizeof line2);
                 if (fgets(line2,JINFO_MAXLINE,floop) != NULL) {
                    if (strncmp("break",line2,5) == 0) {
                       oktoread = 1;
                       memset(line2,'\0',sizeof line2);
                       if (fgets(line2,JINFO_MAXLINE,floop) == NULL) {
                           printf("ocmjinfo: loop file is invalid %s\n",loopfilename);
                       } else {
			 memset(color,'\0',sizeof color);
                         sscanf(line2,"%d %d %d %s",&(*loop_limit),&(*set),&(*start),color);
                       }
                     }
                 }
	     }

             fclose(floop);
          } else {
            printf("ocmjinfo: cannot open=%s\n",loopfilename);
            return(0);
          }         
       } else {
	  /* not a loop job */
          fclose(fmaster);
          return(0);
       }
       found=1;
    }
 }
  fclose(fmaster);
  return(1);
}

/*****************************************************************************
* NAME: convert_tmphour
* PURPOSE: add "$plusorminus$tmphour" to $JINFO_HOUR and put result into 'final_tmphour'
* INPUT: plusorminus - '-' or '+'
*        tmphour - '00', '06', '12' or '18'
*       
* OUTPUT: return 0 if successful and put result into 'final_tmphour'
* example: convert_tmphour('+',"06",final_tmphour);
*******************************************************************************/
static int convert_tmphour(char plusorminus,char tmphour[5],char final_tmphour[5])
{
 int status = 1;
 int numer, denom = 24 , answer;
 div_t ans;

 if ( plusorminus == '+' ) {
    status=0;
    numer = atoi(tmphour) + atoi(JINFO_HOUR);
    ans = div(numer,denom);
    sprintf(final_tmphour,"%02d",ans.rem); 
 }
 if ( plusorminus == '-' ) {
    status=0;
    numer = atoi(tmphour);
    ans = div(numer,denom);
    answer = 24 + atoi(JINFO_HOUR) - ans.rem;
    ans = div(answer,denom);
    sprintf(final_tmphour,"%02d", ans.rem);
 }
 return(status);
}

/**********************************************************************
***S/R		message                                                   
*
*AUTHOR		W. Hodgins                                               
*
*REVISION	Version 3.0 Apr 9 1990                                  
*
*LANGUAGE	C                                                       
*
*OBJECT		Display a message in english or french (depending  
*		on environment variable CMCLNG)
*
*USAGE		message(msg1,msg2,var)                                 
*
*ARGUMENTS	msg1 - english text to be displayed
*		msg2 - french text to be displayed
*		var  - string of characters to be displayed
*
*        
**********************************************************************/
static void message (msg1,msg2,var)
char msg1[100],msg2[100],var[100];
{
 int eng = 1;
 char *str = NULL;

 str = getenv("CMCLNG");

 if (str != NULL) {
    if(!strcmp(str,"francais"))eng = 0;
 }
 if (eng) fprintf(stderr,"%s%s\n",msg1,var);
 else fprintf(stderr,"%s%s\n",msg2,var);

}




