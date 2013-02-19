/******************************************************************************
*FILE: maestro.h
*
*AUTHOR: Sua Lim
*        Dominic Racette
******************************************************************************/
#include "SeqNameValues.h"

/********************************************************************************
*maestro: operational run control manager
*     node parameter is the path to the task/family node       
*
*     sign parameter can be the following:
*     end, endnoxfer, endmodel, endnoxfermodel, initialize, abort,
*     abortcallcs, abortnb, abortnoxfer, abortnoxfernb, begin and submit.
*
*     example: maestro(1,"tidy")
*              maestro(1,"-h")
*              maestro(2,"submit","dbstart_12");
*              maestro(2,"initialize","r112");
********************************************************************************/
extern int maestro( char* _node, char* _sign, char* _flow, SeqNameValuesPtr _loops, int ignoreAllDeps, char * _extraArgs, char *_datestamp);
