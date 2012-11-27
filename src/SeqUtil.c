#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>        /* errno */
#include "SeqUtil.h"
#include <time.h>  /*nsleep*/
#include "SeqNameValues.h"
#include "SeqListNode.h"


int SEQ_TRACE = 0;

void raiseError(const char* fmt, ... );


/********************************************************************************
SeqUtil_trace




********************************************************************************/

void SeqUtil_TRACE (char * fmt, ...) {
   
   va_list ap;
   if ( SEQ_TRACE != 0 ) {
      va_start (ap, fmt);
      vfprintf (stdout, fmt, ap);
      va_end (ap);
   }
}

void SeqUtil_setTraceLevel (int _trace) {
   SEQ_TRACE = _trace;
}

int SeqUtil_getTraceLevel () {
   return SEQ_TRACE;
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
 SeqUtil_TRACE("\n**************** SEQ \"%s\" \"%s\" \"%s\" Action Summary *******************\n",signal, flow, node);
}

/********************************************************************************
*actions: print action message
********************************************************************************/
void actionsEnd(char *signal, char* flow, char* node) {
 SeqUtil_TRACE("\n**************** SEQ \"%s\" \"%s\" \"%s\" Action ENDS *******************\n",signal, flow, node);
}

/********************************************************************************
*genFileList: scan a directory 'directory' and return a list of files 'filelist'
*  using the the filter 'filters'.
********************************************************************************/
int genFileList(LISTNODEPTR *fileList,const char *directory,LISTNODEPTR *filterList) {

LISTNODEPTR tmplist=NULL;
LISTNODEPTR tmpfilters=NULL;
char *filter=NULL;
DIR *dirp=NULL;
struct dirent *direntp=NULL;

 direntp=(struct dirent *) malloc(sizeof(struct dirent));

 tmpfilters=*filterList;

 filter = (char *) malloc(strlen(tmpfilters->data)+1);
 strcpy(filter,tmpfilters->data);

 while (filter != NULL) {
    SeqUtil_TRACE("maestro.genFileList() opening directory=%s trying to match pattern %s\n",directory, filter);
    dirp = opendir(directory);
    if (dirp == NULL) {
       fprintf(stderr,"maestro: invalid directory path %s\n",directory);
       *fileList = NULL;
       return(1);
    }
    while ( (direntp = readdir(dirp)) != NULL) {
       if (match(direntp->d_name,filter) == 1) {
          SeqUtil_TRACE("maestro.genFileList() found file matching=%s\n",direntp->d_name );
          SeqListNode_insertItem(&tmplist,direntp->d_name);
       }
    }
    closedir(dirp);
    free(filter);
    tmpfilters=tmpfilters->nextPtr;
    if (tmpfilters != NULL) {
       filter = (char *) malloc(strlen(tmpfilters->data)+1);
       strcpy(filter,tmpfilters->data);
    } else {
       filter=NULL;
    }
 }
 free(direntp);
 *fileList = tmplist;
 return(0);
}

/********************************************************************************
*removeFile: Removes the named file 'filename'; it returns zero if succeeds 
* and a nonzero value if it does not
********************************************************************************/
int removeFile(const char *filename) {
   int status=0;

   SeqUtil_TRACE( "maestro.removeFile() removing %s\n", filename );
   status = remove(filename);
   return(status);
}


/********************************************************************************
*touch: simulate a "touch" on a given file 'filename'
********************************************************************************/
int touch(const char *filename) {
   FILE *actionfile;
   
   SeqUtil_TRACE("maestro.touch(): filename=%s\n",filename);

   if ((actionfile = fopen(filename,"r")) != NULL ) {
      fclose(actionfile);
      utime(filename,NULL); /*set the access and modification times to current time */     
   } else {
      if ((actionfile = fopen(filename,"w+")) == NULL) {
         fprintf(stderr,"Error: maestro cannot touch file:%s\n",filename);
         return(1);
      }
      fclose(actionfile);
      chmod(filename,00664);
   }
   return(0); 
}



/* returns 1 if succeeds, 0 failure */
int isFileExists( const char* lockfile, const char *caller ) {
   if ( access(lockfile, R_OK) == 0 ) {
      /* SeqUtil_TRACE( "SeqUtil.isFileExists() caller:%s found lock file=%s\n", caller, lockfile ); */
      SeqUtil_TRACE( "caller:%s found lock file=%s\n", caller, lockfile );
      return 1;
   }
   /* SeqUtil_TRACE( "SeqUtil.isFileExists() caller:%s missing lock file=%s\n", caller, lockfile ); */
   SeqUtil_TRACE( "caller:%s missing lock file=%s\n", caller, lockfile );
   return 0;
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

int SeqUtil_mkdir( const char* dir_name, int is_recursive ) {
   char tmp[1000];
   char *split = NULL, *work_string = NULL; 
   SeqUtil_TRACE ( "SeqUtil_mkdir: dir_name %s recursive? %d \n", dir_name, is_recursive );
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
            SeqUtil_TRACE ( "SeqUtil_mkdir: creating dir %s\n", tmp );
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
            SeqUtil_TRACE ( "SeqUtil_mkdir: creating dir %s\n", dir_name );
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
    SeqUtil_TRACE( "SeqUtil_cpuCalculate malloc: Out of memory!\n");
    return(NULL);
  }
  nMpi = atoi(npex) * atoi(cpu_multiplier);
  if ( npey != NULL){ nMpi = nMpi * atoi(npey); }
  sprintf(chreturn,"%d",nMpi);
  if ( omp != NULL){ sprintf(chreturn,"%sx%s",chreturn,omp); }
  return(chreturn);
}

