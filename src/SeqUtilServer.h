#ifndef _SEQ_UTIL_SERVER
#define _SEQ_UTIL_SERVER
#include <openssl/md5.h>
#include "l2d2_socket.h"

int  removeFile_svr(const char *filename ) ;
int  (*_removeFile)(const char *filename ) ;

/* this library fnc is allready declared in unistd.h : int  access (char *filename, int mode); */
int  access_svr(const char *filename, int mode ) ;
int  (*_access)(const char *filename, int mode ) ;

int  touch_svr(const char *filename ) ;
int  (*_touch)(const char *filename );

FILE * fopen_svr ( const char *filename , int  sock );
FILE * (*_fopen) ( const char *filename , int  sock );

int SeqUtil_mkdir_svr ( const char* dirname, int notUsed );
int (*_SeqUtil_mkdir) ( const char* dirname, int notUsed );

int  isFileExists_svr ( const char* lockfile, const char *caller ) ;
int  (*_isFileExists) ( const char* lockfile, const char *caller ) ;

int globPath_svr (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno) );
int (*_globPath) (const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno) );

int WriteNodeWaitedFile_svr (const char* seq_xp_home, const char* nname, const char* datestamp,  const char* loopArgs,
                              const char* filename, const char* statusfile ); 

/*nt WriteInterUserDepFile_svr (const char *filename, const char *DepBuf, const char *ppwdir, const char *maestro_version,
                               const char *datestamp, const char *md5sum);
*/
int lock_svr  ( const char *filename , const char * datestamp );
int (*_lock)  ( const char *filename , const char * datestamp );
int unlock_svr  ( const char *filename , const char * datestamp );
int (*_unlock)  ( const char *filename , const char * datestamp );

#endif
