/******************************************************************************
*FILE: tictac.h
*
*AUTHOR: Dominic Racette
******************************************************************************/

#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "SeqUtil.h"

/*****************************************************************************
* tictac:
* Read or set the datestamp of a given experiment.
*
*
******************************************************************************/
extern void tictac_setDate( char* _expHome, char* datestamp );

extern char* tictac_getDate( char* _expHome, char* format, char * datestamp );

extern void checkValidDatestamp(char *datestamp);
