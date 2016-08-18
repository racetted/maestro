/* Part of the Maestro sequencer software package.
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


#ifndef _SEQ_UTIL
#define _SEQ_UTIL

#include "SeqNode.h"
#include "SeqListNode.h"
#include "regex.h"
#include <openssl/md5.h>
#include <stdio.h>

#define SEQ_MAXFIELD 2048

/* Multiples of 10 to be able to add levels in between in the future */
#define TRACE_LEVEL		1
#define TL_FULL_TRACE 		10
#define TL_MEDIUM 		15
#define TL_ERROR			20
#define TL_CRITICAL 		30

#define TF_TIMESTAMP 	0x0f01

#define TF_ON	1
#define TF_OFF 0


/* Usage: given a string, the macro will allocate a copy on the stack and
 * iterate through it using strtok_r and will provide an iterator named token
 * for the user to use inside the for_tokens block. The macro allows you to
 * specify the name of the save pointer in case you want to use two of these
 * macros in the same function. Note that since the copy of the string is
 * allocated on the stack, it will be deallocated when the function exits. This
 * way, the macro doesn't require the user to declare anything.
 * for_tokens( my_token_name, my_string, my_delimiters, my_sp){
 *    ...
 * }
 */
#define for_tokens( token, string, delimiters, sp) \
   char * sp = NULL;\
   char tmpString[strlen(string) + 1];\
   char * token = NULL; \
   strcpy(tmpString,string);\
   for( token = strtok_r(tmpString,delimiters, &sp); token != NULL; token = strtok_r(NULL,delimiters,&sp))

#define LOCATE SeqUtil_TRACE(TL_FULL_TRACE,"%s():[%s:%d]",__func__,__FILE__,__LINE__)
#define FUNCBEGIN \
   SeqUtil_TRACE(TL_FULL_TRACE, "%s() BEGIN (%s,%d)\n", __func__,__FILE__,__LINE__);

#define FUNCEND \
   SeqUtil_TRACE(TL_FULL_TRACE, "%s() END (%s,%d)\n", __func__,__FILE__,__LINE__);

void SeqUtil_addPadding( char *dst, const char *datestamp, char c, int length);
void  raiseError(const char* fmt, ... );
void  SeqUtil_TRACE( int level,const char * fmt, ...);
void  SeqUtil_setTraceLevel (int _trace) ;
int   SeqUtil_getTraceLevel () ;
void SeqUtil_setTraceFlag(int flag, int value);
void SeqUtil_setTraceEnv();
void SeqUtil_showTraceInfo();
void  SeqUtil_checkExpHome (char * _expHome) ;
void  actions(char *signal, char* flow, char *node) ;
void  actionsEnd(char *signal, char* flow, char* node) ;
int   genFileList(LISTNODEPTR *fileList,const char *directory,LISTNODEPTR *filterList) ;
int   removeFile_nfs(const char *filename, const char * _seq_exp_home) ;
int   touch_nfs(const char *filename, const char * _seq_exp_home) ;
int   access_nfs (const char *filename , int mode, const char * _seq_exp_home ) ;
int   isFileExists_nfs( const char* lockfile, const char *caller, const char * _seq_exp_home );
int   globPath_nfs (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno), const char * _seq_exp_home);
LISTNODEPTR   globExtList_nfs (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno));
int   SeqUtil_mkdir_nfs ( const char* dir_name, int is_recursive, const char * _seq_exp_home );
FILE* fopen_nfs (const char *path, int sock );
int   WriteNodeWaitedFile_nfs (const char* ,const char* ,const char* ,const char * ,const char *,const char *);
int   WriteInterUserDepFile_nfs (const char *, const char * ,const char *,const char *,const char *,const char *);
int   WriteForEachFile_nfs (const char* ,const char* ,const char* ,const char * ,const char *,const char *);
int   WriteInterUserForEachFile_nfs (const char *, const char * ,const char *,const char *,const char *,const char *);
int   lock_nfs ( const char *filename , const char * datestamp, const char * _seq_exp_home );
int   unlock_nfs ( const char *filename , const char * datestamp, const char * _seq_exp_home );
int   SeqUtil_isDirExists( const char* path_name ) ;
char* SeqUtil_getPathLeaf (const char *full_path) ;
char* SeqUtil_getPathBase (const char *full_path) ;
char* SeqUtil_cpuCalculate( const char* npex, const char* npey, const char* omp, const char* cpu_multiplier ) ;
char* SeqUtil_resub (const char *regex_text, const char *repl_text, const char *str) ;
void  SeqUtil_stringAppend( char** source, const char* data );
int   SeqUtil_tokenCount( const char* source, const char* tokenSeparator );
char* SeqUtil_fixPath ( const char* source );
void  SeqUtil_waitForFile( char* filename, int secondsLimit, int intervalTime); 
char* SeqUtil_getdef( const char* filename, const char* key  , const char* _seq_exp_home) ;
char* SeqUtil_parsedef( const char* filename, const char* key ) ;
char* SeqUtil_keysub( const char* _str, const char* _deffile, const char* _srcfile, const char* _seq_exp_home ) ;
char* SeqUtil_striplast( const char* str ) ;
void  SeqUtil_stripSubstring(char ** string, char * substring);
char* SeqUtil_relativePathEvaluation( char* depName, SeqNodeDataPtr _nodeDataPtr); 
void  SeqUtil_printOrWrite( FILE * filename, char * text, ...); 
int   SeqUtil_basicTruncatedAverage(int *unsorted_int_array, int elements, int removal_quantity); 
int   SeqUtil_compareInt (const void * a, const void * b);
char* SeqUtil_normpath(char *out, const char *in); 
const char * SeqUtil_resourceDefFilename(const char * _seq_exp_home);
char *SeqUtil_getTraceLevelString();
int SeqUtil_sprintStatusFile(char *dst,const char * exp_home, const char *node_name, const char *datestamp, const char * extension, const char *status);
int SeqUtil_sprintContainerTaskPath(char *dst, const char *exp_home, const char *node_path);
char * SeqUtil_getScriptOutput(const char *script_path, size_t buffer_size);
const char *SeqUtil_popenGetScriptOutput(const char * script_path, size_t buffer_size);
#endif
