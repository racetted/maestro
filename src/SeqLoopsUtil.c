/* SeqLoopsUtil.c - Utility functions for loops in the Maestro sequencer software package.
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
#include <stdarg.h>
#include "SeqListNode.h"
#include "SeqNameValues.h"
#include "SeqNode.h"
#include "SeqUtil.h"
#include <string.h>

#define DEF_START 0
#define DEF_END 1
#define DEF_STEP 2
#define DEF_SET 3

static char* EXT_TOKEN = "+";

static int hasWildCard(SeqNameValuesPtr depArg);
static LISTNODEPTR loopArg_to_reverse_extension_list( SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr arg);
SeqLoopsPtr SeqLoops_findLoopByName( SeqLoopsPtr loopsPtr, char * name);
static LISTNODEPTR loop_to_reverse_extension_list(SeqLoopsPtr loopPtr);
static LISTNODEPTR switch_to_extension_list(SeqLoopsPtr loopPtr);
static LISTNODEPTR SeqNodeData_to_reverse_extension_list( SeqNodeDataPtr _nodeDataPtr);
static LISTNODEPTR numerical_to_reverse_extension_list(SeqNameValuesPtr loopData);
static LISTNODEPTR expression_to_reverse_extension_list( char * expression );
static LISTNODEPTR def_to_reverse_extension_list( int start, int end, int step);

LISTNODEPTR SeqLoops_childExtensions( SeqNodeDataPtr _nodeDataPtr );
LISTNODEPTR SeqLoops_getLoopContainerExtensions( SeqNodeDataPtr _nodeDataPtr, const char * depIndex );

/* function to parse the loop arguments
   returns 0 if succeeds
   returns -1 if the function fais with an error
   cmd_args must be in the form "loop_name=value,loop_namex=valuex"
*/
char * SeqLoops_indexToExt(const char * index)
{
   if( index == NULL ) return NULL;
   const char * src = index;
   char ext[strlen(index)];
   char * dst = ext;
   while (*src != '\0'){
      /* Skip the left hand side of the '=' */
      while (*src != '=') ++src; ++src;

      /* copy EXT_TOKEN */
      char * tok = EXT_TOKEN;
      while ( *tok != '\0' ) *dst++ = *tok++;

      /* copy the right hand side of the '=' */
      while( *src != ',' && *src != '\0') *dst++ = *src++;
   }
   *dst = '\0';
   return strdup(ext);
}

int SeqLoops_parseArgs( SeqNameValuesPtr* nameValuesPtr, const char* cmd_args ) {
   char *tmpstrtok = NULL, *tmp_args = NULL;
   char loopName[100], loopValue[50];
   int isError = 0, n=0;
   
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_parseArgs cmd_args:%s\n", cmd_args );
   /*
   */
   
   tmp_args = strdup( cmd_args );
   tmpstrtok = (char*) strtok( tmp_args, "," );
   while ( tmpstrtok != NULL ) {
      /* any alphanumeric characters and special chars
         _:/-$()* are supported */
      memset(loopName,'\0',sizeof loopName);
      memset(loopValue,'\0',sizeof loopValue);

      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_parseArgs token: %s\n", tmpstrtok ); 
      n=sscanf( tmpstrtok, " %[A-Za-z0-9._:/]=%[A-Za-z0-9._^/-$()*]", loopName, loopValue );
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_parseArgs parsed items (count=%d): %s = %s\n",n, loopName, loopValue ); 
      /* should add more syntax validation such as spaces not allowed... */
      if ( n != 2) {
         isError = -1;
      } else {
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_parseArgs inserted %s = %s\n", loopName, loopValue ); 
         SeqNameValues_insertItem( nameValuesPtr, loopName, loopValue );
      }
      tmpstrtok = (char*) strtok(NULL,",");
   }
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_parseArgsdone exit status: %d\n", isError ); 
   free( tmp_args );
   return(isError);
}

/* function that returns the base extension of a node.
examples: 
extension=_1_2_3_4 will return _1_2_3
extension=_1 will return ""
*/
char* SeqLoops_getExtensionBase ( SeqNodeDataPtr _nodeDataPtr ) {
   char *split = NULL , *work_string = NULL ,*chreturn =NULL;
   int foundLastSeparator = 0;
   SeqLoopsPtr loopsContainerPtr = _nodeDataPtr->loops;
   work_string = strdup(_nodeDataPtr->extension);
   if( _nodeDataPtr->type == Loop || _nodeDataPtr->type == NpassTask ||  _nodeDataPtr->type == Switch || _nodeDataPtr->type == ForEach ) {
      while( loopsContainerPtr != NULL ) {
         loopsContainerPtr = loopsContainerPtr->nextPtr;
      }
      split = strrchr (work_string,EXT_TOKEN[0]);
      if (split != NULL) {
         foundLastSeparator=1;
      }
   }
   if (foundLastSeparator == 1){ 
      chreturn = strndup(work_string,split-work_string); 
   } else {
      chreturn = strdup(work_string); 
   }
   free( work_string );

   return chreturn;
}

/* converts an extension value to a NameValueList that can be used
to maestro argument. returns NULL if no conversion done.
example: 
   input _1_2_3
   output is a name_value_list something like 
   loop_name_a=1->loop_name_b=2->loop_name_c=3
  */
SeqNameValuesPtr SeqLoops_convertExtension ( SeqLoopsPtr loops_ptr, char* extension ) {
   SeqNameValuesPtr nameValuesPtr = NULL;
   char *token = NULL;
   char *leaf = NULL;
   if (extension != NULL){
       token = (char*) strtok( extension, EXT_TOKEN );
       while ( token != NULL ) {
          free( leaf );
          leaf = (char*) SeqUtil_getPathLeaf( loops_ptr->loop_name );
          SeqNameValues_insertItem( &nameValuesPtr, leaf, token );

          loops_ptr = loops_ptr->nextPtr;
          token = (char*) strtok( NULL, EXT_TOKEN );
       }
   }
   SeqNameValues_printList( nameValuesPtr );
   return nameValuesPtr;
}

/* returns the value of a specific attribute from the attribute list of a loop item,
   return NULL if not found
   the attr_name is something like NAME, START, STEP, END,...
 */
char* SeqLoops_getLoopAttribute( SeqNameValuesPtr loop_attr_ptr, char* attr_name ) {
   char* returnValue = NULL;
   SeqNameValuesPtr tmpptr=loop_attr_ptr;

   SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopAttribute looking for attribute:%s\n", attr_name);
   while ( tmpptr != NULL ) {
      if( strcmp( tmpptr->name, attr_name ) == 0 ) {
         returnValue = strdup( tmpptr->value );
         SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopAttribute found attribute:%s value:%s \n", attr_name, returnValue );
         break;
      }
      tmpptr = tmpptr->nextPtr;
   }
   if (returnValue == NULL ){
      SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopAttribute(): attribute %s not found\n", attr_name);
   }
   return returnValue;
}

/* changes the value of a specific attribute from the attribute list of a loop item,
   if not found, it will ADD it to the list
   the attr_name is something like NAME, START, STEP, END,...
 */
