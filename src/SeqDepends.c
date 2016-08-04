#include <stdlib.h>
#include "SeqDepends.h"
#include "SeqUtil.h"

SeqDepDataPtr SeqDep_newDep(void)
{
   SeqDepDataPtr newDep = malloc( sizeof(*newDep) );

   newDep->type = NodeDependancy;
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
