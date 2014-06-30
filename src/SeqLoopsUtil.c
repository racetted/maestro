#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "SeqListNode.h"
#include "SeqNameValues.h"
#include "SeqNode.h"
#include "SeqUtil.h"
#include <string.h>
/*
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/stat.h>
*/


static char* EXT_TOKEN = "+";


/* function to parse the loop arguments
   returns 0 if succeeds
   returns -1 if the function fais with an error
   cmd_args must be in the form "loop_name=value,loop_namex=valuex"
*/

int SeqLoops_parseArgs( SeqNameValuesPtr* nameValuesPtr, const char* cmd_args ) {
   char *tmpstrtok = NULL, *tmp_args = NULL;
   char *loopName=NULL, *loopValue=NULL;
   int isError = 0;
   
   SeqUtil_TRACE( "SeqLoops_parseArgs cmd_args:%s\n", cmd_args );
   /*
   memset(loopName,'\0',sizeof loopName);
   memset(loopValue,'\0',sizeof loopValue);
   */
   
   tmp_args = strdup( cmd_args );
   tmpstrtok = (char*) strtok( tmp_args, "," );
   while ( tmpstrtok != NULL ) {
      /* any alphanumeric characters and special chars
         _:/-$()* are supported */
      loopName=NULL;
      loopValue=NULL;
      loopName=malloc(100);
      loopValue=malloc(50);

      sscanf( tmpstrtok, "%[A-Za-z0-9._:/]=%[A-Za-z0-9._^/-$()*]", loopName, loopValue );

      /* should add more syntax validation such as spaces not allowed... */
      if ( strlen( loopName ) == 0 || strlen( loopValue ) == 0 ) {
         isError = -1;
      } else {
         SeqNameValues_insertItem( nameValuesPtr, loopName, loopValue );
      }
      tmpstrtok = (char*) strtok(NULL,",");
      free(loopName);
      free(loopValue);
   }
   SeqUtil_TRACE("SeqLoops_parseArgsdone exit status: %d\n", isError ); 
   free( tmp_args );
   return(isError);
}

/* function that returns the base extension of a node.
examples: 
extension=_1_2_3_4 will return _1_2_3
extension=_1 will return ""
*/
char* SeqLoops_getExtensionBase ( SeqNodeDataPtr _nodeDataPtr ) {
   char *split = NULL , *tmpExtension = NULL, *work_string = NULL ,*chreturn =NULL;
   int containerCount = 0;
   SeqLoopsPtr loopsContainerPtr = _nodeDataPtr->loops;
   work_string = strdup(_nodeDataPtr->extension);
   if( _nodeDataPtr->type == Loop || _nodeDataPtr->type == NpassTask ||  _nodeDataPtr->type == Switch  ) {
      while( loopsContainerPtr != NULL ) {
         containerCount++;
         loopsContainerPtr = loopsContainerPtr->nextPtr;
      }
      split  = strrchr (work_string,EXT_TOKEN[0]);
      if (split != NULL) {
         *split = '\0';
      }
   }
   chreturn = strdup(work_string);
   free( tmpExtension );
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

   while ( tmpptr != NULL ) {
      if( strcmp( tmpptr->name, attr_name ) == 0 ) {
         returnValue = strdup( tmpptr->value );
         break;
      }
      tmpptr = tmpptr->nextPtr;
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
      SeqUtil_TRACE( "SeqLoops_setLoopAttribute looking for:%s found:%s \n", attr_name, loopAttrPtr->name );
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
      SeqUtil_TRACE("SeqLoops_ContainerExtension loopLeafName=%s\n", loopLeafName ); 
      thisLoopArgPtr = loop_args_ptr;
      while( thisLoopArgPtr != NULL && isError == 0 ) {
         SeqUtil_TRACE("SeqLoops_ContainerExtension loop_name=%s loop_value=%s\n", thisLoopArgPtr->name, thisLoopArgPtr->value ); 
         if( strcmp( thisLoopArgPtr->name, loopLeafName ) == 0 ) {
            foundLoopArg = 1;
            SeqUtil_stringAppend( &extension, EXT_TOKEN );
            SeqUtil_stringAppend( &extension, thisLoopArgPtr->value );
            SeqUtil_TRACE("SeqLoops_ContainerExtension found loop argument loop_name=%s loop_value=%s\n", thisLoopArgPtr->name, thisLoopArgPtr->value ); 
            break;
         }
         thisLoopArgPtr = thisLoopArgPtr->nextPtr;
      }

      isError = !foundLoopArg;

      free( loopLeafName );
      loops_ptr  = loops_ptr->nextPtr;
   }

   if( isError == 1 )
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
   int loopStart = 0, loopStep = 1, loopEnd = 0, loopCount = 0;
   LISTNODEPTR loopExtensions = NULL;
   memset( tmp, '\0', sizeof tmp );
   baseExtension = SeqLoops_getExtensionBase( _nodeDataPtr );
   nodeSpecPtr = _nodeDataPtr->data;

   loopStart = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "START" ) );
   if( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL ) { 
      loopStep = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) );
   }
   loopEnd = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) );
   SeqUtil_TRACE("SeqLoops_childExtensionsInReverse extension:%s baseExtension:%s START:%d END:%d\n",_nodeDataPtr->extension, baseExtension, loopEnd, loopStart );
   loopCount = loopEnd;
   while( loopCount >= loopStart ) {
      sprintf( tmp, "%s%s%d", baseExtension, EXT_TOKEN, loopCount );
      SeqListNode_insertItem( &loopExtensions, tmp );
      loopCount = loopCount - loopStep;
   }

   return loopExtensions;
}



