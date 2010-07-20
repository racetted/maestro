/******************************************************************************
*FILE: tictac.h
*
*AUTHOR: Dominic Racette
******************************************************************************/

/*****************************************************************************
* tictac:
* Read or set the datestamp of a given experiment.
*
*
******************************************************************************/
extern void tictac_setDate( char* _expHome, char* datestamp );

extern char* tictac_getDate( char* _expHome, char* format );

extern char* checkValidDatestamp(char *datestamp);