/* dynamic string cat, content of source is freed */
void SeqUtil_stringAppend( char** source, char* data )
{
   char* newDataPtr = NULL;
   /*do not change source if data is null*/
   if (data != NULL) {
      if ( *source != NULL ) {
         if( ! (newDataPtr = malloc( strlen(*source) + strlen( data ) + 1 )  )) {
            SeqUtil_TRACE( "SeqUtil_stringAppend malloc: Out of memory!\n"); 
	    return;
         }
         strcpy( newDataPtr, *source );
         strcat( newDataPtr, data );
      } else {
         if( ! (newDataPtr = malloc( strlen( data ) + 1 ) ) ) {
            SeqUtil_TRACE( "SeqUtil_stringAppend malloc: Out of memory!\n"); 
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
      SeqUtil_TRACE ("SeqUtil_regcomp error compiling '%s': %s\n", regex_text, error_message);
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
   SeqUtil_TRACE( "SeqUtil_fixPath source:%s new:%s\n", source, new );
   return new;
}


/* validates username + exp against the current user and current
exp and returns the path of the suite */
/* 2010-10-21... not used anymore, should be removed later... */
char* SeqUtil_getExpPath( const char* username, const char* exp ) {
   char *home = NULL, *expPath = NULL, *currentUser = NULL;
   char *currentExpPath = getenv("SEQ_EXP_HOME");
   struct passwd* currentPasswd = NULL;
   currentPasswd = (struct passwd*) getpwuid(getuid());
   currentUser = strdup( currentPasswd->pw_name );

   SeqUtil_stringAppend( &expPath, "" );
   if( strcmp( (char*) username, currentUser) == 0 ) {
      home = getenv("HOME");
      SeqUtil_stringAppend( &expPath, home );
      SeqUtil_stringAppend( &expPath, "/.suites/" );
      SeqUtil_stringAppend( &expPath, (char*) exp );
   }
   SeqUtil_TRACE( "SeqUtil_getExpPath returning value: %s\n", expPath );
   return expPath;
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

char* SeqUtil_getdef( const char* filename, const char* key ) {
  char *retval=NULL,*home=NULL,*ovpath=NULL,*ovext="/.suites/overrides.def";
  char *seq_exp_home=NULL;
  struct passwd *passwdEnt;
  struct stat fileStat;

  /* User ownership of the suite to determine path to overrides.def */
  if ( (seq_exp_home = getenv("SEQ_EXP_HOME")) == NULL ){
    raiseError("SeqUtil_getdef $SEQ_EXP_HOME not defined\n");
  }
  if (stat(seq_exp_home,&fileStat) < 0){
    raiseError("SeqUtil_getdef unable to stat SEQ_EXP_HOME\n");
  }
  passwdEnt = getpwuid(fileStat.st_uid);
  home = passwdEnt->pw_dir;
  ovpath = (char *) malloc( strlen(home) + strlen(ovext) + 1 );
  sprintf( ovpath, "%s%s", home, ovext );
  SeqUtil_TRACE("SeqUtil_getdef(): looking for definition of %s in %s\n",key,ovpath);
  if ( (retval = SeqUtil_parsedef(ovpath,key)) == NULL ){
    SeqUtil_TRACE("SeqUtil_getdef(): looking for definition of %s in %s\n",key,filename);
    retval = SeqUtil_parsedef(filename,key); 
  } 
  free(ovpath);
  return retval;
}

/* parser for .def simple text definition files (free return pointer in caller)*/
char* SeqUtil_parsedef( const char* filename, const char* key ) {
  FILE* deffile;
  char *retval=NULL;
  char line[SEQ_MAXFIELD],defkey[SEQ_MAXFIELD],defval[SEQ_MAXFIELD];

  if ( (deffile = fopen( filename, "r" )) != NULL ){
    memset(defkey,'\0',sizeof defkey);
    memset(defval,'\0',sizeof defval);
    while ( fgets( line, sizeof(line), deffile ) != NULL ) {
      if ( strstr( line, "#" ) != NULL ) {
	continue;
      }
      if ( sscanf( line, " %[^= ] = %s ", defkey, defval ) == 2 ){	
	if ( strcmp( key, defkey ) == 0 ) {
	  fclose(deffile);
	  if ( ! (retval = (char *) malloc( strlen(defval)+1 )) ) {
	    raiseError("SeqUtil_parsedef malloc: Out of memory!\n");
	  }
	  strcpy(retval,defval);
	  SeqUtil_TRACE("SeqUtil_parsedef(): found definition %s=%s in %s\n",defkey,retval,filename);
	  return retval;
	}
      }
      defkey[0] = '\0';
      defval[0] = '\0';
    }
    fclose(deffile);}
  else{
    SeqUtil_TRACE("SeqUtil_parsedef(): unable to open definition file %s\n",filename);
  }
  return NULL;
}

/* Substitutes a ${.} formatted keyword in a string. To use a definition file (format defined by
   SeqUtils_getdef(), provide the _deffile name; a NULL value passed to _deffile 
   causes the resolver to search in the environment for the key definition.  If 
   _srcfile is NULL, information about the str source is not printed in case of an error.*/
char* SeqUtil_keysub( const char* _str, const char* _deffile, const char* _srcfile ) {
  char *strtmp=NULL,*substr=NULL,*var=NULL,*env=NULL,*post=NULL,*source=NULL;
  char *saveptr1,*saveptr2;
  static char newstr[SEQ_MAXFIELD];
  int start,isvar;

  if (_deffile == NULL){
    SeqUtil_stringAppend( &source, "environment" );}
  else{
    SeqUtil_stringAppend( &source, "definition" );
  }
  SeqUtil_TRACE("XmlUtils_resolve(): performing %s replacements\n",source);

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
      env = SeqUtil_getdef(_deffile,var);
    }
    post = strtok_r(NULL," ",&saveptr2);
    if (env == NULL){
      if (isvar > 0){
	raiseError("Variable %s referenced by %s but is not set in %s\n",var,_srcfile,source);}
      else{
	strncpy(newstr+start,var,strlen(var));
	start += strlen(var);
      }
    }
    else{
      SeqUtil_TRACE("XmlUtils_resolve(): replacing %s with %s value %s\n",var,source,env);
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
  SeqUtil_TRACE("SeqUtil_stripSubstring(): resulting string %s \n",string);
} 


/* returns an memory allocated string of a path whose ".", ".." and "..." keywords have been replaced with the following
 * . -> current container path
 * .. -> parent of the container, can be used more than once to go up the tree
 * ... -> module level, can only be used once */

char* SeqUtil_relativePathEvaluation( char* path, SeqNodeDataPtr _nodeDataPtr) { 

   char *returnString = NULL, *tmpString=NULL, *tmpstrtok=NULL, *prevPtr=NULL; 
   int count=0;

   if (strstr(path, ".") != NULL) {
   /* contains keywords to be replaced */
        /*check which case we're refering to*/
       if ((tmpString=strstr(path, "...")) !=NULL) {
            if (! (returnString=malloc(strlen(path) - 3  + strlen(_nodeDataPtr->pathToModule) + 1 )) ){
               raiseError("SeqUtil_relativePathEvaluation() malloc: Out of memory!\n"); 
	    }
	    strcpy(returnString,_nodeDataPtr->pathToModule);
            strcat(returnString,path+3);
            SeqUtil_TRACE("SeqUtil_relativePathEvaluation(): module keyword replacement: replacing %s with %s\n",path,returnString);
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
            SeqUtil_TRACE("SeqUtil_relativePathEvaluation(): parent keyword replacement: replacing %s with %s\n",path,returnString);
            free(tmpString);
       } else {

            if (! (returnString=malloc(strlen(path) - 1  + strlen(_nodeDataPtr->container) + 1 )) ){
               raiseError("SeqUtil_stripSubstring malloc: Out of memory!\n"); 
	    }
	    strcpy(returnString,_nodeDataPtr->container);
            strcat(returnString,path+1); 
            SeqUtil_TRACE("SeqUtil_relativePathEvaluation(): container replacement: replacing %s with %s\n",path,returnString);

	    if (strstr(returnString,".") != NULL) {
               raiseError("SeqUtil_relativePathEvaluation(): \".\" keyword should only occur once. Check your dep_name = %s", path );
	    }
       }   

   } else {
   /* doesn't contain keywords, just create a copy of the input string and return */
      returnString=strdup(path);
   }
   
   return returnString; 
} 

void SeqUtil_printOrWrite( const char * filename, char * text, ...) {

   va_list ap;
   FILE* tmpFile;

   va_start(ap, text); 
   if (filename != NULL) {
      if ((tmpFile = fopen(filename,"a+")) == NULL) {
         raiseError( "maestro cannot write to file:%s\n",filename );
      }
      vfprintf(tmpFile, text, ap);
      fclose(tmpFile);
   } else {
      vfprintf(stdout, text, ap);
   }
   va_end(ap);
}