void SeqLoops_setLoopAttribute( SeqNameValuesPtr* loop_attr_ptr, char* attr_name, char* attr_value ) {
   int found = 0;
   SeqNameValuesPtr loopAttrPtr = *loop_attr_ptr;

   while ( loopAttrPtr != NULL ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_setLoopAttribute looking for:%s found:%s \n", attr_name, loopAttrPtr->name );
      if( strcmp( loopAttrPtr->name, attr_name ) == 0 ) {
         found = 1;
         free(loopAttrPtr->value);
         loopAttrPtr->value = malloc( strlen(attr_value) + 1 );
         strcpy( loopAttrPtr->value, attr_value );
         break;
      }
      loopAttrPtr = loopAttrPtr->nextPtr;
   }
   if( !found ) {
      SeqUtil_TRACE(TL_FULL_TRACE,"Inserting %s=%s in loop attributes\n", attr_name, attr_value);
      SeqNameValues_insertItem( loop_attr_ptr, attr_name, attr_value );
   }
}

/* returns the extension value that should be appended to the node_name using
   the list of container loops and the list of loop arguments
   returns NULL if validation fails
*/
char* SeqLoops_ContainerExtension( SeqLoopsPtr loops_ptr, SeqNameValuesPtr loop_args_ptr ) {
   SeqNameValuesPtr thisLoopArgPtr = NULL;
   int foundLoopArg = 0;
   char *loopContainerName = NULL, *loopLeafName = NULL;
   char* extension = NULL;
   int isError = 0;
   while( loops_ptr != NULL && isError == 0 ) {
      foundLoopArg = 0;
      loopContainerName = loops_ptr->loop_name;
      /* inside the SeqLoopsPtr, the loop_name value is stored with the full path node 
         of the loop while the loop_name function argument is only the leaf part */
      loopLeafName = (char*) SeqUtil_getPathLeaf( loopContainerName );
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_ContainerExtension loopLeafName=%s\n", loopLeafName ); 
      thisLoopArgPtr = loop_args_ptr;
      while( thisLoopArgPtr != NULL && isError == 0 ) {
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_ContainerExtension loop_name=%s loop_value=%s\n", thisLoopArgPtr->name, thisLoopArgPtr->value ); 
         if( strcmp( thisLoopArgPtr->name, loopLeafName ) == 0 ) {
            foundLoopArg = 1;
            SeqUtil_stringAppend( &extension, EXT_TOKEN );
            SeqUtil_stringAppend( &extension, thisLoopArgPtr->value );
            SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_ContainerExtension found loop argument loop_name=%s loop_value=%s\n", thisLoopArgPtr->name, thisLoopArgPtr->value ); 
            break;
         }
         thisLoopArgPtr = thisLoopArgPtr->nextPtr;
      }

      isError = !foundLoopArg;

      free( loopLeafName );
      loops_ptr  = loops_ptr->nextPtr;
   }

   if( isError == 1 && SeqNameValues_getValue(loop_args_ptr, "newdef") == NULL )
      return NULL;

   return extension;
}

/* returns the extension value that should be appended to the node_name using
   the list of loop arguments. To be used if the node is of type Loop.
   returns NULL if validation fails
   node_name must be the leaf part of the node only.
*/
char* SeqLoops_NodeExtension( const char* node_name, SeqNameValuesPtr loop_args_ptr ) {

   int foundLoopArg = 0;
   char* extension = NULL;
   while( loop_args_ptr != NULL ) {
      if( strcmp( loop_args_ptr->name, node_name ) == 0 ) {
         foundLoopArg = 1;
         SeqUtil_stringAppend( &extension, EXT_TOKEN );
         SeqUtil_stringAppend( &extension, loop_args_ptr->value );
         break;
      }
      loop_args_ptr = loop_args_ptr->nextPtr;
   }

   if( foundLoopArg == 0 )
      return NULL;

   return extension;
}

/* return the loop container extension values that the current node is in.
 * Current node must be a loop node.
 * To be used to check if a loop parent node is done,
 * assuming that extension is set in  _nodeDataPtr->extension 
 * List is given end first for optimization purposes. 
*/
LISTNODEPTR SeqLoops_childExtensionsInReverse( SeqNodeDataPtr _nodeDataPtr ) {
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[20], *baseExtension;
   int loopStart = 0, loopStep = 1, loopEnd = 1, loopCount = 0;
   char *tmpExpression = NULL, *tmpAttributeValue=NULL;
   LISTNODEPTR loopExtensions = NULL, tmpLoopExtensions = NULL;
   memset( tmp, '\0', sizeof tmp );
   baseExtension = SeqLoops_getExtensionBase( _nodeDataPtr );
   nodeSpecPtr = _nodeDataPtr->data;
   
   tmpExpression = SeqLoops_getLoopAttribute( nodeSpecPtr, "EXPRESSION" );
   if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_childExtensionsInReverse expression: %s\n", tmpExpression);
      tmpLoopExtensions = (LISTNODEPTR) SeqLoops_childExtensions(_nodeDataPtr);
      SeqListNode_reverseList ( &tmpLoopExtensions );
      loopExtensions = tmpLoopExtensions;
   } else {

      if (( tmpAttributeValue = SeqLoops_getLoopAttribute( nodeSpecPtr, "START" )) != NULL ) { 
         loopStart = atoi(tmpAttributeValue);
      } 
      if(( tmpAttributeValue = SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" )) != NULL ) { 
	      loopStep = atoi(tmpAttributeValue);
      }
      if(( tmpAttributeValue =  SeqLoops_getLoopAttribute( nodeSpecPtr, "END" )) != NULL ) {
         loopEnd = atoi(tmpAttributeValue);
      }
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_childExtensionsInReverse extension:%s baseExtension:%s START:%d END:%d\n",_nodeDataPtr->extension, baseExtension, loopEnd, loopStart );
      loopCount = loopEnd;
      while( loopCount >= loopStart ) {
	      sprintf( tmp, "%s%s%d", baseExtension, EXT_TOKEN, loopCount );
	      SeqListNode_insertItem( &loopExtensions, tmp );
	      loopCount = loopCount - loopStep;
      }
   }

   return loopExtensions;
}



/* return the loop container extension values that the current node is in. (ex: +3+1 -> +3+2 -> +3+3) 
 * Current node must be a loop node.
   To be used to check if a loop parent node is done,
   assuming that extension is set in  _nodeDataPtr->extension */
