#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SeqListNode.h"

/********************************************************************************
* SeqListNode_insertItem: Inserts an Item into the list
********************************************************************************/
void SeqListNode_insertItem(LISTNODEPTR *sPtr, char *chaine)
{
/*printf("SeqListNode_insertItem() called chaine=%s\n", chaine); */
 LISTNODEPTR newPtr=NULL, previousPtr=NULL, currentPtr=NULL;

   newPtr = malloc(sizeof(LISTNODE));

   if (newPtr != NULL) { /* is space available */
      newPtr->data = (char *) malloc(strlen(chaine)+1);
      strcpy(newPtr->data,chaine);
      newPtr->nextPtr = NULL;
      
      previousPtr = NULL;
      currentPtr = *sPtr;
   
      /* position ourselve at end of list */
      while (currentPtr != NULL) {
         previousPtr = currentPtr; 
         currentPtr = currentPtr->nextPtr;
      }
      
      if (previousPtr == NULL) {
         newPtr->nextPtr=*sPtr;
         *sPtr = newPtr;
      } else {
         previousPtr->nextPtr=newPtr;
         newPtr->nextPtr=currentPtr;
      }
   } else {
         printf("%s not inserted. No memory available.\n",chaine);
   }
/*printf("SeqListNode_insertItem() called done\n", chaine); */
}

void SeqListNode_insertTokenItem(TOKENNODEPTR *sPtr, char *token, char *data)
{
   /*printf("SeqListNode_insertTokenItem() called token=%s\n", token); */
   TOKENNODEPTR newPtr=NULL, previousPtr=NULL, currentPtr=NULL;
   
   newPtr = malloc(sizeof(TOKENNODE));
   
   if (newPtr != NULL) { /* is space available */
      newPtr->token = (char *) malloc(strlen(token)+1);
      strcpy(newPtr->token,token);
      newPtr->data = (char *) malloc(strlen(data)+1);
      strcpy(newPtr->data,data);
      newPtr->nextPtr = NULL;
      
      previousPtr = NULL;
      currentPtr = *sPtr;
      
      /* position ourselve at end of list */
      while (currentPtr != NULL) {
	 previousPtr = currentPtr; 
	 currentPtr = currentPtr->nextPtr;
      }
      
      if (previousPtr == NULL) {
	 newPtr->nextPtr=*sPtr;
	 *sPtr = newPtr;
      } else {
	 previousPtr->nextPtr=newPtr;
	 newPtr->nextPtr=currentPtr;
      }
   } else {
      printf("%s not inserted. No memory available.\n",token);
   }
   /*printf("SeqListNode_insertTokenItem() called done\n", token); */
}

/********************************************************************************
* SeqListNode_deleteTokenItem: deletes the first token node from the list.
********************************************************************************/
void SeqListNode_deleteTokenItem(TOKENNODEPTR *sPtr)
{
 TOKENNODEPTR tempPtr=NULL;
 tempPtr = *sPtr;
 *sPtr = (*sPtr)->nextPtr;
 tempPtr->nextPtr=NULL;
 free(tempPtr->data);
 free(tempPtr->token); 
 free(tempPtr);
 tempPtr=NULL;
}

/********************************************************************************
* SeqListNode_deleteWholeList: deletes an element from the list.
********************************************************************************/
void SeqListNode_deleteWholeList(LISTNODEPTR *sPtr)
{
 if ( *sPtr != NULL) {
   SeqListNode_deleteWholeList(&(*sPtr)->nextPtr);
   free((*sPtr)->data);
   free((*sPtr)->nextPtr);
   free(*sPtr);
   *sPtr=NULL;
 }
}


/********************************************************************************
* SeqListNode_isItemExists: returns true if the data already exists in the list
*                           otherwise returns false
********************************************************************************/
int SeqListNode_isItemExists(LISTNODEPTR sPtr, char *data)
{
   LISTNODEPTR currentPtr = sPtr;
   int found = 0;

   /* position ourselve at beginning of list */
   while (currentPtr != NULL && found == 0) {
      if ( strcmp( currentPtr->data, data ) == 0 ) {
         /* item exists */
	 found = 1;
	 break;
      }
      currentPtr = currentPtr->nextPtr;
   }

   return found;
}

int SeqListNode_isTokenItemExists(TOKENNODEPTR sPtr, char *token)
{
   TOKENNODEPTR currentPtr = sPtr;
   int found = 0;
   
   /* position ourselve at beginning of list */
   while (currentPtr != NULL && found == 0) {
      if ( strcmp( currentPtr->token, token ) == 0 ) {
	 /* item exists */
	 found = 1;
	 if (strcmp(currentPtr->data, "") == 0 ) {
	    SeqListNode_deleteTokenItem(&sPtr);
	 }
	 break;
      }
      currentPtr = currentPtr->nextPtr;
   }
   
   return found;
}

char *SeqListNode_getTokenData(TOKENNODEPTR sPtr, char *token)
{
   TOKENNODEPTR currentPtr = sPtr;
   int found = 0;
   
   /* position ourselve at beginning of list */
   while (currentPtr != NULL && found == 0) {
      if ( strcmp( currentPtr->token, token ) == 0 ) {
	 /* item exists */
	 return currentPtr->data;
      }
      currentPtr = currentPtr->nextPtr;
   }
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
void SeqListNode_printList(LISTNODEPTR currentPtr)
{
 if (currentPtr == NULL) {
    printf("List is empty.\n");
  } else {
      printf("%s", currentPtr->data);
      currentPtr = currentPtr->nextPtr;
      while (currentPtr != NULL) {
         printf(" %s", currentPtr->data);
         currentPtr = currentPtr->nextPtr;
      }
 }
}

/********************************************************************************
 * SeqListNode_reverseList: reverse the contents of the list
 ********************************************************************************/
void SeqListNode_reverseList(LISTNODEPTR *sPtr)
{
   LISTNODEPTR p, q, r;
   
   p = q = r = *sPtr;
   p = p->nextPtr->nextPtr;
   q = q->nextPtr;
   r->nextPtr = NULL;
   q->nextPtr = r;
   
   while (p != NULL)
   {
      r = q;
      q = p;
      p = p->nextPtr;
      q->nextPtr = r;
   }
   *sPtr = q;
}
