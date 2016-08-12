/* SeqListNode.c - Utilities for list of maestro nodes used by the Maestro sequencer software package.
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
#include <stdlib.h>
#include <string.h>
#include "SeqListNode.h"
#include "SeqUtil.h"

/********************************************************************************
* SeqListNode_insertItem: Inserts an Item into the list
********************************************************************************/
void SeqListNode_insertItem(LISTNODEPTR *list_head, char *data)
{
   /*printf("SeqListNode_insertItem() called chaine=%s\n", chaine); */
   LISTNODEPTR new = NULL, current = *list_head;

   if ( (new = malloc(sizeof(LISTNODE))) == NULL){
      SeqUtil_TRACE(TL_ERROR, "SeqListNode_insertItem(): Cannot allocate memory for new item data=%s\n",data);
      return;
   }
   new->data = strdup(data);
   new->nextPtr = NULL;
   if( *list_head == NULL){
      *list_head = new;
   } else {
      while( current->nextPtr != NULL ) current = current->nextPtr;
      current->nextPtr = new;
   }
}
/********************************************************************************
 * Adds an item at the start of a list.  Note that the case where *list_head is
 * NULL (empty list) is implicitely taken into account: newPtr->next will be NULL
 * and list_head will be a pointer to the first element of the list.
********************************************************************************/
void SeqListNode_pushFront(LISTNODEPTR * list_head,const char *data)
{
   LISTNODEPTR newPtr = NULL;
   if( (newPtr = malloc(sizeof(LISTNODE)) ) == NULL ){
      SeqUtil_TRACE(TL_CRITICAL,"SeqListNode_pushFront() No memory available.\n");
      return;
   }

   newPtr->data = strdup(data);
   newPtr->nextPtr = *list_head;

   *list_head = newPtr;
}

void SeqListNode_insertTokenItem(TOKENNODEPTR *list_head, char *token, char *data)
{
   TOKENNODEPTR new = NULL, current = *list_head;
   if( (new = malloc(sizeof(TOKENNODE)) ) == NULL ){
      SeqUtil_TRACE(TL_ERROR, "SeqListNode_insertTokenItem(): Cannot allocate memory for new item token=%s, data=%s\n",token,data);
      return;
   }
   new->data = strdup(data);
   new->token = strdup(token);
   new->nextPtr = NULL;
   if ( current == NULL ){
      *list_head = new;
   } else {
      while(current->nextPtr != NULL ) current = current->nextPtr;
      current->nextPtr = new;
   }
}

/********************************************************************************
* SeqListNode_deleteTokenItem: deletes the first token node from the list.
********************************************************************************/
void SeqListNode_deleteTokenItem(TOKENNODEPTR *list_head)
{
   TOKENNODEPTR to_delete = *list_head;
   *list_head = to_delete->nextPtr;
   free(to_delete->data);
   free(to_delete->token);
   free(to_delete);
}

/********************************************************************************
* SeqListNode_deleteWholeList: deletes an element from the list.
********************************************************************************/
void SeqListNode_deleteWholeList(LISTNODEPTR *list_head)
{
   LISTNODEPTR current,tmp_next;
   current = *list_head;
   while ( current != NULL )
   {
      tmp_next = current->nextPtr;
      free(current->data);
      free(current);
      current = tmp_next;
   }
   *list_head = NULL;
}


/********************************************************************************
* SeqListNode_isItemExists: returns true if the data already exists in the list
*                           otherwise returns false
********************************************************************************/
int SeqListNode_isItemExists(LISTNODEPTR list_head, char *data)
{
   LISTNODEPTR current = list_head;
   for ( current = list_head; current != NULL; current = current->nextPtr)
      if ( strcmp( current->data, data ) == 0 )
         return 1;
   return 0;
}

int SeqListNode_isTokenItemExists(TOKENNODEPTR list_head, char *token)
{
   TOKENNODEPTR current;
   for ( current = list_head; current != NULL; current = current->nextPtr)
      if ( strcmp( current->token, token ) == 0 )
         return 1;
   return 0;
}