LISTNODEPTR SeqLoops_childExtensions( SeqNodeDataPtr _nodeDataPtr ) {
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[20], *baseExtension;
   int expressionArray[256], i, startIndex=0, endIndex=1, stepIndex=2, setIndex=3, detectedEnd=0;
   char *tmpExpression = NULL, *tmpArrayValue = NULL, *tmpAttributeValue = NULL;
   int loopStart = 0, loopStep = 1, loopEnd = 0, loopCount = 0, breakFlag, firstFlag=1;
   LISTNODEPTR loopExtensions = NULL;
   memset( tmp, '\0', sizeof tmp );
   baseExtension = SeqLoops_getExtensionBase( _nodeDataPtr );
   nodeSpecPtr = _nodeDataPtr->data;

   tmpExpression = SeqLoops_getLoopAttribute( nodeSpecPtr, "EXPRESSION" );
   
   if ((tmpExpression != NULL && strcmp(tmpExpression, "") != 0) && ((SeqLoops_getLoopAttribute( nodeSpecPtr, "START" ) != NULL) ||
      (SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) != NULL) ||
      (SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) != NULL) ||
      (SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL))) {
      raiseError("SeqLoops_childExtensions Error: loop syntax must be defined with start, end, step, step attributes OR with an expression attribute\n");
   }
   
   if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_childExtensions expression: %s\n", tmpExpression);
      
      i=0;
      tmpArrayValue = strtok (tmpExpression,":,");
      while (tmpArrayValue != NULL) {
	 expressionArray[i] = atoi(tmpArrayValue);
	 /*End is the biggest number of the expression*/
	 if (expressionArray[i]>detectedEnd)
	    detectedEnd=expressionArray[i];
	 i++;
	 tmpArrayValue = strtok (NULL,":,");
      }
      
      while( loopCount <= detectedEnd ) {
	 loopCount = expressionArray[startIndex];
	 breakFlag = 0;
	 if ((startIndex-4) >= 0 && loopCount != expressionArray[endIndex-4]) {
	    firstFlag = 1;
	 }
	 while( loopCount <= expressionArray[endIndex] && breakFlag == 0){
	    if (firstFlag == 0) {
	       if ((loopCount+expressionArray[stepIndex]) <= expressionArray[endIndex]) {
		  loopCount = loopCount + expressionArray[stepIndex];
	       } else {
		  breakFlag = 1;
	       }
	    } else {
	       firstFlag = 0;
	    }
	    if (breakFlag == 0) {
	       sprintf( tmp, "%s%s%d", baseExtension, EXT_TOKEN, loopCount );
	       SeqListNode_insertItem( &loopExtensions, tmp );
	    }
	 }
	 if ((stepIndex+4) < i) {
	    endIndex=endIndex+4;
	    stepIndex=stepIndex+4;
	    startIndex=startIndex+4;
	 } else {
	    break;
	 }
      }
   } else {
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_childExtensions extension:%s baseExtension:%s\n",_nodeDataPtr->extension, baseExtension);
      if ( (tmpAttributeValue = SeqLoops_getLoopAttribute( nodeSpecPtr, "START" )) != NULL ) { 
         loopStart = atoi(tmpAttributeValue);
      } 
      if( (tmpAttributeValue = SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" )) != NULL ) { 
	      loopStep = atoi(tmpAttributeValue);
      }
      if( (tmpAttributeValue =  SeqLoops_getLoopAttribute( nodeSpecPtr, "END" )) != NULL ) {
         loopEnd = atoi(tmpAttributeValue);
      }

      loopCount = loopStart;
      while( loopCount <= loopEnd ) {
	      sprintf( tmp, "%s%s%d", baseExtension, EXT_TOKEN, loopCount );
	      SeqListNode_insertItem( &loopExtensions, tmp );
	      loopCount = loopCount + loopStep;
      }
   }

   return loopExtensions;
}

/* returns 1 if the parent of a node is a loop container */
int SeqLoops_isParentLoopContainer ( const SeqNodeDataPtr _nodeDataPtr ) {
   int value = 0;
   SeqLoopsPtr loopsPtr =  _nodeDataPtr->loops;
   while( loopsPtr != NULL ) {
      if( strcmp( loopsPtr->loop_name, _nodeDataPtr->container ) == 0 ) {
         value = 1;
         break;
      }
      loopsPtr  = loopsPtr->nextPtr;
   }
   return value;
}



/********************************************************************************
 * This function determines the list of possible extensions specified by
 * depIndex.  There may be many of them if there are wildcards in depIndex.  For
 * example, if we have index="n1=*,n2=x,n3=*", and n1 can be A or B, and n3 can
 * be C,D,E:.  In the first iteration, loopExtensions will be {+B, +A}, then
 * { +B, +A} x { +X} = {+B+X, +A+X}, and then the final list
 * { +B+X, +A+X } x { +E, +D, +C} = { +B+X+E, +B+X+D, +B+X+C, +A+X+E, +A+X+D, +A+X+C }.
********************************************************************************/
LISTNODEPTR SeqLoops_getLoopContainerExtensionsInReverse( SeqNodeDataPtr _nodeDataPtr, const char * depIndex ) {
   LISTNODEPTR loopExtensions=NULL, prod=NULL, rhs=NULL;
   SeqLoopsPtr loopPtr=NULL;
   SeqNameValuesPtr depArgs=NULL, arg=NULL;

   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensionsInReverse(): Begin call\n");

   SeqNode_printLoops(NULL,_nodeDataPtr);
   SeqLoops_parseArgs(&depArgs,depIndex);

   for( arg = depArgs; arg != NULL; arg = arg->nextPtr){
      rhs = loopArg_to_reverse_extension_list(_nodeDataPtr,arg);
      if ( loopExtensions == NULL ){
         loopExtensions = rhs;
      } else {
         /* loopExtensions = loopExtensions (X) rhs */
         prod = SeqListNode_multiply_lists(loopExtensions, rhs);
         SeqListNode_deleteWholeList(&loopExtensions);
         SeqListNode_deleteWholeList(&rhs);
         rhs = NULL;
         loopExtensions = prod;
      }
   }
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensionsInReverse(): returning list:   \n");
   SeqListNode_printList(loopExtensions); SeqUtil_TRACE(TL_FULL_TRACE,"\n");
   return loopExtensions;
}

/********************************************************************************
 * Determines whether a loop argument is one whose value is a wildcard.
********************************************************************************/
static int hasWildCard(SeqNameValuesPtr loop_arg)
{
   return ( strstr(loop_arg->value, "*") != NULL );
}

/********************************************************************************
 * Returns the reverse list of possible extensions for a loop (numerical loop or
 * switch).  The function looks for a loop matching arg->name in
 * _nodeDataPtr->loops, then looks in nodeDataPtr->data if nodeDataPtr is the
 * loop we are looking for.
********************************************************************************/
static LISTNODEPTR loopArg_to_reverse_extension_list( SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr arg)
{
   LISTNODEPTR extensionList = NULL;

   if( ! hasWildCard(arg)){
      char buffer[100] = {'\0'};
      sprintf(buffer,"%s%s",EXT_TOKEN,arg->value);
      SeqListNode_insertItem(&extensionList,buffer);
   } else {
      SeqLoopsPtr loopPtr = NULL;
      loopPtr = SeqLoops_findLoopByName(_nodeDataPtr->loops, arg->name);
      if(loopPtr != NULL)
         extensionList = loop_to_reverse_extension_list(loopPtr);
      else if( strcmp(arg->name, _nodeDataPtr->nodeName) == 0 )
         extensionList = SeqNodeData_to_reverse_extension_list(_nodeDataPtr);
      else
         raiseError("loopArg_to_reverse_extension_list() unable to find loop matching arg->name=%s\n", arg->name);
   }
   return extensionList;
}

/********************************************************************************
 * Looks through a SeqLoopsPtr linked list to find a loopPtr whose loop_name field
 * has a leaf matching the name argument.
********************************************************************************/
SeqLoopsPtr SeqLoops_findLoopByName( SeqLoopsPtr loopsPtr, char * name)
{
   SeqLoopsPtr current = loopsPtr;
   SeqNameValuesPtr loopData = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_findLoopByName(): Looking for loop matching name=%s\n",name);
   for (current = loopsPtr; current != NULL; current = current->nextPtr ){
      if ( strcmp( SeqUtil_getPathLeaf(current->loop_name), name) == 0 ){
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_findLoopByName(): match found %s\n",name);
         return current;
      }
   }
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_findLoopByName(): no match found. returning NULL\n");
   return NULL;
}

/********************************************************************************
 * Creates the reverse list of possible extensions for a loop whose information
 * is given in a loopPtr structure.
********************************************************************************/
static LISTNODEPTR loop_to_reverse_extension_list( SeqLoopsPtr loopPtr){
   if( loopPtr->type == SwitchType)
      return switch_to_extension_list(loopPtr);
   else if ( loopPtr->type == Numerical)
      return numerical_to_reverse_extension_list(loopPtr->values);
   else
      return NULL;
}

