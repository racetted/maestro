#include <stdlib.h>
#include "SeqDepends.h"
#include "SeqUtil.h"

/********************************************************************************
 * Allocate and intialize a new SeqDepData object
********************************************************************************/
SeqDepDataPtr SeqDep_newDep(void)
{
   SeqDepDataPtr newDep = malloc( sizeof(*newDep) );

   newDep->type = NodeDependancy;
   newDep->expScope = IntraSuite;
   newDep->isInScope = -1; /* Invalid value */
   newDep->node_name = NULL;
   newDep->node_path = NULL;
   newDep->exp = NULL;
   newDep->status = NULL;
   newDep->index = NULL;
   newDep->ext = NULL;
   newDep->local_index = NULL;
   newDep->local_ext = NULL;
   newDep->hour = NULL;
   newDep->time_delta = NULL;
   newDep->datestamp = NULL;
   newDep->valid_hour = NULL;
   newDep->valid_dow = NULL;
   newDep->protocol = NULL;

   return newDep;
}

/********************************************************************************
 * Frees up the memory from a DepDataObject and sets the pointer to NULL
********************************************************************************/
void SeqDep_deleteDep(SeqDepDataPtr * dep)
{
   SeqDepDataPtr the_dep = *dep;
   free(the_dep->node_name);
   free(the_dep->node_path);
   free(the_dep->exp);
   free(the_dep->status);
   free(the_dep->index);
   free(the_dep->ext);
   free(the_dep->local_index);
   free(the_dep->local_ext);
   free(the_dep->hour);
   free(the_dep->time_delta);
   free(the_dep->datestamp);
   free(the_dep->valid_hour);
   free(the_dep->valid_dow);
   free(the_dep->protocol);

   free(the_dep);
   dep = NULL;
}

/********************************************************************************
 * Prints the data in a SeqDepData object.  It would be a good idea to make it
 * only print the non-NULL values.
********************************************************************************/
void SeqDep_printDep(int trace_level, SeqDepDataPtr dep)
{
   const char *typeString;
   if( dep->type == NodeDependancy ){
      typeString = "NodeDependency";
   } else {
      typeString = "DateDependency";
   }

   SeqUtil_TRACE(trace_level,"  type = %s\n",typeString);
   SeqUtil_TRACE(trace_level,"  node_name = %s\n", dep->node_name);
   SeqUtil_TRACE(trace_level,"  node_path = %s\n", dep->node_path);
   SeqUtil_TRACE(trace_level,"  exp = %s\n", dep->exp);
   SeqUtil_TRACE(trace_level,"  status = %s\n", dep->status);
   SeqUtil_TRACE(trace_level,"  index = %s\n", dep->index);
   SeqUtil_TRACE(trace_level,"  ext = %s\n", dep->ext);
   SeqUtil_TRACE(trace_level,"  local_index = %s\n", dep->local_index);
   SeqUtil_TRACE(trace_level,"  local_ext = %s\n", dep->local_ext);
   SeqUtil_TRACE(trace_level,"  hour = %s\n", dep->hour);
   SeqUtil_TRACE(trace_level,"  time_delta = %s\n", dep->time_delta);
   SeqUtil_TRACE(trace_level,"  datestamp = %s\n", dep->datestamp);
   SeqUtil_TRACE(trace_level,"  valid_hour = %s\n", dep->valid_hour);
   SeqUtil_TRACE(trace_level,"  valid_dow = %s\n", dep->valid_dow);
   SeqUtil_TRACE(trace_level,"  protocol = %s\n", dep->protocol);
}


/********************************************************************************
 * Allocates and initializes a new SeqDepNode object and returns a pointer to
 * the new object.
********************************************************************************/
SeqDepNodePtr newDepNode(SeqDepDataPtr depData)
{
   SeqDepNodePtr newDepNode = malloc( sizeof(*newDepNode) );
   if( newDepNode == NULL )
      raiseError("ERROR: malloc() failed in newDepNode()\n");

   newDepNode->nextPtr = NULL;
   newDepNode->depData = depData;

   return newDepNode;
}

/********************************************************************************
 * Appends a dependency node at the end of the list pointed to by *list_head
********************************************************************************/
void SeqDep_addDepNode(SeqDepNodePtr* list_head, SeqDepDataPtr depData)
{
   SeqDepNodePtr newNode = newDepNode(depData);
   SeqDepNodePtr current = *list_head;

   if( *list_head == NULL ){
      *list_head = newNode;
   } else {
      while( current->nextPtr != NULL ){
         current = current->nextPtr;
      }
      current->nextPtr = newNode;
   }
}

/********************************************************************************
 * Frees the resources for a SeqDepNodePtr linked list of dependency data.
********************************************************************************/
void SeqDep_deleteDepList(SeqDepNodePtr* list_head)
{
   SeqDepNodePtr current = *list_head,
                 tmp_next;
   while(current != NULL){
      tmp_next = current->nextPtr;
      SeqDep_deleteDep(&(current->depData));
      free(current);
      current = tmp_next;
   }
   *list_head = NULL;
}
