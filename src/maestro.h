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
