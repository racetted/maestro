#ifndef _SEQ_UTIL
#define _SEQ_UTIL

#include "SeqNode.h"
#include "SeqListNode.h"
#include "regex.h"
#include <openssl/md5.h>
#include <stdio.h>

#define SEQ_MAXFIELD 2000

void  raiseError(const char* fmt, ... );
void  SeqUtil_TRACE (char * fmt, ...) ;
void  SeqUtil_setTraceLevel (int _trace) ;
int   SeqUtil_getTraceLevel () ;
void  SeqUtil_checkExpHome (char * _expHome) ;
void  actions(char *signal, char* flow, char *node) ;
void  actionsEnd(char *signal, char* flow, char* node) ;
int   genFileList(LISTNODEPTR *fileList,const char *directory,LISTNODEPTR *filterList) ;
int   removeFile_nfs(const char *filename) ;
int   touch_nfs(const char *filename) ;
int   access_nfs (const char *filename , int mode ) ;
int   isFileExists_nfs( const char* lockfile, const char *caller );
int   globPath_nfs (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno));
int   SeqUtil_mkdir_nfs ( const char* dir_name, int is_recursive );
FILE* fopen_nfs (const char *path, int sock );
int   WriteNodeWaitedFile_nfs (const char* ,const char* ,const char* ,const char* ,const char * ,const char *,const char *);
int   WriteInterUserDepFile_nfs (const char *, const char * ,const char *,const char *,const char *,const char *);
int   lock_nfs ( const char *filename , const char * datestamp );
int   unlock_nfs ( const char *filename , const char * datestamp );
int   SeqUtil_isDirExists( const char* path_name ) ;
char* SeqUtil_getPathLeaf (const char *full_path) ;
char* SeqUtil_getPathBase (const char *full_path) ;
char* SeqUtil_cpuCalculate( const char* npex, const char* npey, const char* omp, const char* cpu_multiplier ) ;
char* SeqUtil_resub (const char *regex_text, const char *repl_text, const char *str) ;
void  SeqUtil_stringAppend( char** source, char* data );
int   SeqUtil_tokenCount( const char* source, const char* tokenSeparator );
char* SeqUtil_fixPath ( const char* source );
char* SeqUtil_getExpPath( const char* username, const char* exp ) ;
void  SeqUtil_waitForFile( char* filename, int secondsLimit, int intervalTime); 
char* SeqUtil_getdef( const char* filename, const char* key ) ;
char* SeqUtil_parsedef( const char* filename, const char* key ) ;
char* SeqUtil_keysub( const char* _str, const char* _deffile, const char* _srcfile ) ;
char* SeqUtil_striplast( const char* str ) ;
void  SeqUtil_stripSubstring(char ** string, char * substring);
char* SeqUtil_relativePathEvaluation( char* depName, SeqNodeDataPtr _nodeDataPtr); 
void  SeqUtil_printOrWrite( const char * filename, char * text, ...); 

#endif
