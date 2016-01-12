/* l2d2_lists.c - List utility functions for server code of the Maestro sequencer software package.
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include "l2d2_lists.h"


/*
* ----------------------------------------------
* insert a node in the last position of the list
* 
* ---------------------------------------------- 
*/
int insert (dpnode **pointer, char *sxp, char *snode, char *depOnXp, char *depOnNode, char * sdtstmp, char *depOnstmp, char *slargs, char *depOnLargs , char *key , char *link , char *wfile )
{
        dpnode *tpointer;

        if ( (*pointer) == NULL ) {
            if ( (*pointer=(dpnode *)malloc(sizeof(dpnode))) == NULL ) {
	              fprintf (stderr,"Cannot malloc for dep. list head pointer ... exiting");
		      return(1);
  	    }
            snprintf((*pointer)->sxp,sizeof((*pointer)->sxp),"%s",sxp);
	    snprintf((*pointer)->snode,sizeof((*pointer)->snode),"%s",snode);
            snprintf((*pointer)->depOnXp,sizeof((*pointer)->depOnXp),"%s",depOnXp);
            snprintf((*pointer)->depOnNode,sizeof((*pointer)->depOnNode),"%s",depOnNode);
            snprintf((*pointer)->sdstmp,sizeof((*pointer)->sdstmp),"%s",sdtstmp);
            snprintf((*pointer)->depOnDstmp,sizeof((*pointer)->depOnDstmp),"%s",depOnstmp);
            snprintf((*pointer)->link,sizeof((*pointer)->link),"%s",link);
            snprintf((*pointer)->waitfile,sizeof((*pointer)->waitfile),"%s",wfile);
            snprintf((*pointer)->slargs,sizeof((*pointer)->slargs),"%s",slargs);
            snprintf((*pointer)->depOnLargs,sizeof((*pointer)->depOnLargs),"%s",depOnLargs);
	    snprintf((*pointer)->key,sizeof((*pointer)->key),"%s",key);
            (*pointer)->next = NULL;
	    return(0);
	} else {
             /* Iterate through the list till we encounter the last node.*/
	     tpointer=*pointer;
             while (tpointer->next != NULL)
             {
		     /* duplicate ? if ( strcmp(pointer->key,key) == 0 ) return (2); */
                     tpointer = tpointer -> next;
             }
	}

        /* Allocate memory for the new node and put data in it.*/
        if ( (tpointer->next=(dpnode *)malloc(sizeof(dpnode)))  == NULL ) {
	        fprintf (stderr,"Cannot malloc in insert () ... exiting");
		/* deallocate list */
		return(1);
	}

        tpointer = tpointer->next;
        snprintf(tpointer->snode,sizeof(tpointer->snode),"%s",snode);
        snprintf(tpointer->sxp,sizeof(tpointer->sxp),"%s",sxp);
        snprintf(tpointer->depOnXp,sizeof(tpointer->depOnXp),"%s",depOnXp);
        snprintf(tpointer->depOnNode,sizeof(tpointer->depOnNode),"%s",depOnNode);
        snprintf(tpointer->link,sizeof(tpointer->link),"%s",link);
        snprintf(tpointer->waitfile,sizeof(tpointer->waitfile),"%s",wfile);
        snprintf(tpointer->slargs,sizeof(tpointer->slargs),"%s",slargs);
        snprintf(tpointer->depOnLargs,sizeof(tpointer->depOnLargs),"%s",depOnLargs);
        snprintf(tpointer->sdstmp,sizeof(tpointer->sdstmp),"%s",sdtstmp);
        snprintf(tpointer->depOnDstmp,sizeof(tpointer->depOnDstmp),"%s",depOnstmp);
        snprintf(tpointer->key,sizeof(tpointer->key),"%s",key);
        tpointer->next = NULL;

	return(0);
}

/*
* ------------------------
* find a node in the list 
* ------------------------
*/
int find (dpnode *pointer, char *key)
{
        /* pointer =  pointer -> next; First node is dummy node. */

        /* Iterate through the entire linked list and search for the key. */
        while (pointer != NULL)
        {
                if ( strcmp(pointer->key,key) == 0 )  /* key is found. */
                {
                        return(1);
                }
                pointer = pointer -> next;/* Search in the next node. */
        }
        /*Key is not found */
        return(0);
}

/*
* --------------------------
* delete a node in the list
* --------------------------
*/
int delete (dpnode *pointer, char *key)
{
        dpnode *temp;

        /* Go to the node for which the node next to it has to be deleted */
        while (pointer != NULL && strcmp(pointer->key,key) != 0 )
        {
                pointer = pointer -> next;
        }

        if (pointer->next == NULL)
        {
                printf("Key:%s is not present in the list\n",key);
                return (1);
        }
        /* Now pointer points to a node and the node next to it has to be removed */
        temp = pointer -> next;      /*temp points to the node which has to be removed*/
	pointer->next = temp->next;
        free(temp);                  /*we removed the node which is next to the pointer (which is also temp) */

        return(0);
}

/* 
* -------------
* free list
*/
void free_list ( dpnode *pointer) 
{
   if ( pointer != NULL ) 
   {
         free_list ( pointer->next );
	 free(pointer);
   }
}