/* return the loop container extension values that the current node is in.
 * Current node must be a loop node.
   To be used to check if a loop parent node is done,
   assuming that extension is set in  _nodeDataPtr->extension */
LISTNODEPTR SeqLoops_childExtensions( SeqNodeDataPtr _nodeDataPtr ) {
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[20], *baseExtension;
   int loopStart = 0, loopStep = 1, loopEnd = 0, loopCount = 0;
   LISTNODEPTR loopExtensions = NULL;
   memset( tmp, '\0', sizeof tmp );
   baseExtension = SeqLoops_getExtensionBase( _nodeDataPtr );
   nodeSpecPtr = _nodeDataPtr->data;

   SeqUtil_TRACE("SeqLoops_childExtensions extension:%s baseExtension:%s START=%s\n",_nodeDataPtr->extension, baseExtension, SeqLoops_getLoopAttribute( nodeSpecPtr, "START" )  );
   loopStart = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "START" ) );
   if( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL ) { 
      loopStep = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) );
   }
   loopEnd = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) );
   loopCount = loopStart;
   while( loopCount <= loopEnd ) {
      sprintf( tmp, "%s%s%d", baseExtension, EXT_TOKEN, loopCount );
      SeqListNode_insertItem( &loopExtensions, tmp );
      loopCount = loopCount + loopStep;
   }

   return loopExtensions;
}

/* returns 1 if the parent of a node is a loop container */
int SeqLoops_isParentLoopContainer ( const SeqNodeDataPtr _nodeDataPtr ) {
   int value = 0;
   SeqLoopsPtr loopsPtr =  _nodeDataPtr->loops;
   SeqUtil_TRACE( "SeqLoops_isParentLoopContainer.isParentLoopContainer() container = %s\n", _nodeDataPtr->container );
   while( loopsPtr != NULL ) {
      SeqUtil_TRACE( "SeqLoops_isParentLoopContainer.isParentLoopContainer() container loop_name = %s\n", loopsPtr->loop_name );
      if( strcmp( loopsPtr->loop_name, _nodeDataPtr->container ) == 0 ) {
         value = 1;
         break;
      }
      loopsPtr  = loopsPtr->nextPtr;
   }
   SeqUtil_TRACE( "SeqLoops_isParentLoopContainer.isParentLoopContainer() return value = %d\n", value );
   return value;
}


/*
 * returns a list containing ALL the loop extensions for a node that is
 * a child of a loop container or loop containers, defined in the dep_index list of loops iterations. For instance, this 
 * function is used to support dependency wildcard when a node is dependant on all
 * the iterations of a node that is a child of a loop container 
 * The list is returned end first, to optimize normal operating procedures. 
 */
