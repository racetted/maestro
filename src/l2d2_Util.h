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

/* function declaration */
int  removeFile (char *x);
int  CreateLock (char *x);
int  touch (char *x);
int  isFileExists ( const char *x );
int  Access ( const char *x );
int  isDirExists ( const char *x );
int  r_mkdir ( const char* x , int a  );
int  globPath (char *, int , int (*) (const char *, int ) );
int  NodeLogr (char * , int );
int  writeNodeWaitedFile ( const char * );
int  writeInterUserdepFile( const char *);
char *getPathBase (const char *);
int  _sleep (double );
int lock ( char * ,_l2d2server L2D2, char *xpn, char *node);
int unlock ( char * ,_l2d2server L2D2 , char *xpn, char *node) ;
struct _depParameters *ParseXmlDepFile(char * );
int  ParseXmlConfigFile(char * , _l2d2server * );
char *getPathLeaf (const char *);
int SendFile (const char * x , int a );
char typeofFile(mode_t mode);
void logZone(int this_Zone, int conf_Zone, int console , char * txt, ...);
#endif
