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

/********************************************************************************
* SeqListNode_deleteItem: deletes an element from the list and return it.
********************************************************************************/
char *SeqListNode_deleteItem(LISTNODEPTR *sPtr)
{
LISTNODEPTR tempPtr=NULL;
 char *data=NULL;

 tempPtr = *sPtr;
 data = (char *) malloc(strlen(tempPtr->data) + 1);

 strcpy(data,(*sPtr)->data);
 *sPtr = (*sPtr)->nextPtr;
 tempPtr->nextPtr=NULL;
 free(tempPtr->data);
 free(tempPtr);
 tempPtr=NULL;
 return(data);
}

/********************************************************************************
* SeqListNode_deleteWholeList: deletes an element from the list and return it.
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
