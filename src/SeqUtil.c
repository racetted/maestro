/* SeqUtil.c - Basic utilities used by the Maestro sequencer software package.
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
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <glob.h>
#include <errno.h>        /* errno */
#include "SeqUtil.h"
#include <time.h>  /*nsleep*/
#include "SeqNameValues.h"
#include "SeqListNode.h"
#include "l2d2_commun.h"

/* used by normpath function */ 
#define ispathsep(ch)   ((ch) == '/' || (ch) == '\\')
#define iseos(ch)       ((ch) == '\0')
#define ispathend(ch)   (ispathsep(ch) || iseos(ch)) 
#define COMP_MAX 256 
int SEQ_TRACE = 0;

void raiseError(const char* fmt, ... );

#define SEQ_MAXFILES 100


struct mappedFile{
  char* filename;  
  char* filestart; /* First char of file */
  char* fileend;   /* One byte past the end of file (so that end - start = size)*/
};

struct TraceFlags{
  int importance;
  int timeStamp;
  int otherInfo;
};
static struct TraceFlags traceFlags = { TL_CRITICAL, TF_OFF, TF_OFF};

/* Maybe have def files and cfg files to speedup the lookup process */
static struct mappedFile mappedFiles[SEQ_MAXFILES];
static int nbMappedFiles = 0;

/******************************************************************************** 
 * Trace function.  Message will only be output if messageImportance is superior
 * or equal to the importance set by the environment or the caller of the
 * executable.  Other information may be added to the message depending on flags
 * also set by the environment variable SEQ_TRACE_LEVEL
 *******************************************************************************/
void SeqUtil_TRACE(int messageImportance ,const char * fmt, ... ){

	char message[SEQ_MAXFIELD],prefix[SEQ_MAXFIELD];
	va_list ap;
	 
	memset( message, '\0', sizeof(message));
	memset( prefix, '\0', sizeof(prefix));

	/* Return if message in not important enough */
	if (messageImportance < traceFlags.importance) 
		return;

	/* Put formatted message in a string */
	va_start (ap, fmt);
	vsprintf(message,fmt,ap); 
	va_end (ap);

	/* Add appropriate info corresponding to the set flags */
	if ( traceFlags.timeStamp ){
		get_time( prefix, 3 );
		strcat(prefix, "  ");
	}
	if ( traceFlags.otherInfo )
		strcat(prefix,"OtherInfo " /*other info */);
	strcat(prefix,message);

	/* Print message */
	fprintf(stderr, "%s", prefix);
}

/********************************************************************************
 * Function used set tracing information based on the environment variable
 * SEQ_TRACE_LEVEL
 *******************************************************************************/
void SeqUtil_setTraceEnv(){
	extern struct TraceFlags traceFlags;

	/* Set Default values */
	traceFlags.importance = TL_CRITICAL;
	traceFlags.timeStamp = TF_OFF;

	/* Obtenir et copier la variable d'environnement */
	char seq_trace_level[SEQ_MAXFIELD];
	strcpy(seq_trace_level, getenv("SEQ_TRACE_LEVEL"));
	if( seq_trace_level == NULL ){
		SeqUtil_TRACE( TL_FULL_TRACE, "SeqUtil_setTraceEnv(): Unable to find SEQ_TRACE_LEVEL environment variable \n" );
		return;
	}

	/* Parse string and set appropriate flags */
	char separator[2] = ":";
	char * token = strtok(seq_trace_level, separator);
	while(token != NULL)
	{
		/* Treat token */
		if(strcmp(token,"FULL") == 0 ){
			traceFlags.importance = TL_FULL_TRACE;
			traceFlags.timeStamp = TF_ON;
			return;
		} else if ( strcmp(token, "TL_FULL_TRACE") == 0 ){
			traceFlags.importance = TL_FULL_TRACE;
		} else if ( strcmp(token, "TL_MEDIUM" ) == 0 ){
			traceFlags.importance = TL_MEDIUM;
		} else if ( strcmp(token, "TL_ERROR" ) == 0 ){
			traceFlags.importance = TL_ERROR;
		} else if ( strcmp(token, "TL_CRITICAL") == 0 ){
			traceFlags.importance = TL_CRITICAL;
		} else if ( strcmp(token, "TF_TIMESTAMP") == 0 ){
			traceFlags.timeStamp = TF_ON;
		} else {
			SeqUtil_TRACE(TL_FULL_TRACE, "Invalid flag %s in SEQ_TRACE_LEVEL \n", token);
		} 
		/* Advance token */
		token = strtok(NULL, separator);
	}
}
/********************************************************************************
 * Function used to set trace flags by the program.  Look in SeqUtil.h for the
 * values of the macros.
 *******************************************************************************/
void SeqUtil_setTraceFlag(int flag, int value)
{
	switch(flag){
		case TF_TIMESTAMP:
			traceFlags.timeStamp = value;
			break;
		case TRACE_LEVEL:
			traceFlags.importance = value;
			break;
		default:
			SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil_setTraceFlat(): Invalid Flag \n");
	}
}

/********************************************************************************
 * Returns the value of the traceFlags importance attribute. 
 *******************************************************************************/
int SeqUtil_getTraceLevel () {
   return traceFlags.importance;
}

/********************************************************************************
 * Returns the string to use as SET_TRACE_LEVEL environment variable.  This is
 * equivalent to getting the environment variable itself if it is set, but if it
 * is not set, this will give the SEQ_TRACE_LEVEL that sets the default values
 * as they are when struct TraceFlags traceFlags is initialized.
 *******************************************************************************/
char *SeqUtil_getTraceLevelString(){
   static char traceString[100];
   switch(traceFlags.importance){
      case TL_FULL_TRACE:
         strcpy(traceString, "TL_FULL_TRACE");
         break;
      case TL_MEDIUM:
         strcpy(traceString, "TL_MEDIUM");
         break;
      case TL_ERROR:
         strcpy(traceString, "TL_ERROR");
         break;
      case TL_CRITICAL:
         strcpy(traceString, "TL_CRITICAL");
         break;
   }
   if( traceFlags.timeStamp == TF_ON )
      strcat(traceString, ":TF_TIMESTAMP");

   return traceString;
}

void SeqUtil_showTraceInfo(){
	fprintf(stderr , "Setting trace level to ");
	switch(traceFlags.importance){
		case TL_FULL_TRACE:
			fprintf(stderr , "TL_FULL_TRACE \n");
			break;
		case TL_MEDIUM:
			fprintf(stderr , "TL_MEDIUM \n");
			break;
		case TL_ERROR:
			fprintf(stderr , "TL_ERROR \n");
			break;
		case TL_CRITICAL:
		default:
			fprintf(stderr , "TL_CRITICAL \n");
			break;
	}
}