/********************************************************************************
 * Creates the reverse list of possible extensions for a loop when the loop is
 * the current node.  It looks in _nodeDataPtr->data for the information and
 * interprets it based on whether the loop is a numerical loop or a switch.
********************************************************************************/
static LISTNODEPTR SeqNodeData_to_reverse_extension_list( SeqNodeDataPtr _nodeDataPtr)
{
   LISTNODEPTR newList = NULL;
   if ( _nodeDataPtr->type == Switch ){
      char extension[10];
      char * switchValue = SeqNameValues_getValue(_nodeDataPtr->data, "VALUE");
      sprintf(extension, "%s%s", EXT_TOKEN, switchValue);
      SeqListNode_insertItem(&newList, extension );
      free(switchValue);
      return newList;
   } else if (_nodeDataPtr->type == Loop) {
      return numerical_to_reverse_extension_list(_nodeDataPtr->data);
   } else {
      return NULL;
   }
}


/********************************************************************************
 * Creates the extension list of a switch by simply returning a list whose only
 * element is the current value of the switch.
********************************************************************************/
static LISTNODEPTR switch_to_extension_list(SeqLoopsPtr loopPtr){
   LISTNODEPTR newList=NULL;
   char * switch_value=NULL;
   char buffer[128] = {'\0'};
   SeqUtil_TRACE(TL_FULL_TRACE,"switch_to_extension_list() begin call\n");

   switch_value = (char*)SeqLoops_getLoopAttribute( loopPtr->values, "VALUE" );

   sprintf(buffer,"%s%s", EXT_TOKEN,switch_value);
   SeqListNode_insertItem(&newList, buffer);

   SeqUtil_TRACE(TL_FULL_TRACE,"switch_to_extension_list() end call\n");
   return newList;
}

/********************************************************************************
 * Creates the list of all possible extensions for a loop by either calling
 * def_to_reverse_extension_list or by calling
 * expression_to_reverse_extension_list based on the way the loop is defined.
********************************************************************************/
static LISTNODEPTR numerical_to_reverse_extension_list(SeqNameValuesPtr loopData){
   char * attrib = NULL;
   int start=0,end=0,step=0;
   LISTNODEPTR retval = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE,"numerical_to_reverse_extension_list(): begin call\n");

   if ( (attrib = (char*)SeqLoops_getLoopAttribute( loopData, "EXPRESSION" )) != NULL && strcmp(attrib,"") != 0 ){
      retval = expression_to_reverse_extension_list( attrib );
   } else {
      if ( (attrib = (char*)SeqLoops_getLoopAttribute( loopData, "START" )) != NULL ){
         start = atoi(attrib);
         free(attrib);
      }
      if ( (attrib = (char*)SeqLoops_getLoopAttribute( loopData, "END" )) != NULL ){
         end = atoi(attrib);
         free(attrib);
      }
      if ( (attrib = (char*)SeqLoops_getLoopAttribute( loopData, "STEP" )) != NULL ){
         step = atoi(attrib);
         free(attrib);
      }
      retval = def_to_reverse_extension_list(start,end,step);
   }
   SeqUtil_TRACE(TL_FULL_TRACE,"numerical_to_reverse_extension_list(): returning list: ");
   SeqListNode_printList(retval);SeqUtil_TRACE(TL_FULL_TRACE,"\n");
   return retval;
}

/********************************************************************************
 * Creates the reverse list of possible extensions for a loop defined by an
 * expression.  Makes a call to def_to_reverse_extension_list() for each
 * definition of the expression and adds the lists together.
********************************************************************************/
static LISTNODEPTR expression_to_reverse_extension_list( char * expression ){
   LISTNODEPTR newList = NULL, rhs = NULL;
   char * token = NULL;
   int * currentDef = NULL;
   int i=0,expressionArray[SEQ_MAXFIELD] = {'\0'},number_of_definitions=0;

   for ( i = 0, token = strtok(expression,":,") ; token != NULL ; i++, token = strtok(NULL,":,") )
      expressionArray[i] = atoi(token);
   number_of_definitions = (i / 4);

   /* Make an currentDef point to the start of the last definition */
   currentDef = &expressionArray[i - 4];

   /* Iterate over the definitions, constructing the desired list */
   for ( i = number_of_definitions; i > 0; i--,currentDef -= 4 ){
      rhs = def_to_reverse_extension_list(currentDef[DEF_START], currentDef[DEF_END], currentDef[DEF_STEP]);
      SeqListNode_addLists(&newList, rhs);
   }
   return newList;
}

/********************************************************************************
 * Creates a reverse list of the possible extensions for a loop whose iterations
 * are defined by start, end, step.  Note the use of push front to make this
 * process O(n).  Going from end to start and using SeqListNode_insertItem would
 * be O(n^2) because insertItem() goes through the whole list every time.
 * Otherwise, we could keep a pointer to the last element of the list, but this
 * is more elegant.
********************************************************************************/
static LISTNODEPTR def_to_reverse_extension_list( int start, int end, int step){
   LISTNODEPTR newList=NULL;
   char buffer[128];
   int loopCount;

   for(loopCount = start; loopCount <= end; loopCount += step){
      sprintf(buffer,"%s%d",EXT_TOKEN,loopCount);
      SeqListNode_pushFront(&newList, buffer);
   }

   return newList;
}


/*
 * returns a list containing ALL the loop extensions for a node that is
 * a child of a loop container or loop containers, defined in the dep_index list of loops iterations. For instance, this 
 * function is used to support dependency wildcard when a node is dependant on all
 * the iterations of a node that is a child of a loop container
 */