LISTNODEPTR SeqLoops_getLoopContainerExtensionsInReverse( SeqNodeDataPtr _nodeDataPtr, const char * depIndex ) {
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[100], *baseExtension = NULL;
   int foundIt = 0, loopStart = 0, loopStep = 1, loopEnd = 0, loopCount = 0;
   LISTNODEPTR loopExtensions = NULL, tmpLoopExts = NULL;
   SeqNameValuesPtr loopsDataPtr = NULL;
   SeqLoopsPtr loopsPtr = NULL; 
   SeqNameValuesPtr depArgs = NULL;

   SeqLoops_parseArgs( &depArgs, depIndex);

   memset( tmp, '\0', sizeof tmp );
   SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse depIndex: %s\n", depIndex);  

   /* looping over each depedency index argument */
   while (depArgs != NULL){
        SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse dealing with argument %s=%s\n", depArgs->name, depArgs->value);  
	if( strstr( depArgs->value, "*" ) != NULL ) {
	/* wildcard found */
            SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse getting loop information from node %s\n", _nodeDataPtr -> name );  
            loopsPtr = _nodeDataPtr->loops; 
  	    while (loopsPtr != NULL && !foundIt ) {
                 loopsDataPtr =  loopsPtr->values;
                 if (loopsDataPtr != NULL ) {
                     SeqUtil_TRACE("%s\n", loopsPtr->loop_name);
		     /* find the right loop arg to match*/
                     if (strcmp(SeqUtil_getPathLeaf(loopsPtr->loop_name),depArgs->name)==0) {
		         loopsDataPtr = loopsDataPtr->nextPtr;
                         loopStart = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "START" ) );
                         if( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) != NULL ) { 
                             loopStep = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) );
                         }
                         loopEnd = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "END" ) );
                         loopCount = loopEnd;
                         SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse loopStart:%d loopStep:%d loopEnd:%d \n", loopStart, loopStep, loopEnd );
			 foundIt=1;
		     } else {
		         loopsPtr = loopsPtr->nextPtr; 
		     }
                  }
            }

            /* if targetting a loop node as a dependency, _nodeDataPtr->data will have the loop attributes */
            loopsDataPtr=_nodeDataPtr->data; 
	    if ((strcmp(_nodeDataPtr->nodeName,depArgs->name)==0) && (loopsDataPtr != NULL)) {
               SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse targetting a loop node\n");  
	       loopStart = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "START" ) );
               if( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) != NULL ) { 
                   loopStep = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) );
               }
               loopEnd = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "END" ) );
               loopCount = loopEnd;
               SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse loopStart:%d loopStep:%d loopEnd:%d \n", loopStart, loopStep, loopEnd );
	    }

	    /*reset for next loop to find*/
	    foundIt=0;

	} else {
	/* only one value*/
               loopCount = atoi(depArgs->value);
	       loopStart=loopCount;
               SeqUtil_TRACE("SeqLoops_getLoopContainerExtensionsInReverse Found single value for loop: loop name = %s ; loop value = %d \n", depArgs->name, loopCount );
	}

        /* add values to the list of extensions */ 
	do {

            while( loopCount >= loopStart ) {
                if( loopExtensions != NULL ) {
	           sprintf( tmp, "%s%s%d", loopExtensions->data, EXT_TOKEN, loopCount );
                } else {
	           sprintf( tmp, "%s%d", EXT_TOKEN, loopCount );
	        }
	        /* build the tmp list of extensions */
	        SeqUtil_TRACE( "SeqLoops_getLoopContainerExtensionsInReverse new extension added:%s\n", tmp );
                SeqListNode_insertItem( &tmpLoopExts, tmp );
                loopCount = loopCount - loopStep;
            }

	        /* if not done, do next */
            if( loopExtensions != NULL ) {
	       loopExtensions = loopExtensions->nextPtr;
               loopCount = loopEnd;
            }
		 
        } while( loopExtensions != NULL );


	/* delete the previous list of extensions */
	SeqListNode_deleteWholeList( &loopExtensions );

        /* store the new list of extensions */
	loopExtensions = tmpLoopExts; 

	tmpLoopExts = NULL;

        /* re-init loop values */
        loopCount = 0; loopStep = 1; loopStart = 0; loopEnd = 0;
        /* get next loop argument */
        depArgs = depArgs->nextPtr;
   } 

   return loopExtensions;

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
   int foundIt = 0, loopStart = 0, loopStep = 1, loopEnd = 0, loopCount = 0;
   LISTNODEPTR loopExtensions = NULL, tmpLoopExts = NULL;
   SeqNameValuesPtr loopsDataPtr = NULL;
   SeqLoopsPtr loopsPtr = NULL; 
   SeqNameValuesPtr depArgs = NULL;

   SeqLoops_parseArgs( &depArgs, depIndex);

   memset( tmp, '\0', sizeof tmp );
   SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions depIndex: %s\n", depIndex);  

   /* looping over each depedency index argument */
   while (depArgs != NULL){
        SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions dealing with argument %s=%s\n", depArgs->name, depArgs->value);  
	if( strstr( depArgs->value, "*" ) != NULL ) {
	/* wildcard found */
            SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions getting loop information from node %s\n", _nodeDataPtr -> name );  
            loopsPtr = _nodeDataPtr->loops; 
  	    while (loopsPtr != NULL && !foundIt ) {
                 loopsDataPtr =  loopsPtr->values;
                 if (loopsDataPtr != NULL ) {
                     SeqUtil_TRACE("%s\n", loopsPtr->loop_name);
		     /* find the right loop arg to match*/
                     if (strcmp(SeqUtil_getPathLeaf(loopsPtr->loop_name),depArgs->name)==0) {
		         loopsDataPtr = loopsDataPtr->nextPtr;
                         loopStart = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "START" ) );
                         if( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) != NULL ) { 
                             loopStep = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) );
                         }
                         loopEnd = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "END" ) );
                         loopCount = loopStart;
                         SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions loopStart:%d loopStep:%d loopEnd=%d \n", loopStart, loopStep, loopEnd );
			 foundIt=1;
		     } else {
		         loopsPtr = loopsPtr->nextPtr; 
		     }
                  }
            }

            /* if targetting a loop node as a dependency, _nodeDataPtr->data will have the loop attributes */
            loopsDataPtr=_nodeDataPtr->data; 
	    if ((strcmp(_nodeDataPtr->nodeName,depArgs->name)==0) && (loopsDataPtr != NULL)) {
               SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions targetting a loop node\n");  
	       loopStart = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "START" ) );
               if( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) != NULL ) { 
                   loopStep = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "STEP" ) );
               }
               loopEnd = atoi( SeqLoops_getLoopAttribute( loopsDataPtr, "END" ) );
               loopCount = loopStart;
               SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions loopStart:%d loopStep:%d loopEnd=%d \n", loopStart, loopStep, loopEnd );
	    }

	    /*reset for next loop to find*/
	    foundIt=0;

	} else {
	/* only one value*/
               loopCount = atoi(depArgs->value);
	       loopEnd=loopCount;
               SeqUtil_TRACE("SeqLoops_getLoopContainerExtensions Found single value for loop: loop name = %s ; loop value = %d \n", depArgs->name, loopCount );
	}

        /* add values to the list of extensions */ 
	do {

            while( loopCount <= loopEnd ) {
                if( loopExtensions != NULL ) {
	           sprintf( tmp, "%s%s%d", loopExtensions->data, EXT_TOKEN, loopCount );
                } else {
	           sprintf( tmp, "%s%d", EXT_TOKEN, loopCount );
	        }
	        /* build the tmp list of extensions */
	        SeqUtil_TRACE( "SeqLoops_getLoopContainerExtensions new extension added:%s\n", tmp );
                SeqListNode_insertItem( &tmpLoopExts, tmp );
                loopCount = loopCount + loopStep;
            }

	        /* if not done, do next */
            if( loopExtensions != NULL ) {
	       loopExtensions = loopExtensions->nextPtr;
               loopCount = loopStart;
            }
		 
        } while( loopExtensions != NULL );


	/* delete the previous list of extensions */
	SeqListNode_deleteWholeList( &loopExtensions );

        /* store the new list of extensions */
	loopExtensions = tmpLoopExts; 

	tmpLoopExts = NULL;

        /* re-init loop values */
        loopCount = 0; loopStep = 1; loopStart = 0; loopEnd = 0;
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
   SeqUtil_TRACE( "SeqLoops_getExtPattern() return value = %s\n", tmp );   
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
   SeqUtil_TRACE ( "%s loop arguments: %s\n", _caller, value );
   free( value );
}