char *SeqListNode_getTokenData(TOKENNODEPTR list_head, char *token)
{
   TOKENNODEPTR current;
   for ( current = list_head; current != NULL; current = current->nextPtr)
      if ( strcmp( current->token, token ) == 0 )
         return current->data;
   return NULL;
}

/********************************************************************************
* SeqListNode_isListEmpty: Returns 1 if the list is empty, 0 otherwise
********************************************************************************/
int SeqListNode_isListEmpty(LISTNODEPTR sPtr)
{
   if (sPtr == NULL) {
      return(1);
   } else {
      return(0);
   }
}

/********************************************************************************
*SeqListNode_printList: Prints the contents of the list
********************************************************************************/
void SeqListNode_printList(LISTNODEPTR list_head)
{
   LISTNODEPTR current = NULL;
   if( list_head == NULL ){
      SeqUtil_TRACE(TL_FULL_TRACE,"List pointer is NULL\n");
      return;
   }

   if (SeqListNode_isListEmpty(list_head)) {
      SeqUtil_TRACE(TL_FULL_TRACE,"List is empty.\n");
      return;
   }

   for ( current = list_head; current != NULL; current = current->nextPtr)
      SeqUtil_TRACE(TL_FULL_TRACE," %s", current->data);
}

/********************************************************************************
 * SeqListNode_reverseList: Reverses linked list 'in place'
 * A -> B -> C -> D -> NULL
 * while current != NULL
 *    A <- prev    current -> D
 *    A <- prev    current -> tmp_next
 *    A <- prev <- current    tmp_next
 *    A <- B    <-  prev      current
 * A <- B    <-  C      <-  prev(=D)     current(=NULL)
 ********************************************************************************/
void SeqListNode_reverseList(LISTNODEPTR *list_head)
{
   LISTNODEPTR prev = NULL, current = *list_head, tmp_next;
   while( current != NULL ){
      tmp_next = current->nextPtr;
      current->nextPtr = prev;
      prev = current;
      current = tmp_next;
   }
   *list_head = prev;
}

/********************************************************************************
 * Creates the product of two lists:
 * {a_1, ..., a_m }  (X) { b_1, ..., b_n }
 *  = { a_1 b_1,..., a_1 b_n, ...., a_m b_1, ..., a_m b_n } where a_i b_j is a
 * concatenation of the strings a_i and b_j.
 * Note that if one of the lists is empty, the product will be empty ( 0*x == 0 )
********************************************************************************/
LISTNODEPTR SeqListNode_multiply_lists(LISTNODEPTR lhs, LISTNODEPTR rhs){
   LISTNODEPTR newList=NULL, lhs_itr, rhs_itr;
   char buffer[SEQ_MAXFIELD];
   for( lhs_itr = lhs; lhs_itr != NULL; lhs_itr = lhs_itr->nextPtr )
      for( rhs_itr = rhs; rhs_itr != NULL; rhs_itr = rhs_itr->nextPtr ){
         if( lhs_itr->data == NULL || rhs_itr->data == NULL )
            SeqUtil_TRACE(TL_FULL_TRACE, "SeqListNode.multiply_lists(): something's wrong\n");
         sprintf(buffer,"%s%s", lhs_itr->data, rhs_itr->data);
         SeqListNode_insertItem(&newList,buffer);
      }
   return newList;
}

/********************************************************************************
 * Concatenates lists: Makes the last element of lhs point to the first element
 * of rhs.
********************************************************************************/
void SeqListNode_addLists(LISTNODEPTR * lhs, LISTNODEPTR rhs ){
   LISTNODEPTR current = *lhs;
   if(*lhs == NULL){
      *lhs = rhs;
      return;
   }
   while( current->nextPtr != NULL )
      current = current->nextPtr;
   current->nextPtr = rhs;
}