LISTNODEPTR SeqLoops_getLoopContainerExtensions( SeqNodeDataPtr _nodeDataPtr, const char * depIndex ) {
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[100], *baseExtension = NULL;
   int expressionArray[256], i, j, startIndex=0, endIndex=1, stepIndex=2, setIndex=3, detectedEnd=0, breakFlag=0, firstFlag=1, detectedStart;
   char *tmpExpression = NULL, *tmpArrayValue = NULL;
   int foundIt = 0, loopStart = 0, loopStep = 1, loopEnd = 0, loopCount = 0;
   LISTNODEPTR loopExtensions = NULL, tmpLoopExts = NULL;
   SeqNameValuesPtr loopsDataPtr = NULL;
   SeqLoopsPtr loopsPtr = NULL; 
   SeqNameValuesPtr depArgs = NULL;

   SeqLoops_parseArgs( &depArgs, depIndex);

   memset( tmp, '\0', sizeof tmp );
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions depIndex: %s\n", depIndex);  

   /* looping over each depedency index argument */
   while (depArgs != NULL){
        SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions dealing with argument %s=%s\n", depArgs->name, depArgs->value);  
	if( strstr( depArgs->value, "*" ) != NULL ) {
	/* wildcard found */
            SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions getting loop information from node %s\n", _nodeDataPtr -> name );  
            loopsPtr = _nodeDataPtr->loops; 
  	    while (loopsPtr != NULL && !foundIt ) {
                 loopsDataPtr =  loopsPtr->values;
                 if (loopsDataPtr != NULL ) {
		     /* find the right loop arg to match*/
                     if (strcmp(SeqUtil_getPathLeaf(loopsPtr->loop_name),depArgs->name)==0) {
		         loopsDataPtr = loopsDataPtr->nextPtr;
			 
			 tmpExpression = SeqLoops_getLoopAttribute( loopsDataPtr, "EXPRESSION" );
			 if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {
			   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions expression: %s\n", tmpExpression);
			 } else {
			   loopStart = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "START" ) );
			   if( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) != NULL ) { 
			      loopStep = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) );
			   }
			   loopEnd = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "END" ) );
			   loopCount = loopStart;
			   SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions loopStart:%d loopStep:%d loopEnd=%d \n", loopStart, loopStep, loopEnd );
			 }
			 foundIt=1;
		     } else {
		         loopsPtr = loopsPtr->nextPtr; 
		     }
                  }
            }

            /* if targetting a loop node as a dependency, _nodeDataPtr->data will have the loop attributes */
            loopsDataPtr=_nodeDataPtr->data; 
	    if ((strcmp(_nodeDataPtr->nodeName,depArgs->name)==0) && (loopsDataPtr != NULL)) {
               SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions targetting a loop node\n");
	       tmpExpression = SeqLoops_getLoopAttribute( loopsDataPtr, "EXPRESSION" );
	       if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {
		  SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions expression: %s\n", tmpExpression);
	       } else {
		  loopStart = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "START" ) );
		  if( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) != NULL ) { 
		     loopStep = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) );
		  }
		  loopEnd = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "END" ) );
		  loopCount = loopStart;
		  SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions loopStart:%d loopStep:%d loopEnd=%d \n", loopStart, loopStep, loopEnd );
	       }
	    }
	    
	    /* parse the loop expression to expressionArray */
	    if (strcmp(tmpExpression, "") != 0) {
	       i=0;
	       tmpArrayValue = strtok (tmpExpression,":,");
	       while (tmpArrayValue != NULL) {
		  expressionArray[i] = atoi(tmpArrayValue);
		  /*End is the biggest number of the expression*/
		  if (expressionArray[i]>detectedEnd)
		     detectedEnd=expressionArray[i];
		  i++;
		  tmpArrayValue = strtok (NULL,":,");
	       }
	       detectedStart=detectedEnd;
	       for (j=0; j<i; j=j+4) {
		  if(expressionArray[j]<detectedStart){
		     detectedStart=expressionArray[j];
		     startIndex=j;
		  }
	       }
	       loopCount=detectedStart;
	       loopEnd=detectedEnd;
	    }

	    /*reset for next loop to find*/
	    foundIt=0;

	} else {
	/* only one value*/
               loopCount = atoi(depArgs->value);
	       loopEnd=loopCount;
               SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopContainerExtensions Found single value for loop: loop name = %s ; loop value = %d \n", depArgs->name, loopCount );
	}

        /* add values to the list of extensions */ 
	do {
	   if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0 && loopEnd != loopCount) {
	       while( loopCount <= detectedEnd ) {
		  loopCount = expressionArray[startIndex];
		  breakFlag = 0;
		  if ((startIndex-4) >= 0 && loopCount != expressionArray[endIndex-4]) {
		     firstFlag = 1;
		  }
		  while( loopCount <= expressionArray[endIndex] && breakFlag == 0){
		     if (firstFlag == 0) {
			if ((loopCount+expressionArray[stepIndex]) <= expressionArray[endIndex]) {
			   loopCount = loopCount + expressionArray[stepIndex];
			} else {
			   breakFlag = 1;
			}
		     } else {
			firstFlag = 0;
		     }
		     if (breakFlag == 0) {
			if( loopExtensions != NULL ) {
			   sprintf( tmp, "%s%s%d", loopExtensions->data, EXT_TOKEN, loopCount );
			} else {
			   sprintf( tmp, "%s%d", EXT_TOKEN, loopCount );
			}
			/*SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopContainerExtensions new extension added based on expression:%s\n", tmp );*/
			SeqListNode_insertItem( &tmpLoopExts, tmp );
		     }
		  }
		  if ((stepIndex+4) < i) {
		     endIndex=endIndex+4;
		     stepIndex=stepIndex+4;
		     startIndex=startIndex+4;
		  } else {
		     break;
		  }
	       }
	    } else {
	       while( loopCount <= loopEnd ) {
		  if( loopExtensions != NULL ) {
		     sprintf( tmp, "%s%s%d", loopExtensions->data, EXT_TOKEN, loopCount );
		  } else {
		     sprintf( tmp, "%s%d", EXT_TOKEN, loopCount );
		  }
		  /* build the tmp list of extensions */
		  SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopContainerExtensions new extension added:%s\n", tmp );
		  SeqListNode_insertItem( &tmpLoopExts, tmp );
		  loopCount = loopCount + loopStep;
	       }
	    }

	        /* if not done, do next */
            if( loopExtensions != NULL ) {
	       loopExtensions = loopExtensions->nextPtr;
	       if (strcmp(tmpExpression, "") != 0)
		  loopCount = detectedStart;
	       else
		  loopCount = loopStart;
            }
		 
        } while( loopExtensions != NULL );


	/* delete the previous list of extensions */
	SeqListNode_deleteWholeList( &loopExtensions );

        /* store the new list of extensions */
	loopExtensions = tmpLoopExts; 

	tmpLoopExts = NULL;

        /* re-init loop values */
        loopCount = 0; loopStep = 1; loopStart = 0; loopEnd = 0; breakFlag = 0; firstFlag = 1;
	detectedEnd = 0; startIndex=0; endIndex=1; stepIndex=2; setIndex=3;
	tmpExpression = NULL; tmpArrayValue = NULL;
        /* get next loop argument */
        depArgs = depArgs->nextPtr;
   } 

   return loopExtensions;

}


/* add escape characters in front of the loop separator character (+)
so that it can be used in pattern matching */
char* SeqLoops_getExtPattern ( char* extension ) {
   char tmp[100];
   int i=0,j=0;
   memset( tmp, '\0', sizeof(tmp) );
   while( extension[i] != '\0' ) {
      if( extension[i] == EXT_TOKEN[0] ) {
         tmp[j] = '\\';
         tmp[j+1] = extension[i];
         j = j+2;
      } else {
         tmp[j] = extension[i];
         j++;
      }
      i++;
   }
   tmp[j] = '\0';
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getExtPattern() return value = %s\n", tmp );   
   return strdup(tmp);
}

/* return the loop arguments from Name-Value list to char pointer */
char* SeqLoops_getLoopArgs( SeqNameValuesPtr _loop_args ) {
   char* value = NULL;
   SeqNameValuesPtr myLoopArgs = _loop_args;
   SeqUtil_stringAppend( &value, "" );
   while( myLoopArgs!= NULL ) {
      SeqUtil_stringAppend( &value, myLoopArgs->name );      
      SeqUtil_stringAppend( &value, "=" );
      SeqUtil_stringAppend( &value, myLoopArgs->value );
      if( myLoopArgs->nextPtr != NULL ) {
         SeqUtil_stringAppend( &value, "," );
      }
      myLoopArgs = myLoopArgs->nextPtr;
   }
   return value;
}

void SeqLoops_printLoopArgs( SeqNameValuesPtr _loop_args, const char* _caller ) {
   char* value = SeqLoops_getLoopArgs( _loop_args );
   SeqUtil_TRACE(TL_FULL_TRACE, "%s loop arguments: %s\n", _caller, value );
   free( value );
}

/* If the user doesn't provide a loop extension, we're returning the first loop iteration
   If the user provides a loop extension, we're returning the current extension
   We're assuming that the _nodeDataPtr->extension has already been set previously
   To be used when the current node is of type loop */
