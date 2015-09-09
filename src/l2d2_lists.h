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


#include <time.h>
#ifndef L2D2_LISTS_H
#define L2D2_LISTS_H

typedef struct _dpnode 
{
	char sxp[1024];
	char snode[1024];
	char depOnXp[1024];
	char depOnNode[1024];
	char slargs[512];
	char depOnLargs[512];
	char link[256];
	char waitfile[1024]; 
	char sdstmp[32];
	char depOnDstmp[32];
	char key[64];
        struct _dpnode *next;
} dpnode;

/* forward function declarations */
/* insert list_ptr , dependent|dependee xp , */
int  insert (dpnode **pointer, char *sxp , char *snode , char *depOnxp , char *depOnnode, char * sdtsmp, char * depstmp, char *slargs, char *depOnlargs , char *depkey , char *link , char *wfile );
int  find   (dpnode *pointer, char *key);
int  delete (dpnode *pointer, char *key);
void free_list ( dpnode *ptr);

#endif