/* If the user doesn't provide a loop extension, we're returning the first loop iteration
   If the user provides a loop extension, we're returning the current extension
   We're assuming that the _nodeDataPtr->extension has already been set previously
   To be used when the current node is of type loop */
SeqNameValuesPtr SeqLoops_submitLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr newLoopsArgsPtr = NULL, loopArgsTmpPtr = NULL;
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char *loopStart = NULL;
   int foundExt = 0;
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
      loopStart = SeqLoops_getLoopAttribute( nodeSpecPtr, "START" );
      fprintf(stdout,"SeqLoops_submitLoopArgs() loopstart:%s\n", loopStart);
      SeqLoops_setLoopAttribute( &newLoopsArgsPtr, _nodeDataPtr->nodeName, loopStart );
   }
   free( loopStart );
   return newLoopsArgsPtr;
}

/* returns 1 if we have reach the last iteration of the loop,
   returns 0 otherwise
*/
int SeqLoops_isLastIteration( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[20];
   char *loopCurrentStr = NULL, *loopStepStr = NULL, *loopEndStr = NULL, *loopSetStr = NULL;
   int loopCurrent = 0, loopStep = 1, loopSet = 1, loopTotal = 0;
   int isLast = 0;
   memset( tmp, '\0', sizeof(tmp) );
   /* get the first loop iteration */
   nodeSpecPtr = _nodeDataPtr->data;

   /* for now, we only support numerical data */
   /* get the current iteration from the loop arguments */
   if( (loopCurrentStr = SeqLoops_getLoopAttribute( _loop_args, _nodeDataPtr->nodeName ) ) != NULL )
      loopCurrent = atoi(loopCurrentStr);
   /* get the iteration step */
   if( ( loopStepStr = SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) ) != NULL )
      loopStep = atoi(loopStepStr);
   /* if the set has a value, the next iteration is (current iteration + set value)*/
   if( ( loopSetStr = SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) ) != NULL )
      loopSet = atoi(loopSetStr);
   /* get the iteration end */
   if( ( loopEndStr = SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) ) != NULL )
      loopTotal = atoi(loopEndStr);

   fprintf(stdout,"SeqLoops_isLastIteration() loopCurrent:%d loopStep:%d loopTotal:%d\n", loopCurrent, loopStep, loopTotal);

   /* have we reached the end? */
   if( (loopCurrent + (loopSet * loopStep)) > loopTotal ) {
      isLast = 1;
   }
   
   return isLast;
}

