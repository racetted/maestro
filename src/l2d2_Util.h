#ifndef L2D2UTIL_H
#define L2D2UTIL_H

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include "l2d2_server.h"

#define CONSOLE_OUT 0
#define CONSOLE_ERR 1
#define LOCK_TIME_TO_LIVE 5
#define MAX_RETRIES 10

/* function declaration */
int  removeFile (char *x);
int  CreateLock (char *x);
int  touch (char *x);
int  isFileExists ( const char *x );
int  Access ( const char *x );
int  isDirExists ( const char *x );
int  r_mkdir ( const char* x , int a  );
int  globPath (char *, int , int (*) (const char *, int ) );
int  NodeLogr (char * , int , FILE *);
int  writeNodeWaitedFile ( const char * , FILE *);
int  writeInterUserdepFile( const char *, FILE *);
int  writeInterUserdepFile_v2( const char *, int  , FILE *);
char *getPathBase (const char *);
int  _sleep (double );
int lock ( char * ,_l2d2server L2D2, char *xpn, char *node, FILE *fp);
int unlock ( char * ,_l2d2server L2D2 , char *xpn, char *node, FILE *fp) ;
int  ParseXmlConfigFile(char * , _l2d2server * );
struct _depParameters *ParseXmlDepFile(char *filename , FILE * , FILE * );
int SendFile (const char * x , int a , FILE *);
void logZone(int this_Zone, int conf_Zone, FILE *fp  , char * txt, ...);
char *getPathLeaf (const char *);
char typeofFile(mode_t mode);
int l2d2_Util_isNodeXState (const char* node, const char* loopargs, const char* datestamp, const char* exp, const char* state);
#endif