SeqNameValuesPtr SeqLoops_submitLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr newLoopsArgsPtr = NULL, loopArgsTmpPtr = NULL;
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char *loopStart = NULL, *tmpExpression = NULL, *tmpArrayValue = NULL;
   int foundExt = 0, _i, _j, expressionArray[256], detectedEnd=0, detectedStart;
   loopArgsTmpPtr = _loop_args;
   while( loopArgsTmpPtr != NULL ) {
      if( strcmp( loopArgsTmpPtr->name, _nodeDataPtr->nodeName ) == 0 ) {
         /* ok the user has provided an extension */
         foundExt = 1;
         break;
      }
      loopArgsTmpPtr  = loopArgsTmpPtr->nextPtr;
   }
   /* start with same as current iteration */
   newLoopsArgsPtr = SeqNameValues_clone( _loop_args );

   if( ! foundExt ) {
      /* get the first loop iteration */
      nodeSpecPtr = _nodeDataPtr->data;
      /* for now, we only support numerical data */
      
      tmpExpression = SeqLoops_getLoopAttribute( nodeSpecPtr, "EXPRESSION" );
      if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {
         _i=0;
         tmpArrayValue = strtok (tmpExpression,":,");
         while (tmpArrayValue != NULL) {
            expressionArray[_i] = atoi(tmpArrayValue);
            /*End is the biggest number of the expression*/
            if (expressionArray[_i]>detectedEnd)
               detectedEnd=expressionArray[_i];
            _i++;
            tmpArrayValue = strtok (NULL,":,");
         }
         detectedStart=detectedEnd;
         for (_j=0; _j<_i; _j=_j+4) {
            if(expressionArray[_j]<detectedStart){
               detectedStart=expressionArray[_j];
            }
         }
         loopStart = (char *) detectedStart;
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_submitLoopArgs() loopstart:%s\n", loopStart);
         SeqLoops_setLoopAttribute( &newLoopsArgsPtr, _nodeDataPtr->nodeName, loopStart );
 
      } else {
      
         loopStart = SeqLoops_getLoopAttribute( nodeSpecPtr, "START" );
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_submitLoopArgs() loopstart:%s\n", loopStart);
         SeqLoops_setLoopAttribute( &newLoopsArgsPtr, _nodeDataPtr->nodeName, loopStart );
      }
   }
   free( loopStart );
   return newLoopsArgsPtr;
}

/********************************************************************************
 * Given a loop definition given by start, end, step, 
 * returns 1 if the given iteration is part of this definition and
 * returns 0 otherwise
********************************************************************************/
int isInDefinition( int iteration, int start, int end, int step) {
   if( ((start - iteration) % step) == 0 ){
      if( ( step > 0 && start <= iteration && iteration <= end ) ||
          ( step < 0 && end <= iteration && iteration <= start )  ) {
         return 1;
      }
   }
   return 0;
}

/********************************************************************************
 * Returns the last iteration of the definition given by start, end, step
 * (because end may not be the the last iteration of definition).
********************************************************************************/
int lastDefIter( int start, int end, int step){
   return start + step * ((end-start)/step);
}

/********************************************************************************
 * returns 1 if we have reach the last iteration of the loop,
 * returns 0 otherwise
********************************************************************************/
int SeqLoops_isLastIteration( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
	SeqNameValuesPtr nodeSpecPtr = NULL;
	char tmp[20];
	char *loopCurrentStr = NULL, *loopStepStr = NULL, *loopEndStr = NULL, *loopSetStr = NULL;
	char *tmpExpression = NULL, *tmpArrayValue = NULL;
	int loopCurrent = 0, loopStep = 1, loopSet = 1, loopTotal = 0, _i, expressionArray[256];
	int isLast = 0;
	int* lastDef;
	memset( tmp, '\0', sizeof(tmp) );
	/* get the first loop iteration */
	nodeSpecPtr = _nodeDataPtr->data;

	/* for now, we only support numerical data */
	/* get the current iteration from the loop arguments */
	if( (loopCurrentStr = SeqLoops_getLoopAttribute( _loop_args, _nodeDataPtr->nodeName ) ) != NULL )
		loopCurrent = atoi(loopCurrentStr);

	tmpExpression = SeqLoops_getLoopAttribute( nodeSpecPtr, "EXPRESSION" );
	if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {

		/* Parse the expression into an array */
		_i=0;
		tmpArrayValue = strtok (tmpExpression,":,");
		while (tmpArrayValue != NULL) {
			expressionArray[_i++] = atoi(tmpArrayValue);
			tmpArrayValue = strtok (NULL,":,");
		}

		/* LastDef is the last four integers of the expression */
		lastDef = &expressionArray[_i - 4]; 

		/* If current is the last iteration of the last definition, then it is the
		 * last iteration of the whole expression */
		if ( loopCurrent == lastDefIter( lastDef[DEF_START], lastDef[DEF_END], lastDef[DEF_STEP]) ){
			isLast = 1;
		}
	} else {
		/* get the iteration step */
		if( ( loopStepStr = SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) ) != NULL )
			loopStep = atoi(loopStepStr);
		/* get the iteration end */
		if( ( loopEndStr = SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) ) != NULL )
			loopTotal = atoi(loopEndStr);

		fprintf(stdout,"SeqLoops_isLastIteration() loopCurrent:%d loopStep:%d loopTotal:%d\n", loopCurrent, loopStep, loopTotal);

		if( (loopCurrent + loopStep) > loopTotal ) {
			isLast = 1;
		}
	}

	return isLast;
}