/* returns the next loop iteration of the current loop node as a
   name-value argument list so that it can be used in an maestro call.
   returns NULL if we have reached the end
   */
SeqNameValuesPtr SeqLoops_nextLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr newLoopsArgsPtr = NULL;
   SeqNameValuesPtr nodeSpecPtr = NULL;
   char tmp[20];
   int loopCurrent = 0, loopSet = 1 , loopStep = 1, loopTotal = 0;
   memset( tmp, '\0', sizeof(tmp) );
   /* get the first loop iteration */
   nodeSpecPtr = _nodeDataPtr->data;
   /* for now, we only support numerical data */
   loopCurrent = atoi( SeqLoops_getLoopAttribute( _loop_args, _nodeDataPtr->nodeName ) );
   if( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL ) { 
      loopStep = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) );
   }
   if( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) != NULL ) { 
      loopSet = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) );
   }

   loopTotal = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) );

   fprintf(stdout,"SeqLoops_nextLoopArgs() loopCurrent:%d loopSet:%d loopStep:%d loopTotal:%d\n", loopCurrent, loopSet, loopStep, loopTotal);

   /* calculate next iteration */
   if( (loopCurrent + (loopSet * loopStep)) <= loopTotal ) {
      sprintf( tmp, "%d", loopCurrent + (loopSet * loopStep) );
      newLoopsArgsPtr = SeqNameValues_clone( _loop_args );
      SeqLoops_setLoopAttribute( &newLoopsArgsPtr, _nodeDataPtr->nodeName, tmp );
   }
   return newLoopsArgsPtr;
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

   /* Check for :last NPT arg */
   tmpValue=SeqNameValues_getValue(_loop_args, _nodeDataPtr->nodeName); 
   if  (_nodeDataPtr->type == NpassTask && tmpValue != NULL){
         /*raise flag that node has a ^last*/
         if ( strstr (tmpValue, "^last" ) !=NULL) {
            _nodeDataPtr->isLastNPTArg=1; 
         }
    }

   /* validate loop containers */
   if( loopsPtr != NULL ) {
      SeqUtil_TRACE( "SeqLoops_validateLoopArgs() loop pointer found\n" );
      /* check if the user has given us argument for looping */
      if( _loop_args == NULL ) {
         raiseError( "SeqLoops_validateLoopArgs(): No loop arguments found for container loop!\n" );
      }

      /* build loop extension for containers */
      if( (loopExtension = (char*) SeqLoops_ContainerExtension( loopsPtr, _loop_args )) == NULL ) {
         raiseError( "SeqLoops_validateLoopArgs(): Missing loop arguments for container loop!\n" );
      }
      SeqUtil_TRACE( "SeqLoops_validateLoopArgs() loop extension:%s\n", loopExtension );
   }

   /* build extension for current node if loop */
   if( _nodeDataPtr->type == Loop ||  _nodeDataPtr->type == NpassTask  ||  _nodeDataPtr->type == Switch ) {
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
   SeqUtil_TRACE( "SeqLoops_getExtFromLoopArgs: %s\n", loopExtension );
   return loopExtension;
}

