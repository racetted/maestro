/* SeqNameValues.c - Utility functions for name,value tuples construct used by the Maestro sequencer software package.
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
#include "SeqNameValues.h"
#include "SeqUtil.h"
/********************************************************************************
* SeqListNode_insertItem: Inserts an Item into the list
********************************************************************************/
void SeqNameValues_insertItem(SeqNameValuesPtr *listPtrPtr, char *name, char* value)
{
 SeqNameValuesPtr newPtr=NULL, previousPtr=NULL, currentPtr=NULL;

   newPtr = malloc(sizeof(SeqNameValues));

   if (newPtr != NULL) { /* is space available */
      newPtr->name = (char *) malloc(strlen(name)+1);
      newPtr->value = (char *) malloc(strlen(value)+1);
      strcpy(newPtr->name,name);
      strcpy(newPtr->value,value);
      newPtr->nextPtr = NULL;
      
      previousPtr = NULL;
      currentPtr = *listPtrPtr;
   
      /* position ourselve at end of list */
      while (currentPtr != NULL) {
         previousPtr = currentPtr; 
         currentPtr = currentPtr->nextPtr;
      }
      
      if (previousPtr == NULL) {
         newPtr->nextPtr=*listPtrPtr;
         *listPtrPtr = newPtr;
      } else {
         previousPtr->nextPtr=newPtr;
         newPtr->nextPtr=currentPtr;
      }
   } else {
         SeqUtil_TRACE(TL_CRITICAL,"SeqNameValues_insertItem() No memory available.\n");
   }
}

/* will remove the first element of the list that matches the name argument */
void SeqNameValues_deleteItem(SeqNameValuesPtr *listPtrPtr, char *name)
{
   SeqNameValuesPtr currentPtr = NULL, previousPtr = NULL, tmpPtr = NULL;

   currentPtr = *listPtrPtr;
   while (currentPtr != NULL) {
      if( currentPtr->name != NULL && strcmp( currentPtr->name, name ) == 0 ) {
         /* found the one to delete */
         /* save the next one */
         tmpPtr = currentPtr;

         /* link the next one to the previous one */
         if( previousPtr != NULL ) {
            previousPtr->nextPtr = currentPtr->nextPtr;
         } else {
            *listPtrPtr = currentPtr->nextPtr;
         }

         /* delete the current one */
         tmpPtr->nextPtr = NULL;
         free( tmpPtr->name );
         free( tmpPtr->value );
         free( tmpPtr );
         
         /* we're done */
         break;
      }
      previousPtr = currentPtr;
      currentPtr = currentPtr->nextPtr;
   }
}

void SeqNameValues_printList( SeqNameValuesPtr listPtr ){
   SeqNameValuesPtr myPtr=listPtr;
   if ( myPtr == NULL ) {
      SeqUtil_TRACE(TL_MINIMAL, "List is empty.\n" );
   } else {
      while ( myPtr != NULL ) {
         SeqUtil_TRACE(TL_MINIMAL,"name=%s value=%s\n", myPtr->name, myPtr->value );
         myPtr = myPtr->nextPtr;
      }
   }
}

SeqNameValuesPtr SeqNameValues_clone(  SeqNameValuesPtr listPtr ){
   SeqNameValuesPtr clonePtr = NULL;
   while ( listPtr != NULL ) {
      SeqNameValues_insertItem( &clonePtr, listPtr->name, listPtr->value );
      listPtr = listPtr->nextPtr;
   }
   return clonePtr;
}

/* returns the value that is found for a specific name attributes stored in the list */
char* SeqNameValues_getValue( SeqNameValuesPtr ptr, char* attr_name ) {
   char* returnValue = NULL;

   while ( ptr != NULL ) {
      if( strcmp( ptr->name, attr_name ) == 0 ) {
         returnValue = strdup( ptr->value );
         break;
      }
      ptr = ptr->nextPtr;
   }
   return returnValue;
}

/* changes the value of a specific attribute from the attribute list
   if not found, it will ADD it to the list
 */
void SeqNameValues_setValue( SeqNameValuesPtr* ptr, char* attr_name, char* attr_value ) {
   int found = 0;
   SeqNameValuesPtr thisPtr = *ptr;

   while ( thisPtr != NULL ) {
      if( strcmp( thisPtr->name, attr_name ) == 0 ) {
         found = 1;
         free(thisPtr->value);
         thisPtr->value = malloc( strlen(attr_value) + 1 );
         strcpy( thisPtr->value, attr_value );
         break;
      }
      thisPtr = thisPtr->nextPtr;
   }
   if( !found ) {
      SeqNameValues_insertItem( &thisPtr, attr_name, attr_value );
   }
}

void SeqNameValues_deleteWholeList(SeqNameValuesPtr *sPtr)
{
 if ( *sPtr != NULL) {
   SeqNameValues_deleteWholeList(&(*sPtr)->nextPtr);
   free((*sPtr)->value);
   free((*sPtr)->name);
   free((*sPtr)->nextPtr);
   free(*sPtr);
   *sPtr=NULL;
 }

}
