#ifndef _SEQ_UTIL
#define _SEQ_UTIL

#include "SeqNode.h"
#include "SeqListNode.h"
#include "regex.h"

#define SEQ_MAXFIELD 1000

void raiseError(const char* fmt, ... );
void SeqUtil_TRACE (char * fmt, ...) ;
void SeqUtil_setTraceLevel (int _trace) ;
int SeqUtil_getTraceLevel () ;
void SeqUtil_checkExpHome (char * _expHome) ;
void actions(char *signal, char* flow, char *node) ;
void actionsEnd(char *signal, char* flow, char* node) ;
int genFileList(LISTNODEPTR *fileList,const char *directory,LISTNODEPTR *filterList) ;
int removeFile(char *filename) ;
int touch(char *filename) ;
int isFileExists( const char* lockfile, const char *caller ) ;
int SeqUtil_isDirExists( const char* path_name ) ;
char *SeqUtil_getPathLeaf (const char *full_path) ;
char *SeqUtil_getPathBase (const char *full_path) ;
int SeqUtil_mkdir( const char* dir_name, int is_recursive ) ;
char *SeqUtil_cpuCalculate( const char* npex, const char* npey, const char* omp, const char* cpu_multiplier ) ;
char *SeqUtil_resub (const char *regex_text, const char *repl_text, const char *str) ;
void SeqUtil_stringAppend( char** source, char* data );
int SeqUtil_tokenCount( char* source, char* tokenSeparator );
char* SeqUtil_fixPath ( const char* source );
char* SeqUtil_getExpPath( const char* username, const char* exp ) ;
void SeqUtil_waitForFile( char* filename, int secondsLimit, int intervalTime); 
int match(const char *string, char *pattern) ;
char* SeqUtil_getdef( const char* filename, const char* key ) ;
char* SeqUtil_parsedef( const char* filename, const char* key ) ;
char* SeqUtil_keysub( const char* _str, const char* _deffile, const char* _srcfile ) ;
char* SeqUtil_striplast( const char* str ) ;

#endif