/********************************************************************************
 * SeqLoops_nextLoopArgs(): Works with maestro.go_end().  For loops without
 * expressions, either returns loop arguments for the next iteration or returns
 * NULL if there are none.
 *
 * For loops with expressions, the function will return the next iteration of
 * the current definition if there is one.  Otherwise, if the current iteration
 * is the last one of it's definition, the function will return NULL but will
 * inform the caller that there is a new definition to start by specifying the
 * number of the next definition in the parameter _newDefNumber.  If the current
 * iteration is the last of it's definition, and the current definition is the
 * last one, NULL will be returned and 0 will be put in *_newDefNumber
 * indicating to the caller that there is no next iteration to submit and no
 * next definition to start.
********************************************************************************/
SeqNameValuesPtr SeqLoops_nextLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args, int* _newDefNumber ) {

	SeqNameValuesPtr nextLoopArgsPtr = NULL; /* Return value */
	SeqNameValuesPtr nodeSpecPtr = _nodeDataPtr->data;
	const int loopCurrent = atoi( SeqLoops_getLoopAttribute( _loop_args, _nodeDataPtr->nodeName ) );
	char *expression, *token; 
	char nextIterStr[200]; 
	int expressionArray[256], index = 0, startIndex = 0, nextIter,loopStart, loopSet = 1, loopStep = 1, loopEnd;
	int *currentDef;
	int currentDefIsLast; /* Boolean value */ 
	memset( nextIterStr, '\0', sizeof(nextIterStr));

	/* Determine if loopArgs are defined by an expression */
	expression = SeqLoops_getLoopAttribute( nodeSpecPtr, "EXPRESSION" );
	if( expression != NULL && strcmp(expression , "")){

		/* Parse expression into array */
		token = strtok(expression,":,");
		index = 0;
		while (token != NULL){
			expressionArray[index++] = atoi(token);
			token = strtok(NULL, ":,");
		} /* index points after last valid element in expressionArray */

		/* Find the definition that we are currently in */
		startIndex = 0;
		while ( ((startIndex + 4) < index) && !isInDefinition(loopCurrent, expressionArray[startIndex + DEF_START], expressionArray[startIndex + DEF_END] , expressionArray[startIndex + DEF_STEP])  ){
			startIndex += 4;
		}
		currentDefIsLast = ( startIndex + 4 == index ? 1 : 0 );
		currentDef = &(expressionArray[startIndex]);

		/* Decide what to do next */
		nextIter = loopCurrent + currentDef[DEF_SET]*currentDef[DEF_STEP];
		if ( isInDefinition(nextIter, currentDef[DEF_START], currentDef[DEF_END], currentDef[DEF_STEP]) ){ 
			/* Start a new iteration within the current definition */
			sprintf( nextIterStr, "%d", nextIter );
			nextLoopArgsPtr = SeqNameValues_clone( _loop_args );
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_nextLoopArgs(): setting loop attribure of %s\n", _nodeDataPtr->nodeName);
			SeqLoops_setLoopAttribute( &nextLoopArgsPtr, _nodeDataPtr->nodeName, nextIterStr );
		} else if( loopCurrent == lastDefIter(currentDef[DEF_START], currentDef[DEF_END], currentDef[DEF_STEP]) ) {
			if(!currentDefIsLast){
				/* Inform caller that a new definition is to be started by * specifying it's number*/
				*_newDefNumber = (startIndex/4) + 1;
				SeqUtil_TRACE(TL_MEDIUM, "SeqLoops_nextLoopArgs():Informing caller of newdefinition, and also returning newdefinition number = %d within expressionArray.\n",*_newDefNumber);
			}
		}  /* else nextLoopArgsPtr == NULL and *_newDefNumber == 0 indicating loop is finished */
	} /* end if ( loop uses expression ) */
	else
	{
		SeqUtil_TRACE(TL_FULL_TRACE , "SeqLoops_nextLoopArgs(): Running non-expression code \n");
		if( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL ) {
			loopStep = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) );
		}

		if( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) != NULL ) {
			loopSet = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) );
		}

		loopEnd = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) );
		loopStart = atoi ( SeqLoops_getLoopAttribute ( nodeSpecPtr, "START" ) );

		/* calculate next iteration */
		nextIter = loopCurrent + loopStep*loopSet;
		if( isInDefinition(nextIter, loopStart, loopEnd, loopStep) ) {
			sprintf( nextIterStr, "%d", loopCurrent + (loopSet * loopStep) );
         SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_nextLoopArgs(): setting loop attribure of %s\n", _nodeDataPtr->nodeName);
			nextLoopArgsPtr = SeqNameValues_clone( _loop_args );
			SeqLoops_setLoopAttribute( &nextLoopArgsPtr, _nodeDataPtr->nodeName, nextIterStr );
		} /* else nextLoopArgsPtr == NULL indicating loop is finished */
	}

	return nextLoopArgsPtr;
}

/* function that validates loop arguments and builds the extension
fails if validations fails
returns 1 if succeeds
*/

int SeqLoops_validateLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args) {
   SeqLoopsPtr loopsPtr = _nodeDataPtr->loops;
   SeqNameValuesPtr loopArgsTmpPtr = NULL;
   char *loopExtension = NULL;
   char *tmpValue=NULL; 

   /* Check for :last NPT or ForEach arg */
   tmpValue=SeqNameValues_getValue(_loop_args, _nodeDataPtr->nodeName); 
   if  ( (_nodeDataPtr->type == NpassTask || _nodeDataPtr->type == ForEach ) && tmpValue != NULL){
      /*raise flag that node has a ^last*/
      if ( strstr (tmpValue, "^last" ) !=NULL) {
         _nodeDataPtr->isLastArg=1; 
      }
   }

   SeqNode_showLoops(loopsPtr,TL_FULL_TRACE);
   /* validate loop containers */
   if( loopsPtr != NULL ) {
      SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_validateLoopArgs() loop pointer found\n" );
      /* check if the user has given us argument for looping */
      if( _loop_args == NULL ) {
         raiseError( "SeqLoops_validateLoopArgs(): No loop arguments found for container loop!\n" );
      }

      /* build loop extension for containers */
      if( (loopExtension = (char*) SeqLoops_ContainerExtension( loopsPtr, _loop_args )) == NULL ) {
         raiseError( "SeqLoops_validateLoopArgs(): Missing loop arguments for container loop!\n" );
      }
      SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_validateLoopArgs() loop extension:%s\n", loopExtension );
   } else {
      SeqUtil_TRACE(TL_ERROR, "SeqLoops_validateLoopArgs() loop pointer not found\n" );
   }

   /* build extension for current node if loop */
   if( _nodeDataPtr->type == Loop ||  _nodeDataPtr->type == NpassTask  ||  _nodeDataPtr->type == Switch || _nodeDataPtr->type == ForEach ) {
      loopArgsTmpPtr = _loop_args;
      while( loopArgsTmpPtr != NULL ) {
         if( strcmp( loopArgsTmpPtr->name, _nodeDataPtr->nodeName ) == 0 ) {
            SeqUtil_stringAppend( &loopExtension, EXT_TOKEN );
            SeqUtil_stringAppend( &loopExtension, loopArgsTmpPtr->value );
         }
         loopArgsTmpPtr  = loopArgsTmpPtr->nextPtr;
      }
   }

   SeqNode_setExtension( _nodeDataPtr, loopExtension );
   free( loopExtension );
   free( tmpValue); 
   return(1);
}