void raiseError(const char* fmt, ... ) {
   va_list ap;
   va_start (ap, fmt);
   printf( "!!!!!!!!!!!!!!!!!!!!!!!!!!!! APPLICATION ERROR !!!!!!!!!!!!!!!!!!!!!!!!!!!!\n" );
   vfprintf (stderr, fmt, ap);
   va_end (ap);
   printf( "Program exiting with status 1!\n" );

   exit(1);
}

void SeqUtil_checkExpHome (char * _expHome) {

   DIR *dirp = NULL;

   if ( _expHome != NULL ) {
      dirp = opendir(_expHome);
      if (dirp == NULL) {
         raiseError("ERROR: invalid SEQ_EXP_HOME=%s\n",_expHome);
      }
      closedir(dirp);
   } else {
      raiseError("ERROR: SEQ_EXP_HOME not set!\n");
   }

}


/********************************************************************************
*actions: print action message
********************************************************************************/
void actions(char *signal, char* flow, char *node) {
 SeqUtil_TRACE(TL_FULL_TRACE,"\n**************** SEQ \"%s\" \"%s\" \"%s\" Action Summary *******************\n",signal, flow, node);
}

/********************************************************************************
*actions: print action message
********************************************************************************/
void actionsEnd(char *signal, char* flow, char* node) {
 SeqUtil_TRACE(TL_FULL_TRACE,"\n**************** SEQ \"%s\" \"%s\" \"%s\" Action ENDS *******************\n",signal, flow, node);
}

/********************************************************************************
*removeFile_nfs: Removes the named file 'filename'; it returns zero if succeeds 
* and a nonzero value if it does not
********************************************************************************/
int removeFile_nfs(const char *filename, const char * _seq_exp_home) {
   int status=0;

   SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil.removeFile_nfs() removing %s\n", filename );
   status = remove(filename);
   return(status);
}


/********************************************************************************
*access_nfs: access the named file 'filename'; it returns zero if succeeds 
* and a nonzero value if it does not
********************************************************************************/
int access_nfs (const char *filename , int stat, const char * _seq_exp_home ) {
   int status=0;

   SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil.access_nfs() accessing %s\n", filename);
   status = access(filename, stat);
   return (status);
}
 




/********************************************************************************
*touch_nfs: simulate a "touch" on a given file 'filename'
********************************************************************************/
int touch_nfs(const char *filename, const char * _seq_exp_home) {
   FILE *actionfile;
   
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil.touch_nfs(): filename=%s\n",filename);

   if ((actionfile = fopen(filename,"r")) != NULL ) {
      fclose(actionfile);
      utime(filename,NULL); /*set the access and modification times to current time */     
   } else {
      if ((actionfile = fopen(filename,"w+")) == NULL) {
         fprintf(stderr,"Error: maestro cannot touch_nfs file:%s\n",filename);
         return(1);
      }
      fclose(actionfile);
   }
   return(0); 
}

/* isFileExists_nfs
* returns 1 if succeeds, 0 failure 
*/
int isFileExists_nfs( const char* lockfile, const char *caller, const char * _seq_exp_home ) {
    if ( access(lockfile, R_OK) == 0 ) {
       SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil.isFileExists_nfs() caller:%s found lock file=%s\n", caller, lockfile ); 
       return 1;
    }
    SeqUtil_TRACE(TL_MEDIUM, "SeqUtil.isFileExists_nfs() caller:%s missing lock file=%s\n", caller, lockfile ); 
    return 0;
}
 
/**
* fopen_nfs
* use link to make waited_end file available to host
*/
FILE * fopen_nfs (const char *path, int sock )
{
    FILE *fp;
    if ( (fp=fopen (path, "r")) != NULL )  return(fp);
    raiseError("fopen_nfs Cannot open waited file, aborting.");
}



int SeqUtil_isDirExists( const char* path_name ) {
   DIR *pDir = NULL;
   int bExists = 0;

   if ( path_name == NULL) return 0;
   pDir = opendir (path_name);

   if (pDir != NULL)
   {
      bExists = 1;
      (void) closedir (pDir);
   }

   return bExists;
}


/* 
* nfs Wrapper to glob function for
 * pathnames matching
*/
int globPath_nfs (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno), const char * _seq_exp_home) 
{
    glob_t glob_p;
    int ret;
    /* The real glob */
    ret = glob(pattern, GLOB_NOSORT,  0 , &glob_p);
    switch (ret) {
        case GLOB_NOSPACE:
            SeqUtil_TRACE(TL_ERROR, "SeqUtil.globPath_nfs() Glob running out of memory \n"); 
            return(0);
            break;
        case GLOB_ABORTED:
            SeqUtil_TRACE(TL_ERROR, "SeqUtil.globPath_nfs() Glob read error \n" ); 
            return(0);
            break;
        case GLOB_NOMATCH:
            SeqUtil_TRACE(TL_ERROR, "SeqUtil.globPath_nfs() Glob no found matches \n"); 
            globfree(&glob_p);
            return(0);
            break;/* not reached */
     }

     ret=glob_p.gl_pathc;
     globfree(&glob_p);
     return (ret);
}

/* 
* nfs Wrapper to glob function to return list of extensions found by pattern with a * wildcard. 
*/
LISTNODEPTR globExtList_nfs (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno)) 
{
    
    glob_t glob_p;
    size_t ret=0, wildcardLocation=0, totalfiles=0, filecounter=0;
    char * wildcardPtr=NULL;
    char * tmpString=NULL;
    LISTNODEPTR extensionList=NULL; 
    SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil.globExtList_nfs() looking for pattern %s \n",pattern); 

    wildcardPtr=strchr(pattern,'*'); 
    if (wildcardPtr==NULL) return NULL;
    /* location used to know where the new patterns will start in the glob return strings */
    wildcardLocation=wildcardPtr-pattern ;  
    
    /* The real glob */
    ret = glob(pattern, GLOB_NOSORT,  0 , &glob_p);
    switch (ret) {
        case GLOB_NOSPACE:
            SeqUtil_TRACE(TL_MEDIUM, "SeqUtil.globExtList_nfs() Glob running out of memory \n"); 
            return(0);
            break;
        case GLOB_ABORTED:
            SeqUtil_TRACE(TL_MEDIUM, "SeqUtil.globExtList_nfs() Glob read error \n" ); 
            return(0);
            break;
        case GLOB_NOMATCH:
            SeqUtil_TRACE(TL_MEDIUM, "SeqUtil.globExtList_nfs() Glob no found matches \n"); 
            globfree(&glob_p);
            return(0);
            break;/* not reached */
     }

     totalfiles=glob_p.gl_pathc;
     for (filecounter=0; filecounter<totalfiles; ++filecounter) {
         /*file return format should be /path/to/files/filename.*.some_state, and * should replace a "+index" where index can be a string or a number */
         /* TODO NPT ^last... remove from string here? */
         SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil.globExtList_nfs() iteration found, Target: %s (length=%d); wildcardLocation=%d, wildcardPtr=%s (length %d); \n",glob_p.gl_pathv[filecounter], strlen(glob_p.gl_pathv[filecounter]), wildcardLocation, wildcardPtr, strlen(wildcardPtr)); 

         tmpString=strndup(glob_p.gl_pathv[filecounter]+wildcardLocation, strlen(glob_p.gl_pathv[filecounter]) - strlen(pattern) +1); 
         SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil.globExtList_nfs() iteration found. Extension: %s \n",tmpString); 
         /* temporary removal to compile other test
         */         
          SeqListNode_insertItem(&extensionList,tmpString);
          free(tmpString); tmpString=NULL; 
     }
     globfree(&glob_p);
     return (extensionList);
}