/* returns only the loop arguments of parent loop containers
 * For instance, if  arguments is: outer_loop=2,inner_loop=3,
 * this function will return outer_loop=2 in the NameValue list
 *
 * NAME=outer_loop VALUE=2
 *
 *
 */
SeqNameValuesPtr SeqLoops_getContainerArgs (const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr loopArgsTmpPtr = _loop_args;
   SeqNameValuesPtr newLoopsArgsPtr = NULL;
   while( loopArgsTmpPtr != NULL ) {
      if( strcmp( loopArgsTmpPtr->name, _nodeDataPtr->nodeName ) == 0 ) {
         break;
      }
      SeqUtil_TRACE( "SeqLoops_getContainerArgs adding loop item %s of value %s \n",  loopArgsTmpPtr->name, loopArgsTmpPtr->value);

      SeqNameValues_insertItem( &newLoopsArgsPtr,  loopArgsTmpPtr->name, loopArgsTmpPtr->value);
      loopArgsTmpPtr  = loopArgsTmpPtr->nextPtr;
   }
   return newLoopsArgsPtr;
}

/* returns the loop arguments for the first set iterations for a loop set
 * however, if the _loop_args already contains a value for the current loop,
 * it means the set is already started, we will only submit the (current + set) iteration
 * Example Set loop 3, start value=1, the returned NameValue list is something like:
 *            NAME: outer_loop VALUE=1
 *            NAME: outer_loop VALUE=2
 *            NAME: outer_loop VALUE=3
 */
SeqNameValuesPtr SeqLoops_getLoopSetArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args ) {
   SeqNameValuesPtr newLoopsArgsPtr = NULL;
   SeqNameValuesPtr nodeSpecPtr = NULL;
   SeqNameValuesPtr loopArgsTmpPtr = _loop_args;


   char tmp[20];
   int loopStart = 0, loopEnd= 0, loopSet = 1, loopStep = 1,
       loopCount = 0, loopSetCount = 0;
   memset( tmp, '\0', sizeof(tmp) );

   int foundExt = 0;
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

      loopStart = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "START" ) );
      if( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) != NULL ) { 
         loopStep = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "STEP" ) );
      }
      if( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) != NULL ) { 
         loopSet = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "SET" ) );
      }
      loopEnd = atoi( SeqLoops_getLoopAttribute( nodeSpecPtr, "END" ) );
      fprintf(stdout,"SeqLoops_getLoopSetArgs() loopstart:%d loopSet:%d loopStep:%d loopEnd:%d\n", loopStart, loopSet, loopStep, loopEnd);

      loopCount = loopStart;
      /* calculate next iteration */
      while( loopCount <= loopEnd && loopSetCount < loopSet ) {
         sprintf( tmp, "%d", loopCount );
         SeqNameValues_insertItem( &newLoopsArgsPtr,  _nodeDataPtr->nodeName, tmp);
         loopCount=loopCount+loopStep;
         loopSetCount++;
      }
   } else {
      /* we need to submit only one iteration */
      newLoopsArgsPtr = SeqNameValues_clone( loopArgsTmpPtr );
   }
   /* SeqNameValues_printList( newLoopsArgsPtr); */
   return newLoopsArgsPtr;
}