char* SeqLoops_getExtFromLoopArgs( SeqNameValuesPtr _loop_args ) {
   char* loopExtension = NULL;
   while( _loop_args != NULL ) {
      SeqUtil_stringAppend( &loopExtension, EXT_TOKEN );
      SeqUtil_stringAppend( &loopExtension, _loop_args->value );
      _loop_args  = _loop_args->nextPtr;
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getExtFromLoopArgs: %s\n", loopExtension );
   return loopExtension;
}

/******************************************************************************** 
 * Function SeqLoops_getContainerArgs
 * returns only the loop arguments of parent loop containers For instance, if
 * arguments is: outer_loop=2,inner_loop=3,this function will return
 * outer_loop=2 in the NameValue list
 *
 * The function works by associating tokens in the container path with args of
 * loopArgs.  This way is more robust against name conflicts than taking all the
 * loop_args that don't match the node name.
 *
 * NAME=outer_loop VALUE=2
 *
 * Note: some modifications were made to take into account the possibility of a
 * loop (say Loop1) with a child or descendant by the same name. (say
 * Loop1/task/Loop1).  Some testing will need to be done for NpassTasks.
 ********************************************************************************/
SeqNameValuesPtr SeqLoops_getContainerArgs (const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr current = _loop_args; /* Linked list iterator */
   SeqNameValuesPtr containerArgs = NULL; /* Return value */
   char * path = NULL;
   char * token = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getContainerArgs(): Called for node %s and _loop_args :\n", _nodeDataPtr->taskPath);
   SeqNameValues_printList( _loop_args);
   SeqUtil_TRACE(TL_FULL_TRACE, "_nodeDataPtr->name : %s\n", _nodeDataPtr->name);
   SeqUtil_TRACE(TL_FULL_TRACE, "_nodeDataPtr->container : %s\n", _nodeDataPtr->container);

   /* Container is the full path without the current node */
   path  = strdup(_nodeDataPtr->container);
   token = strtok(path,"/");
   while ( token != NULL && current != NULL ){
      if( strcmp( token, current->name ) == 0 ) {
         /* If the token matches the key, add that to the container args */
         SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getContainerArgs adding loop item %s of value %s \n",  current->name, current->value);
         SeqNameValues_insertItem( &containerArgs, current->name, current->value);
         /* And move on to the next loop_arg */
         current = current->nextPtr;
      }
      token = strtok(NULL,"/");
   }
   free(path);
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getContainerArgs(): returning loopArgs : \n");
   SeqNameValues_printList(containerArgs);
   return containerArgs;
}

/********************************************************************************
 * SeqLoops_getLoopSetArgs()
 * Returns the intial set of iterations for a given definition in an expression
 * specified by defNumber, or the initial set of iterations for a loop defined
 * without the use of an expression.
 * REMARK: If this function is used to return the initial set of iterations for
 * a definition that is not the first one, then NULL should be passed as the
 * second argument (see maestro.c: go_end()).
********************************************************************************/
SeqNameValuesPtr SeqLoops_getLoopSetArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args , int defNumber) {
	SeqNameValuesPtr newLoopsArgsPtr = NULL;
	SeqNameValuesPtr nodeSpecPtr = NULL;
	SeqNameValuesPtr loopArgsTmpPtr = _loop_args;

	char tmp[20];
	int loopStart = 0, loopEnd= 0, loopSet = 1, loopStep = 1, loopCount = 0, loopSetCount = 0;
	char *tmpExpression=NULL, *tmpArrayValue=NULL;
	int _i, expressionArray[256], detectedEnd=0, endIndex=1, startIndex=0, detectedStart;
	memset( tmp, '\0', sizeof(tmp) );
	int foundExt = 0;
	int* currentDef = NULL;
	SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopSetArgs(): Called with defNumber = %d\n",defNumber);

	/* see if the user has provied an ext for this loop */
	while( loopArgsTmpPtr != NULL ) {
		if( strcmp( loopArgsTmpPtr->name, _nodeDataPtr->nodeName ) == 0 ) {
			/* ok the user has provided an extension for the current loop*/
			foundExt = 1;
			break;
		}
		loopArgsTmpPtr  = loopArgsTmpPtr->nextPtr;
	}

	if( ! foundExt ) {
		/* we need to submit the full set */
		nodeSpecPtr = _nodeDataPtr->data;

		/* If loop is defined by an expression */
		tmpExpression = SeqLoops_getLoopAttribute( nodeSpecPtr, "EXPRESSION" );
		if (tmpExpression != NULL && strcmp(tmpExpression, "") != 0) {
			SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopSetArgs() loop expression:%s\n", tmpExpression);

			/* Parse expression into array */
			_i=0;
			tmpArrayValue = strtok (tmpExpression,":,");
			while (tmpArrayValue != NULL) {
				expressionArray[_i++] = atoi(tmpArrayValue);
				tmpArrayValue = strtok (NULL,":,");
			}
         
			/* Current definition starts at position defNumber*4 of expressionArray */
			currentDef = expressionArray + defNumber*4;
			SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopSetArgs():Definition for which to submit initial set is \n");
			SeqUtil_TRACE(TL_FULL_TRACE, "\t Start = %d, end = %d, step = %d, set = %d \n", currentDef[DEF_START], currentDef[DEF_END], currentDef[DEF_STEP], currentDef[DEF_SET]);
			loopCount=currentDef[DEF_START];
			/* Deal with the possibility of current definition's start being equal to end of previous definition */
			if( defNumber > 0 && currentDef[DEF_START] == currentDef[DEF_END - 4])
				loopCount = loopCount + currentDef[DEF_STEP];

			/* calculate next set of iterations */
			while (loopCount <= currentDef[DEF_END] && loopSetCount < currentDef[DEF_SET]) {
				sprintf( tmp, "%d", loopCount );
				SeqNameValues_insertItem( &newLoopsArgsPtr,  _nodeDataPtr->nodeName, tmp);
				loopCount = loopCount + currentDef[DEF_STEP];
				loopSetCount++;
			}
			SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_getLoopSetArgs():Returning newLoopsArgsPtr with contents: \n");
			SeqNameValues_printList( newLoopsArgsPtr );
		} else {
			/* Loop is defined without expression */
			if( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL ) { 
				loopStep = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) );
			}
			if( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) != NULL ) { 
				loopSet = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) );
			}
			loopStart = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "START" ) );
			loopEnd = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) );

			SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_getLoopSetArgs() loopstart:%d loopSet:%d loopStep:%d loopEnd:%d\n", loopStart, loopSet, loopStep, loopEnd);

			loopCount = loopStart;
			/* calculate initial set of iterations */
			while( loopCount <= loopEnd && loopSetCount < loopSet ) {
				sprintf( tmp, "%d", loopCount );
				SeqNameValues_insertItem( &newLoopsArgsPtr,  _nodeDataPtr->nodeName, tmp);
				loopCount=loopCount+loopStep;
				loopSetCount++;
			}
		}
	} else {
		/* we need to submit only one iteration */
		newLoopsArgsPtr = SeqNameValues_clone( loopArgsTmpPtr );
	}
	/* SeqNameValues_printList( newLoopsArgsPtr); */
	return newLoopsArgsPtr;
}


/* checks if values are valid for a numerical loop in an expression format, so step !=0, set !=0, loop start <= loop end when step is positive, and loop start >= loop end when step is negative */ 
 
void SeqLoops_validateNumLoopExpression( char * _expression){
   char *tmpExpression = _expression, *expressionEnd = NULL; 
   char expressionBuffer[1024]; 
   int tmpStart =0, tmpEnd=0, tmpStep =0, tmpSet=0, done=0; 
   size_t expressionSize=0;

   while ((tmpExpression != NULL) && (done == 0)) { 
         SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_validateNumLoopExpression working expression %s \n", tmpExpression);
         if ((expressionEnd=strstr(tmpExpression,",")) == NULL) {
            expressionSize=strlen(tmpExpression)+1; 
            done=1;
         } else {
            expressionSize=expressionEnd - tmpExpression +1; 
         } 
         memset(expressionBuffer,'\0',sizeof(expressionBuffer));
         snprintf(expressionBuffer,expressionSize,"%s",tmpExpression); 
         SeqUtil_TRACE(TL_FULL_TRACE, "SeqLoops_validateNumLoopExpression checking sub-expression %s \n", expressionBuffer);
         if ( sscanf(expressionBuffer,"%d:%d:%d:%d",&tmpStart,&tmpEnd,&tmpStep,&tmpSet) != 4) { 
            raiseError("SeqLoops_validateNumLoopExpression format error in expression.Is %s; Accepted format is comma-separated list of start:end:step:set.\n", expressionBuffer);
         }
         if (tmpStep == 0) {
            raiseError("SeqLoops_validateNumLoopExpression value error. loop step (%d) cannot be 0. Readjust value.\n",tmpStep);
         }
         if (tmpSet <= 0) {
            raiseError("SeqLoops_validateNumLoopExpression value error. loop set (%d) must be greater than 0. Readjust value.\n",tmpSet);
         }

         if ((tmpStart > tmpEnd) && (tmpStep > 0 )) {
            raiseError("SeqLoops_validateNumLoopExpression value error. loop start (%d) > loop end (%d)  with loop step > 0 (%d) . Readjust values.\n",tmpStart,  tmpEnd , tmpStep);
         } 
         if ((tmpStart < tmpEnd) && (tmpStep < 0)) {
            raiseError("SeqLoops_validateNumLoopExpression value error. loop start (%d) < loop end (%d)  with loop step < 0 (%d) . Readjust values.\n",tmpStart , tmpEnd ,tmpStep);
         }
         tmpExpression+=expressionSize; 
   }
}