char *SeqUtil_getPathLeaf (const char *full_path) {
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

char *SeqUtil_getPathBase (const char *full_path) {
  char *split = NULL ,*work_string = NULL ,*chreturn =NULL; 
  work_string = strdup(full_path);
  split = strrchr (work_string,'/');
  if (split != NULL) {
    *split = '\0';
    chreturn = strdup (work_string);
  }
  free( work_string );
  return chreturn;
}


/**
  * Create directory hierarchy
  *
  */
int SeqUtil_mkdir_nfs( const char* dir_name, int is_recursive, const char * _seq_exp_home ) {
   char tmp[1000];
   char *split = NULL, *work_string = NULL; 
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil_mkdir: dir_name %s recursive? %d \n", dir_name, is_recursive );
   if ( is_recursive == 1) {
      work_string = strdup( dir_name );
      strcpy( tmp, "/" );
      split  = strtok( work_string, "/" );
      if( split != NULL ) {
         strcat( tmp, split );
      }
      while ( split != NULL ) {
         strcat( tmp, "/" );
         if( ! SeqUtil_isDirExists( tmp ) ) {
            SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil_mkdir: creating dir %s\n", tmp );
            if ( mkdir( tmp, 0755 ) == -1 ) {
               fprintf ( stderr, "ERROR: %s\n", strerror(errno) );
               return(EXIT_FAILURE);
            }
         }

         split = strtok (NULL,"/");
         if( split != NULL ) {
            strcat( tmp, split );
         }
      }
   } else {
      if( ! SeqUtil_isDirExists( dir_name ) ) {
         if ( mkdir( dir_name, 0755 ) == -1 ) {
            SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil_mkdir: creating dir %s\n", dir_name );
            fprintf ( stderr, "ERROR: %s\n", strerror(errno) );
            return(EXIT_FAILURE);
         }
      }
   }
   free( work_string );
   return(0);
}


char *SeqUtil_cpuCalculate( const char* npex, const char* npey, const char* omp, const char* cpu_multiplier ){
  char *chreturn=NULL;
  int nMpi=1;
  if ( ! (chreturn = malloc( strlen(npex) + (npey==NULL || strlen(npey)) + (omp==NULL || strlen(omp)) + strlen(cpu_multiplier) + 1 ) )){
    SeqUtil_TRACE( TL_CRITICAL,"SeqUtil_cpuCalculate malloc: Out of memory!\n");
    return(NULL);
  }
  nMpi = atoi(npex) * atoi(cpu_multiplier);
  if ( npey != NULL){ nMpi = nMpi * atoi(npey); }
  sprintf(chreturn,"%d",nMpi);
  if ( omp != NULL){ sprintf(chreturn,"%sx%s",chreturn,omp); }
  return(chreturn);
}

char *SeqUtil_cpuCalculate_new( const char* npex, const char* npey, const char* omp, const char* cpu_multiplier  ){
  char *chreturn=NULL;
  int nMpi=1, totalsize=0;

  if (npex!=NULL) totalsize+=strlen(npex); 
  if (npey!=NULL) totalsize+=strlen(npey); 
  if (omp!=NULL)  totalsize+=strlen(omp); 
  if (cpu_multiplier!=NULL) totalsize+=strlen(omp); 
 
  if ( ! (chreturn = malloc( totalsize + 1 ) )){
    SeqUtil_TRACE(TL_CRITICAL, "SeqUtil_cpuCalculate(): malloc: Out of memory!\n");
    return(NULL);
  }
  nMpi = atoi(npex) * atoi(cpu_multiplier);
  if ( npey != NULL){ nMpi = nMpi * atoi(npey); }
  sprintf(chreturn,"%d",nMpi);
  if ( omp != NULL && atoi(omp) != 1 ) { sprintf(chreturn,"%sx%s",chreturn,omp); }
   
  return(chreturn);
}

/* dynamic string cat, content of source is freed */
void SeqUtil_stringAppend( char** source,const char* data )
{
   char* newDataPtr = NULL;
   /*do not change source if data is null*/
   if (data != NULL) {
      if ( *source != NULL ) {
         if( ! (newDataPtr = malloc( strlen(*source) + strlen( data ) + 1 )  )) {
            SeqUtil_TRACE(TL_CRITICAL, "SeqUtil_stringAppend malloc: Out of memory!\n"); 
	    return;
         }
         strcpy( newDataPtr, *source );
         strcat( newDataPtr, data );
      } else {
         if( ! (newDataPtr = malloc( strlen( data ) + 1 ) ) ) {
            SeqUtil_TRACE(TL_CRITICAL, "SeqUtil_stringAppend malloc: Out of memory!\n"); 
   	    return;
         }
         strcpy( newDataPtr, data );
      }

      free( *source );
      *source = newDataPtr;
  }
}

char *SeqUtil_resub (const char *regex_text, const char *repl_text, const char *str)
{
    static char buffer[4096];
    const int n_matches = 50, MAX_ERROR_MSG=100;
    regmatch_t m[n_matches];
    regex_t r;

    int status = regcomp( &r, regex_text, REG_EXTENDED|REG_NEWLINE );
    if (status != 0) {
      char error_message[MAX_ERROR_MSG];
      regerror( status, &r, error_message, MAX_ERROR_MSG );
      SeqUtil_TRACE(TL_ERROR,"SeqUtil_regcomp error compiling '%s': %s\n", regex_text, error_message);
      return NULL;
    }
    strncpy(buffer,str,strlen(str));
    while (1) {
        int i = 0;
        int nomatch = regexec( &r, buffer, n_matches, m, 0 );
        if (nomatch) {
	  regfree( &r );
	  return buffer;
        }
        for (i = 0; i < n_matches; i++) {
            int start;
	    int finish;
            if (m[i].rm_so == -1) {
                break;
            }
	    start = m[i].rm_so;
	    finish = m[i].rm_eo;
	    sprintf( buffer+start, "%s%s", repl_text, buffer+finish );
        }
    }
    regfree( &r );
    return NULL;
}

int SeqUtil_tokenCount( const char* source, const char* tokenSeparator )
{
   int count = 0;
   char *tmpSource = NULL, *tmpstrtok = NULL; 

   tmpSource = (char*) malloc( strlen( source ) + 1 );
   strcpy( tmpSource, source );
   tmpstrtok = (char*) strtok( tmpSource, tokenSeparator );

   while ( tmpstrtok != NULL ) {
        count++;
        tmpstrtok = (char*) strtok(NULL, tokenSeparator);
   }

   free(tmpSource);
   return count;
}

/* fixes the path name and returns the new path
removes '/' at the end and ands '/' at beginning if not there
*/
char* SeqUtil_fixPath ( const char* source ) {
   char *working = NULL, *new = NULL;
   int len = 0;
   SeqUtil_stringAppend( &new, "" );
   if( source == NULL || (len = strlen( source )) == 0 ) {
      return new;
   }
   working = strdup( source );
   if( source[0] != '/' ) {
      SeqUtil_stringAppend( &new, "/" );
   }
   if( source[len-1] == '/' ) {
      working[len-1] = '\0';
   }
   SeqUtil_stringAppend( &new, working );
   free( working );
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqUtil_fixPath source:%s new:%s\n", source, new );
   return new;
}


/* waits for the presence of a file (checking every timeInterval) or max time elapses */

void SeqUtil_waitForFile (char* filename, int secondsLimit, int timeInterval) {

   struct timespec interval;
   int counter = 0; 

   while (access(filename, R_OK) != 0) {
      interval.tv_sec = timeInterval;
      if (nanosleep(&interval, NULL) == -1) {
         raiseError("SeqUtil_waitForFile timer interrupted, aborting.");
      } else {
         counter=counter+timeInterval; 
         if (counter >= secondsLimit){ 
            raiseError("SeqUtil_waitForFile timed out, aborting.");
         }
      }
   }
} 

char* SeqUtil_getdef( const char* filename, const char* key , const char* _seq_exp_home) {
  char *retval=NULL,*home=NULL,*ovpath=NULL,*ovext="/.suites/overrides.def", *defpath=NULL, *defext="/.suites/default_resources.def";
  struct passwd *passwdEnt;
  struct stat fileStat;

  /* Use ownership of the suite to determine path to overrides.def */
  if (stat(_seq_exp_home,&fileStat) < 0){
     raiseError("SeqUtil_getdef unable to stat SEQ_EXP_HOME\n");
  }

  passwdEnt = getpwuid(fileStat.st_uid);
  home = passwdEnt->pw_dir;
  ovpath = (char *) malloc( strlen(home) + strlen(ovext) + 1 );
  defpath = (char *) malloc( strlen(home) + strlen(defext) + 1 );
  sprintf( ovpath, "%s%s", home, ovext );
  sprintf( defpath, "%s%s", home, defext );
  SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_getdef(): looking for definition of %s in %s\n",key,ovpath);
  if ( (retval = SeqUtil_parsedef(ovpath,key)) == NULL ){
    SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_getdef(): looking for definition of %s in %s\n",key,filename);
    if ( (retval = SeqUtil_parsedef(filename,key)) == NULL ){
       SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_getdef(): looking for definition of %s in %s\n",key,defpath);
       retval = SeqUtil_parsedef(defpath,key);
    }
  } 
  free(ovpath);
  return retval;
}

/********************************************************************************
 * Function that manages memory mapped files.  We verify if the file is already 
 * in memory.  
 * If it is not, the file is opened and mapped into memory.
 * In either case, a pointer is returned to where the file resides in memory.
 * NOTE: The variable may need to be changed from static to global 
 * to allow another function to do cleanup.  That or have an extra parameter so
 * that will tell the function to do unmap on all the files.
********************************************************************************/
int SeqUtil_getmappedfile(const char *filename, char ** filestart , char** fileend){
  extern struct mappedFile mappedFiles[SEQ_MAXFILES];
  extern nbMappedFiles;
  int status = 0;

  /* Find the file if it is mapped */
  int i;
  for(i = 0; i < nbMappedFiles; ++i) {
    /* If the file is found, return pointer to where it is mapped and filesize */
    if(strcmp(mappedFiles[i].filename, filename) == 0) {
      SeqUtil_TRACE( TL_FULL_TRACE, "getmappedfile(): File %s was found in mmapped files\n",filename);
      *filestart = mappedFiles[i].filestart;
      *fileend = mappedFiles[i].fileend;
      return 0;
    }
  }

  /* if it is not found, open it, map it, close it,add it to the list and return
   * the pointers. */

  /* Open the file  */
  int fd = open(filename,O_RDONLY|O_NONBLOCK);
  if(fd == -1){
    SeqUtil_TRACE( TL_FULL_TRACE, "SeqUtil_getmappedfile(): Could not open file %s for definition lookup \n", filename);
    return 1;
  }

  /* Get the size of the file */
  struct stat fileStat;
  if(fstat(fd,&fileStat) == -1){
    SeqUtil_TRACE(TL_ERROR,"SeqUtil_getmappedfile(): Error getting stats for file %s \n", filename);
    return 1;
  }

  /* Map the file and close it */
  char* addr = mmap(NULL, fileStat.st_size, PROT_READ, MAP_SHARED, fd, 0); 
  close(fd);
  if(addr == MAP_FAILED){
    SeqUtil_TRACE(TL_ERROR,"SeqUtil_getmappedfile(): Error mapping file %s into memory \n", filename);
    return 1;
  }
  SeqUtil_TRACE( TL_FULL_TRACE,"getmappedfile(): Mapped file %s into memory using file descriptor %d \n", filename, fd);

  /* Add it to the list */
  /* Filename */
  mappedFiles[nbMappedFiles].filename = (char*)malloc(strlen(filename) + 1);
  strcpy(mappedFiles[nbMappedFiles].filename, filename);
  /* Address */
  *filestart = mappedFiles[nbMappedFiles].filestart = addr; 
  /* Filesize */
  *fileend = mappedFiles[nbMappedFiles].fileend = addr + fileStat.st_size;
  ++nbMappedFiles;

  return 0 ;
}

/********************************************************************************
 * Function for cleanup on program end: Unmaps all the files that were mapped
 * into memory. 
********************************************************************************/
void SeqUtil_unmapfiles()
{
  extern struct mappedFile mappedFiles[SEQ_MAXFILES];
  extern int nbMappedFiles;
  int filesize;

  while(nbMappedFiles-- > 0)
  {
    filesize = mappedFiles[nbMappedFiles].fileend - mappedFiles[nbMappedFiles].filestart;
    munmap(mappedFiles[nbMappedFiles].filestart, filesize);
  }
}

/********************************************************************************
 * Reads one line from a source into line of length size
 * Returns -1 if the line to copy is too big for the destination 
********************************************************************************/
int readline(char *line, int size, char* source)
{
  unsigned long int i = 0;
  register char c;
  while((c = source[i]) != '\n'&& c != '\0') {
    line[i++] = c;
    if(i >= size) {
      raiseError("readline(): Buffer is too small to receive line from source\n");
      return -1;
    }
  }
  line[i] = '\0';
  return i;
}

/********************************************************************************
 * parser for .def simple text definition files (free return pointer in caller)
 * Returns NULL if definition was not found 
********************************************************************************/
char* SeqUtil_parsedef( const char* filename, const char* key ) {
  char *retval=NULL;
  unsigned long int iterator=0;
  char line[SEQ_MAXFIELD],defkey[SEQ_MAXFIELD],defval[SEQ_MAXFIELD];
  char *filestart;
  char *fileend;
  
  int status = SeqUtil_getmappedfile(filename, &filestart, &fileend);
  if(status == 1){
    SeqUtil_TRACE( TL_FULL_TRACE, "SeqUtil_parsdef failed to open/map file %s \n", filename);
    return NULL;
  }

  char *current = filestart;
  /* long int to help the compiler because we will be adding it to a 64 bit pointer */
  long int readsize = 0;

  do{
    /* Read a line in memory starting at current */
    readsize = readline(line, SEQ_MAXFIELD, current);
    if(readsize < 0)
      return NULL; /* ( RaiseError will have been called in readline() ) */
    current += readsize + 1;
    /* If the string is a comment skip to next */
    if ( strstr( line, "#" ) != NULL ) {
      continue;
    }

    /* Parse the string: put left of "=" into defkey and right into defval */
    if ( sscanf( line, " %[^= \t] = %[^\n \t]", defkey, defval ) == 2 ){
      if ( strcmp( key, defkey ) == 0 ) {
        /* Copy the string into retval for the caller and return it. */
        if ( ! (retval = (char *) malloc( strlen(defval)+1 )) ) {
          raiseError("SeqUtil_parsedef malloc: Out of memory!\n");
        }
        strcpy(retval,defval);
        SeqUtil_TRACE( TL_FULL_TRACE, "SeqUtil_parsedef(): found definition %s=%s in %s\n",defkey,retval,filename);
        return retval;
      }
    }
    defkey[0] = '\0';
    defval[0] = '\0';
  }while(current < fileend);

  /* Return NULL if nothing was found as an error code for the caller */
  return NULL;
}

/* Substitutes a ${.} formatted keyword in a string. To use a definition file (format defined by
   SeqUtils_getdef(), provide the _deffile name; a NULL value passed to _deffile 
   causes the resolver to search in the environment for the key definition.  If 
   _srcfile is NULL, information about the str source is not printed in case of an error.*/
char* SeqUtil_keysub( const char* _str, const char* _deffile, const char* _srcfile ,const char* _seq_exp_home) {
  char *strtmp=NULL,*substr=NULL,*var=NULL,*env=NULL,*post=NULL,*source=NULL;
  char *saveptr1,*saveptr2;
  static char newstr[SEQ_MAXFIELD];
  int start,isvar;

  if (_deffile == NULL){
    SeqUtil_stringAppend( &source, "environment" );}
  else{
    SeqUtil_stringAppend( &source, "definition" );
  }
  SeqUtil_TRACE(TL_FULL_TRACE,"XmlUtils_resolve(): performing %s replacements in string \"%s\"\n",source, _str);

  strtmp = (char *) malloc( strlen(_str) + 1 ) ;
  strcpy(strtmp,_str);
  substr = strtok_r(strtmp,"${",&saveptr1);
  start=0;
  while (substr != NULL){
    isvar = (strstr(substr,"}") == NULL) ? 0 : 1;
    var = strtok_r(substr,"}",&saveptr2);
    if (strcmp(source,"environment") == 0){
      env = getenv(var);}
    else{
      env = SeqUtil_getdef(_deffile,var,_seq_exp_home);
    }
    post = strtok_r(NULL,"\0",&saveptr2);
    if (env == NULL){
      if (isvar > 0){
	      raiseError("Variable %s referenced by %s but is not set in %s\n",var,_srcfile,source);}
      else{
	      strncpy(newstr+start,var,strlen(var));
	      start += strlen(var);
      }
    }
    else {
      SeqUtil_TRACE(TL_FULL_TRACE,"XmlUtils_resolve(): replacing %s with %s value \"%s\"\n",var,source,env);
      strncpy(newstr+start,env,strlen(env));
      start += strlen(env);
    }
    if (post != NULL){
      strncpy(newstr+start,post,strlen(post));
      start += strlen(post);
    }
    newstr[start]=0;
    substr = strtok_r(NULL,"${",&saveptr1);
  }
  free(source);
  free(strtmp);
  return newstr;
}  

/* remove ^last from extension if it's in there */
char* SeqUtil_striplast( const char* str ) {
  char *noLast=NULL;
  int stringLength;

  stringLength = strlen( str );
  if (strstr( str, "^last" )){
    stringLength -= 5;
  }
  noLast = malloc( stringLength+1 ); 
  memset( noLast,'\0', stringLength+1 );
  strncpy( noLast, str, stringLength ); 
  SeqUtil_stringAppend( &noLast, "" );
  return( noLast );
}

/*removes first occurance of the substring from a string, modifies the string argument */
void SeqUtil_stripSubstring( char ** string, char * substring) {
 
  int substringPos=0, stringLength=0, substringLength=0; 
  char * tmpString =NULL , * tmpSub = NULL; 
  

  if ((*string != NULL) && (substring != NULL) && (strcmp(substring,"") != 0)){
      stringLength=strlen(*string);
      substringLength=strlen(substring);
      
      tmpSub=strstr(*string, substring);
      if (tmpSub != NULL) {
         substringPos=stringLength - strlen(tmpSub);
         if (! (tmpString=malloc(stringLength - substringLength + 1 )) ){
            raiseError("SeqUtil_stripSubstring malloc: Out of memory!\n"); 
         }
         strncpy(tmpString,*string,substringPos);
         strcat(tmpString,*string+substringPos+substringLength); 

         free(*string);
         *string = tmpString; 
      }
  }
  SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_stripSubstring(): resulting string %s \n",string);
} 


/* returns an memory allocated string of a path whose ".", ".." and "..." keywords have been replaced with the following
 * . -> current container path
 * .. -> parent of the container, can be used more than once to go up the tree
 * ... -> module level, can only be used once */

char* SeqUtil_relativePathEvaluation( char* path, SeqNodeDataPtr _nodeDataPtr) { 

   char *returnString = NULL, *tmpString=NULL, *tmpstrtok=NULL, *prevPtr=NULL; 
   int count=0;

   if (path == NULL) return NULL; 
   if (strstr(path, "..") != NULL || strstr(path, "./") != NULL) {
   /* contains keywords to be replaced */
        /*check which case we're refering to*/
       if ((tmpString=strstr(path, "...")) !=NULL) {
            if (! (returnString=malloc(strlen(path) - 3  + strlen(_nodeDataPtr->pathToModule) + 1 )) ){
               raiseError("SeqUtil_relativePathEvaluation() malloc: Out of memory!\n"); 
	    }
	         strcpy(returnString,_nodeDataPtr->pathToModule);
            strcat(returnString,path+3);
            SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_relativePathEvaluation(): module keyword replacement: replacing %s with %s\n",path,returnString);
	         if (strstr(returnString,"...") != NULL) {
               raiseError("SeqUtil_relativePathEvaluation(): \"...\" keyword should only occur once. Not permitted to go beyond a single module level. Check your dep_name = %s", path);
	    }
	    
       } else if ((tmpString=strstr(path, "..")) !=NULL) {
            /*count the number of containers to go up*/
            count=SeqUtil_tokenCount( path, ".." ); 
	         returnString=strdup(_nodeDataPtr->container);
            tmpString = (char*) malloc( strlen( path ) + 1 );
            strcpy( tmpString, path );
            tmpstrtok = (char*) strtok( tmpString, ".." );
	         while (tmpstrtok != NULL ) {
 		          returnString=SeqUtil_getPathBase(returnString);
                prevPtr=tmpstrtok; 
                tmpstrtok = (char*) strtok( NULL, ".." );
	         }
            SeqUtil_stringAppend(&returnString,prevPtr);
            SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_relativePathEvaluation(): parent keyword replacement: replacing %s with %s\n",path,returnString);
            free(tmpString);
       } else if ((tmpString=strstr(path, "./")) !=NULL) {

            if (! (returnString=malloc(strlen(path) - 1  + strlen(_nodeDataPtr->container) + 1 )) ){
               raiseError("SeqUtil_stripSubstring malloc: Out of memory!\n"); 
	    }
	    strcpy(returnString,_nodeDataPtr->container);
            strcat(returnString,path+1); 
            SeqUtil_TRACE(TL_FULL_TRACE,"SeqUtil_relativePathEvaluation(): container replacement: replacing %s with %s\n",path,returnString);

	    if (strstr(returnString,"./") != NULL) {
               raiseError("SeqUtil_relativePathEvaluation(): \"./\" keyword should only occur once. Check your dep_name = %s", path );
	    }
       }

   } else {
   /* doesn't contain keywords, just create a copy of the input string and return */
      returnString=strdup(path);
   }
   
   return returnString; 
} 

/*
 * WriteNodeWaitedFile_nfs
 * Writes (nfs) the dependency lockfile in the directory of the node that this current node is waiting for.
 * 
 * Inputs:
 *    _dep_exp_path - the SEQ_EXP_HOME of the dependant node
 *    _dep_node     - the path of the node including the container
 *    _dep_status   - the status that the node is waiting for (end,abort,etc)
 *    _dep_index    - the loop index that this node is waiting for (.+1+6)
 *    _dep_scope    - dependency scope
 *     Sfile        - Status file of the dependent node
 *     xpinode      - Experiment Inode
 */ 

int WriteNodeWaitedFile_nfs ( const char* seq_xp_home, const char* nname, const char* datestamp,  const char* loopArgs,
                              const char* filename, const char* statusfile) {
 
    FILE *waitingFile = NULL;
    char tmp_line[SEQ_MAXFIELD];
    char line[SEQ_MAXFIELD];
    char Lexp[256],Lnode[256],Ldatestamp[25],LloopArgs[128];
    int n,found=0;
    size_t num, inode, Linode;
 
    fprintf(stderr,"maestro.writeNodeWaitedFile(): Using WriteNodeWaitedFile_nfs routine\n");

    memset(tmp_line,'\0',sizeof(tmp_line));
    memset(line,'\0',sizeof(line));
    memset(Lexp,'\0',sizeof(Lexp));
    memset(Lnode,'\0',sizeof(Lnode));
    memset(Ldatestamp,'\0',sizeof(Ldatestamp));
    memset(LloopArgs,'\0',sizeof(LloopArgs));

    /* if cannot get inode of xp, skip this dependency */
    if   ( (inode=get_Inode(seq_xp_home)) < 0 ) {
             fprintf(stderr,"writeNodeWaitedFile_nfs: Cannot get Inode of xp=%s\n",seq_xp_home);
             return (1);
    }

    /* be carfull here, argument a will position both read and write pointer at the end, 
       while a+ will position read at beginning and write at the end.
       NOTE: the file is open through NFS from different machines, we dont know the exact 
             behaviour of the append command */
    if ((waitingFile = fopen(filename,"a+")) == NULL) {
            raiseError( "maestro.WriteNodeWaitedFile_nfs cannot write to file:%s\n",filename );
    }

    SeqUtil_TRACE(TL_FULL_TRACE, "maestro.writeNodeWaitedFile_nfs updating %s\n", filename);

    /* sua   : need to add more logic for duplication and handle more than one entry in the waited file 
       Rochdi: we added comparaison of xp inode:  /.suites vs /maestro_suites (ie real case) */
    while( fgets(line, SEQ_MAXFIELD, waitingFile) != NULL ) {
           n=sscanf(line,"exp=%s node=%s datestamp=%s args=%s",Lexp,Lnode,Ldatestamp,LloopArgs);
           if ( (Linode=get_Inode(Lexp)) < 0 ) {
                   fprintf(stderr,"writeNodeWaitedFile_nfs: Cannot get Inode of registred xp=%s\n",Lexp);
                   continue;
           }
           if ( Linode == inode && strcmp(Lnode,nname) == 0 && strcmp(Ldatestamp,datestamp) == 0 && strcmp(LloopArgs,loopArgs) == 0 ) {
                  found = 1;
                  break;
           }

    }

    if ( !found ) {
             snprintf( tmp_line, sizeof(tmp_line), "exp=%s node=%s datestamp=%s args=%s\n",seq_xp_home, nname, datestamp, loopArgs );
             /* fprintf( waitingFile,"%s", tmp_line );  */
	     num = fwrite(tmp_line ,sizeof(char) , strlen(tmp_line) , waitingFile);
	     if ( num != strlen(tmp_line) )  fprintf(stderr,"writeNodeWaitFile Error: written:%zu out of:%zd \n",num,strlen(tmp_line));
    }

    fclose( waitingFile );
    return(0);
}
/**
*
*
*/
int  WriteInterUserDepFile_nfs (const char *filename , const char * DepBuf , const char *ppwdir, const char* maestro_version, 
                                const char *datestamp, const char *md5sum)
{
     char buff[1024];
     char DepFileName[1024];
     FILE *fp=NULL;
     int ret;


     if ((fp = fopen(filename,"w")) == NULL) {
               raiseError( "WriteInterUserDepFile_nfs: Cannot write to interUser dependency file:%s\n",filename );
     }

     fwrite(DepBuf, 1, strlen(DepBuf) , fp);
     fclose(fp);

     /* create server dependency directory (based on maestro version) 
     * Note: multiple client could try to create this when  do not exist. Give a static inode for this action */
     snprintf(buff, sizeof(buff), "%s/.suites/maestrod/dependencies/polling/v%s",ppwdir,maestro_version);
   
     if ( access(buff,R_OK ) != 0 ) {
          if ( SeqUtil_mkdir_nfs ( buff , 1, NULL) != 0 ) { 
             raiseError( "WriteInterUserDepFile_nfs: Could not create dependency directory:%s",buff );
          }
     }

     /* build dependency filename and link it to true dependency file under the xp. tree */
     snprintf(DepFileName,sizeof(DepFileName),"%s/.suites/maestrod/dependencies/polling/v%s/%s_%s",ppwdir,maestro_version,datestamp,md5sum);

     /* have to check for re-runs */
     ret=symlink(filename,DepFileName); /* atomic */

     return(0);
}

/*
 * WriteForEachFile_nfs
 * Writes (nfs) the for each lockfile in the directory of the node that this current node is waiting for.
 * 
 * Inputs:
 *    _exp            - the SEQ_EXP_HOME of the forEach node
 *    _node           - the path of the ForEach node including the container
 *    _datestamp         - the datestamp of the FE node
 *    _target_index       - the loop index name expected to be filled up by FE target 
 *    _loopArgs          - the name=value,name=value string of loop arguments belonging to the forEach node.
 *    _filename          - filename where info will be located
 * Output:
 *    int status 
 *    file created in target area
 */ 

int WriteForEachFile_nfs ( const char* _exp, const char* _node, const char* _datestamp, const char * _target_index, const char* _loopArgs,
                              const char* _filename) {
 
    FILE *waitingFile = NULL;
    char tmp_line[SEQ_MAXFIELD];
    char line[SEQ_MAXFIELD];
    char Lexp[256],Lnode[256],Ldatestamp[25],LloopArgs[128], Lindex[128];
    int n,found=0;
    size_t inode, Linode, num;
 
    SeqUtil_TRACE(TL_FULL_TRACE,"WriteNodeWaitedFile(): Using WriteForEachFileFile_nfs routine\n");
    memset(tmp_line,'\0',sizeof(tmp_line));
    memset(line,'\0',sizeof(line));
    memset(Lexp,'\0',sizeof(Lexp));
    memset(Lnode,'\0',sizeof(Lnode));
    memset(Ldatestamp,'\0',sizeof(Ldatestamp));
    memset(LloopArgs,'\0',sizeof(LloopArgs));
    memset(Lindex,'\0',sizeof(Lindex));

    /* if cannot get inode of xp, skip this dependency */
    if   ( (inode=get_Inode(_exp)) < 0 ) {
             fprintf(stderr,"writeForEachFile_nfs: Cannot get Inode of xp=%s\n",_exp);
             return (1);
    }

    /* be carfull here, argument a will position both read and write pointer at the end, 
       while a+ will position read at beginning and write at the end.
       NOTE: the file is open through NFS from different machines, we dont know the exact 
             behaviour of the append command */
    if ((waitingFile = fopen(_filename,"a+")) == NULL) {
            raiseError( "WriteForEachFile_nfs cannot write to file:%s\n",_filename );
    }

    SeqUtil_TRACE(TL_FULL_TRACE, "writeForEachFile_nfs updating %s\n", _filename);

    /* sua   : need to add more logic for duplication and handle more than one entry in the waited file 
       Rochdi: we added comparaison of xp inode:  /.suites vs /maestro_suites (ie real case) */
    while( fgets(line, SEQ_MAXFIELD, waitingFile) != NULL ) {
           n=sscanf(line,"exp=%s node=%s datestamp=%s index_to_add=%s args=%s",Lexp,Lnode,Ldatestamp,Lindex, LloopArgs);
           if ( (Linode=get_Inode(Lexp)) < 0 ) {
                   fprintf(stderr,"writeNodeWaitedFile_nfs: Cannot get Inode of registred xp=%s\n",Lexp);
                   continue;
           }
           SeqUtil_TRACE(TL_FULL_TRACE, "writeForEachFile_nfs checking inodes: %zu vs %zu, %s vs %s, %s vs %s, %s vs %s, %s vs %s\n", Linode, inode, Lnode, _node, Ldatestamp, _datestamp, LloopArgs, _loopArgs, Lindex, _target_index);
           if ( Linode == inode && strcmp(Lnode,_node) == 0 && strcmp(Ldatestamp,_datestamp) == 0 && strcmp(LloopArgs,_loopArgs) == 0 && strcmp(Lindex,_target_index) == 0 ) {
                  found = 1;
                  break;
           }
    }
    if ( !found ) {
         snprintf( tmp_line, sizeof(tmp_line), "exp=%s node=%s datestamp=%s index_to_add=%s args=%s\n",_exp, _node, _datestamp, _target_index,_loopArgs );
	      num = fwrite(tmp_line ,sizeof(char) , strlen(tmp_line) , waitingFile);
	      if ( num != strlen(tmp_line) )  {
            fprintf(stderr,"writeForEachFile Error: written:%zu out of:%zd \n",num,strlen(tmp_line));
            fclose( waitingFile ); 
            return(1); 
         }
    }

    fclose( waitingFile );
    return(0);
}


void SeqUtil_printOrWrite( FILE * tmpFile, char * text, ...) {

   va_list ap;
   va_start(ap, text); 

   if (tmpFile != NULL) {
      vfprintf(tmpFile, text, ap);
   } else {
      vfprintf(stdout, text, ap);
   }
   va_end(ap);
}

/**
 * build a symlink on sequencing/sync/$datestamp/$version
 *
 */
int lock_nfs ( const char * filename , const char * datestamp, const char * _seq_exp_home ) 
{
    char lpath[1024], src[1024], dest[1024], Ltime[25];
     char *mversion, *md5Token ;
     int i,ret=0;
     struct stat st;
     time_t now;
     double diff_t;


     if ( _seq_exp_home == NULL ) {
	            fprintf(stderr,"lock_nfs: Cannot bad SEQ_EXP_HOME received as argument ...\n");
     }

     if ( (mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
                     fprintf(stderr, "lock_nfs: Cannot get env. var SEQ_MAESTRO_VERSION\n");
     }
     
     snprintf(lpath,sizeof(lpath),"%s/sequencing/sync/%s/v%s",_seq_exp_home,datestamp,mversion);
     if ( access(lpath,R_OK) != 0 ) ret=SeqUtil_mkdir_nfs(lpath , 1,NULL);

     snprintf(src,sizeof(src),"%s/END_TASK_LOCK",lpath);
     if ( access(src,R_OK) != 0 ) {
                 if ( (ret=touch_nfs(src,_seq_exp_home)) != 0 ) fprintf(stderr,"cannot touch file:lock on %s \n",lpath);
     }

     md5Token = (char *) str2md5(filename,strlen(filename));
     snprintf(dest,sizeof(dest),"%s/%s",lpath,md5Token);

     for ( i=0 ; i < 5 ; i++ ) {
          get_time(Ltime,3);
          ret=symlink("END_TASK_LOCK",dest);
          if ( ret == 0 )  {
                 fprintf(stdout,"symlink obtained loop=%d AT:%s xpn=%s Token:%s\n",i,Ltime,_seq_exp_home,md5Token);
                 break;
          }
          usleep(500000);
     }
     
     if ( ret != 0 ) {
          if ( (stat(dest,&st)) == 0 ) {
                time(&now);
                if ( (diff_t=difftime(now,st.st_mtime)) > 5 ) {
                       ret=unlink(dest);
                       fprintf(stderr,"symlink timeout xpn=%s Token:%s\n",_seq_exp_home,md5Token);
                }
          }
     }
    free(md5Token);

     return(ret);
}

/*
* remove a symlink in sequencing/sync/$datestamp/$version
*/
int unlock_nfs ( const char * filename , const char * datestamp , const char * _seq_exp_home) 
{
     char lpath[1024], src[1024], Ltime[25];
     char *mversion, *md5Token ;
     int ret=0;

     if ( _seq_exp_home == NULL ) {
	            fprintf(stderr,"lock_nfs: bad SEQ_EXP_HOME received as argument\n");
     }

     if ( (mversion=getenv("SEQ_MAESTRO_VERSION")) == NULL ) {
                     fprintf(stderr, "lock_nfs: Cannot get SEQ_MAESTRO_VERSION variable\n");
     }
     
     snprintf(lpath,sizeof(lpath),"%s/sequencing/sync/%s/v%s",_seq_exp_home,datestamp,mversion);
     if ( access(lpath,R_OK) != 0 ) {
             fprintf(stderr,"unlock_nfs: dir=%s not exist\n",lpath);
	     return(1);
     }
    
     md5Token = (char *) str2md5(filename,strlen(filename));
     snprintf(src,sizeof(src),"%s/%s",lpath,md5Token);

     get_time(Ltime,3);
     if ( access(src,R_OK) == 0 ) {
         if ( (ret=unlink(src)) != 0 ) {
               fprintf(stderr,"unlink error:%d AT:%s xpn=%s Token:%s\n",ret,Ltime,_seq_exp_home,md5Token);
               ret=1;
         } else {
               fprintf(stdout,"symlink released AT:%s xpn=%s Token:%s\n",Ltime,_seq_exp_home,md5Token);
         }
     } else {
               fprintf(stdout,"unlock_nfs: file not there:%s !!!\n",src);
               ret=1;
     }

     free(md5Token);
     return(ret);

}

/* Returns the average of the input array, but sorting it, and removing the removal_quantity of extreme values from each side. */

int SeqUtil_basicTruncatedMean(int *unsorted_int_array, int elements, int removal_quantity) {
   int total=0; 
   int i;
   if (elements <= 2*removal_quantity) return 0; 
   qsort (unsorted_int_array, elements, sizeof(int), SeqUtil_compareInt);
   for(i=removal_quantity; i < elements-removal_quantity; ++i) { 
       total+=unsorted_int_array[i];
   } 
   return (total/(elements-2*removal_quantity)); 
}

/* Returns the integer differences, used for sorting algorithm qsort. */


int SeqUtil_compareInt (const void * a, const void * b)
{
  return ( *(int*)a - *(int*)b );
}

/* taken from https://gist.github.com/starwing/2761647 . Will not remove trailing slashes (so can be used in conjunction with SeqUtil_fixPath) */
/* will return a normalized path, removing relative notations and additional slashes */
char * SeqUtil_normpath(char *out, const char *in) {
    char *pos[COMP_MAX], **top = pos, *head = out;
    int isabs = ispathsep(*in);

    if (isabs) *out++ = '/';
    *top++ = out;

    while (!iseos(*in)) {
        while (ispathsep(*in)) ++in;

        if (iseos(*in))
            break;

        if (memcmp(in, ".", 1) == 0 && ispathend(in[1])) {
            ++in;
            continue;
        }

        if (memcmp(in, "..", 2) == 0 && ispathend(in[2])) {
            in += 2;
            if (top != pos + 1)
                out = *--top;
            else if (isabs)
                out = top[-1];
            else {
                strcpy(out, "../");
                out += 3;
            }
            continue;
        }

        if (top - pos >= COMP_MAX)
            return NULL; /* path too long */

        *top++ = out;
        while (!ispathend(*in))
            *out++ = *in++;
        if (ispathsep(*in))
            *out++ = '/';
    }
    *out = '\0';
    if (*head == '\0')
        strcpy(head, "./");
    return head;
}



