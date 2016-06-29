/* SeqListNode.h - Utilities for list of maestro nodes used by the Maestro sequencer software package.
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

#ifndef _SEQ_LISTNODE
#define _SEQ_LISTNODE

#define for_list(iterator, list_head) \
   LISTNODEPTR iterator;\
   for( iterator = list_head;\
        iterator != NULL;\
        iterator = iterator->nextPtr \
      )

struct listNode {
    char *data;
    struct listNode *nextPtr;
  };

typedef struct listNode LISTNODE;
typedef LISTNODE *LISTNODEPTR;

struct tokenNode {
   char *token;
   char *data;
   struct tokenNode *nextPtr;
};

typedef struct tokenNode TOKENNODE;
typedef TOKENNODE *TOKENNODEPTR;

/****************************************************************
*insertItem: Insert an item 's' into the list 'list'. The memory
*necessary to store the string 's' will be allocated by insertItem.
*****************************************************************/
void SeqListNode_insertItem(LISTNODEPTR *list, char *s);
void SeqListNode_insertTokenItem(TOKENNODEPTR *list, char *token, char *data);

/****************************************************************
* pushFront: Adds an item at the start of a list.
*****************************************************************/
void SeqListNode_pushFront(LISTNODEPTR * list_head, char *data);

/****************************************************************
*deleteItem: Delete the first item from the token node list pointer 'list'. 
*The memory will be deallocated by deleteItem.
*****************************************************************/
void SeqListNode_deleteTokenItem(TOKENNODEPTR *sPtr);

/****************************************************************
*isListEmpty: Returns 1 if list 'sPtr' is empty, eitherwise 0
*****************************************************************/
int SeqListNode_isListEmpty(LISTNODEPTR sPtr);
    
/********************************************************************************
* SeqListNode_isItemExists: returns true if the data already exists in the list
*                           otherwise returns false
********************************************************************************/
int SeqListNode_isItemExists(LISTNODEPTR sPtr, char *data);
int SeqListNode_isTokenItemExists(TOKENNODEPTR sPtr, char *token);
char *SeqListNode_getTokenData(TOKENNODEPTR sPtr, char *token);

/****************************************************************
*deleteWholeList: delete the list 'list' and deallocate the memories
*****************************************************************/
void SeqListNode_deleteWholeList(LISTNODEPTR *list);

/****************************************************************
*printList: print the list 'list'
*****************************************************************/
void SeqListNode_printList(LISTNODEPTR list);

/********************************************************************************
 * SeqListNode_reverseList: reverse the contents of the list
 ********************************************************************************/
void SeqListNode_reverseList(LISTNODEPTR *sPtr);

/********************************************************************************
 * SeqListNode_multiply_lists: Creates the product of two lists
 ********************************************************************************/
LISTNODEPTR SeqListNode_multiply_lists(LISTNODEPTR lhs, LISTNODEPTR rhs);

/********************************************************************************
 * SeqListNode_addLists: Concatenates lists lhs->...->end_lhs->rhs->...->end_rhs->NULL
 ********************************************************************************/
void SeqListNode_addLists(LISTNODEPTR * lhs, LISTNODEPTR rhs );
#endif

