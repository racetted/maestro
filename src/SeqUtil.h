#ifndef _SEQ_UTIL
#define _SEQ_UTIL

#include "SeqListNode.h"
void raiseError(const char* fmt, ... );
void SeqUtil_TRACE (char * fmt, ...) ;
void SeqUtil_setDebug (int _debug) ;
int SeqUtil_isDebug (int _debug) ;
void SeqUtil_setTrace (int _trace) ;
int SeqUtil_isTraceOn (int _trace) ;
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
void SeqUtil_stringAppend( char** source, char* data );
int SeqUtil_tokenCount( char* source, char* tokenSeparator );
char* SeqUtil_fixPath ( const char* source );
char* SeqUtil_getExpPath( const char* username, const char* exp ) ;
int match(const char *string, char *pattern) ;

#endif
