/* SeqNode.c - Node construct definitions and utility functions used by the Maestro sequencer software package.
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
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <unistd.h>
#include "SeqNode.h"
#include "SeqUtil.h"
#include "SeqLoopsUtil.h"
#include "SeqNameValues.h"
#include "SeqUtilServer.h"
#include "nodeinfo_filters.h"

static char* FamilyTypeString = "Family";
static char* TaskTypeString = "Task";
static char* NpassTaskTypeString = "NpassTask";
static char* LoopTypeString = "Loop";
static char* ModuleTypeString = "Module";
static char* SwitchTypeString = "Switch"; 
static char* ForEachTypeString = "ForEach";

/* this function is just a simple enabling of printf calls when
the user passes -d option */
char* SeqNode_getTypeString( SeqNodeType _node_type ) {
   char* typePtr = NULL;
   switch (_node_type) {
      case Task:
         typePtr = TaskTypeString;
         break;
      case NpassTask:
         typePtr = NpassTaskTypeString;
         break;
      case Family:
         typePtr = FamilyTypeString;
         break;
      case Module:
         typePtr = ModuleTypeString;
         break;
      case Loop:
         typePtr = LoopTypeString;
         break;
      case ForEach:
         typePtr = ForEachTypeString;
         break;
      case Switch:
         typePtr = SwitchTypeString;
         break;
      default:
         typePtr = TaskTypeString;
   }
   return typePtr;
}

void SeqNode_setName ( SeqNodeDataPtr node_ptr, const char* name ) {
   if ( name != NULL ) {
      free( node_ptr->name );
      if (node_ptr->name = malloc( strlen(name) + 1 )) {
          strcpy( node_ptr->name, name );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setName()\n");
      }
   }
}

void SeqNode_setNodeName ( SeqNodeDataPtr node_ptr, const char* nodeName ) {
   if ( nodeName != NULL ) {
      free( node_ptr->nodeName );
      if (node_ptr->nodeName = malloc( strlen(nodeName) + 1 )) {
         strcpy( node_ptr->nodeName, nodeName );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setNodeName()\n");
      }
   }
}

void SeqNode_setModule ( SeqNodeDataPtr node_ptr, const char* module ) {
   if ( module != NULL ) {
      free( node_ptr->module );
      if (node_ptr->module = malloc( strlen(module) + 1 )) {
          strcpy( node_ptr->module, module );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setModule()\n");
      }
   }
}

void SeqNode_setPathToModule ( SeqNodeDataPtr node_ptr, const char* pathToModule ) {
   if ( pathToModule != NULL ) {
      free( node_ptr->pathToModule );
      if (node_ptr->pathToModule = malloc( strlen(pathToModule) + 1 )){
          strcpy( node_ptr->pathToModule, pathToModule );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setPathToModule()\n");
      }
   }
}

void SeqNode_setIntramoduleContainer ( SeqNodeDataPtr node_ptr, const char* intramodule_container ) {
   if( intramodule_container != NULL ) {
      free( node_ptr->intramodule_container );
      if (node_ptr->intramodule_container = malloc( strlen(intramodule_container) + 1 )) {
          strcpy( node_ptr->intramodule_container, intramodule_container );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setIntramoduleContainer()\n");
      }
   }
}

void SeqNode_setContainer ( SeqNodeDataPtr node_ptr, const char* container ) {
   if( container != NULL ) {
      free( node_ptr->container );
      if (node_ptr->container = malloc( strlen(container) + 1 )) {
          strcpy( node_ptr->container, container );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setContainer()\n");
      }
   }
}

void SeqNode_setCpu ( SeqNodeDataPtr node_ptr, const char* cpu ) {
   char *tmpstrtok=NULL;
   char *tmpCpu=NULL;
   if ( cpu != NULL ) {
      free( node_ptr->cpu );
      if (node_ptr->cpu = malloc( strlen(cpu) + 1 )){
          strcpy( node_ptr->cpu, cpu );
          tmpCpu=strdup(cpu);
      } else {
          raiseError("OutOfMemory exception in SeqNode_setCpu()\n");
      }
  
      /* parse NPEX */
      tmpstrtok = (char*) strtok( tmpCpu, "x" );
      if ( tmpstrtok != NULL ) {
          free( node_ptr->npex );
	  if (node_ptr->npex=malloc( strlen(tmpstrtok) +1)){ 
              strcpy(node_ptr->npex, tmpstrtok);
          } else {
              raiseError("OutOfMemory exception in SeqNode_setCpu()\n");
          }
      }
      /* NPEY */
      tmpstrtok = (char*) strtok( NULL, "x" );
      if ( tmpstrtok != NULL ) {
          free( node_ptr->npey );
	  if (node_ptr->npey=malloc( strlen(tmpstrtok) +1)){ 
   	     strcpy(node_ptr->npey, tmpstrtok);
          } else {
              raiseError("OutOfMemory exception in SeqNode_setCpu()\n");
          }
      }
      /* OMP */
      tmpstrtok = (char*) strtok( NULL, "x" );
      if ( tmpstrtok != NULL ) {
          free( node_ptr->omp );
	  if (node_ptr->omp=malloc( strlen(tmpstrtok) +1)){ 
  	      strcpy(node_ptr->omp, tmpstrtok);
          } else {
              raiseError("OutOfMemory exception in SeqNode_setCpu()\n");
          }
      }
   free (tmpCpu);
   }
}

void SeqNode_setCpu_new ( SeqNodeDataPtr node_ptr, const char* cpu ) {
   char * strPtr=cpu;
   int value1=0, value2=0,value3=0;
   size_t x_count=0;
   if ( cpu != NULL ) {
      free( node_ptr->cpu );
      if (node_ptr->cpu = malloc( strlen(cpu) + 1 )){
          strcpy( node_ptr->cpu, cpu );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setCpu()\n");
      }  

      /*find count of "x" separator*/
      for (x_count=0; strPtr[x_count]; strPtr[x_count]=='x' ? x_count++ : *(strPtr++));

      SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_setCpu() cpu=%s, x-separator count=%d\n",cpu,x_count);

      switch (x_count) {
         case 0:
            if (sscanf(cpu,"%d",&value1) == 1 ) {
               /* 1 value matching, so value1 = OMP when not mpi, npex when mpi, resetting the other value in case*/ 
               if (node_ptr->mpi == 0) { 
                  free( node_ptr->npex);
                  free( node_ptr->omp); 
                  (node_ptr->omp=malloc(10)) !=NULL ? snprintf( node_ptr->omp,10,"%d",value1) : raiseError("OutOfMemory Exception"); 
                  node_ptr->npex=strdup("1");
               } else {
                  free( node_ptr->npex); 
                  free( node_ptr->omp); 
                  (node_ptr->npex=malloc(10)) !=NULL ? snprintf( node_ptr->npex,10,"%d",value1) : raiseError("OutOfMemory Exception"); 
                  node_ptr->omp=strdup("1"); 
               }
            } else  raiseError("Format error in cpu %s. Should be 1x1x1, 1x1 or 1.\n",cpu);
            break ; 
         case 1: 
            if (sscanf(cpu,"%dx%d",&value1, &value2) == 2 ) {
               /* 2 value matching, so value1=npex; value2 = OMP */ 
               free( node_ptr->npex); 
               free( node_ptr->omp); 
               (node_ptr->npex=malloc(10)) !=NULL ? snprintf( node_ptr->npex,10,"%d",value1) : raiseError("OutOfMemory Exception"); 
               (node_ptr->omp=malloc(10)) !=NULL ? snprintf( node_ptr->omp,10,"%d",value2) : raiseError("OutOfMemory Exception"); 
            }   else  raiseError("Format error in cpu %s. Should be 1x1x1, 1x1 or 1.\n",cpu);
            break ; 
         case 2: 
            if (sscanf(cpu,"%dx%dx%d",&value1, &value2, &value3) == 3) {
               free( node_ptr->npex); 
               free( node_ptr->npey); 
               free( node_ptr->omp); 
               (node_ptr->npex=malloc(10)) !=NULL ? snprintf( node_ptr->npex,10,"%d",value1) : raiseError("OutOfMemory Exception"); 
               (node_ptr->npey=malloc(10)) !=NULL ? snprintf( node_ptr->npey,10,"%d",value2) : raiseError("OutOfMemory Exception"); 
               (node_ptr->omp=malloc(10)) !=NULL ? snprintf( node_ptr->omp,10,"%d",value3) : raiseError("OutOfMemory Exception"); 
            } else   raiseError("Format error in cpu %s. Should be 1x1x1, 1x1 or 1.\n",cpu);
            break ; 
         default: raiseError("Format error in cpu %s. Should be 1x1x1, 1x1 or 1.\n",cpu);
      } 
   }
}

void SeqNode_setCpuMultiplier ( SeqNodeDataPtr node_ptr, const char* cpu_multiplier ) {
  char *tmpMult=NULL;
  char *tmpMultTok=NULL;
  int mult=1;
  tmpMult = strdup(cpu_multiplier);
  if ( cpu_multiplier != NULL ) {
    tmpMultTok = (char*) strtok( tmpMult, "x" );
    while ( tmpMultTok != NULL ) {
      mult = mult * atoi(tmpMultTok);
      tmpMultTok = (char*) strtok( NULL, "x" );
    }
    sprintf(tmpMult,"%d",mult);
    free( node_ptr->cpu_multiplier );
    if (node_ptr->cpu_multiplier = malloc( strlen(cpu_multiplier) + 1 )){
        strcpy( node_ptr->cpu_multiplier, tmpMult );
    } else {
        raiseError("OutOfMemory exception in SeqNode_setCpuMultiplier()\n");
    }
    free (tmpMultTok);
  }
   free (tmpMult);
}

void SeqNode_setMachine ( SeqNodeDataPtr node_ptr, const char* machine ) {
   if ( machine != NULL ) {
      free( node_ptr->machine );
      if (node_ptr->machine = malloc( strlen(machine) + 1 )){
          strcpy( node_ptr->machine, machine );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setMachine()\n");
      }
   }
}

void SeqNode_setShell ( SeqNodeDataPtr node_ptr, const char* shell ) {
   if ( shell != NULL ) {
      free( node_ptr->shell );
      if (node_ptr->shell = malloc( strlen(shell) + 1 )){
         strcpy( node_ptr->shell, shell );
      } else {
         raiseError("OutOfMemory exception in SeqNode_setShell()\n");
      }
   }
}

void SeqNode_setMemory ( SeqNodeDataPtr node_ptr, const char* memory ) {
   if ( memory != NULL ) {
      free( node_ptr->memory );
      if (node_ptr->memory = malloc( strlen(memory) + 1 )){
          strcpy( node_ptr->memory, memory );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setMemory()\n");
      }
   }
}

void SeqNode_setQueue ( SeqNodeDataPtr node_ptr, const char* queue ) {
   if ( queue != NULL ) {
      free( node_ptr->queue );
      if (node_ptr->queue = malloc( strlen(queue) + 1 )){
          strcpy( node_ptr->queue, queue );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setQueue()\n");
      }
   }
}

void SeqNode_setSuiteName ( SeqNodeDataPtr node_ptr, const char* suiteName ) {
   if ( suiteName != NULL ) {
      free( node_ptr->suiteName );
      if (node_ptr->suiteName = malloc( strlen(suiteName) + 1 )){
          strcpy( node_ptr->suiteName, suiteName );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setSuiteName()\n");
      }
   }
}

void SeqNode_setSeqExpHome ( SeqNodeDataPtr node_ptr, const char* expHome ) {
   if ( expHome != NULL ) {
      free( node_ptr->expHome );
      if (node_ptr->expHome = malloc( strlen(expHome) + 1 )){
          strcpy( node_ptr->expHome, expHome );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setSeqExpHome()\n");
      }
   }
}


void SeqNode_setInternalPath ( SeqNodeDataPtr node_ptr, const char* path ) {
   if ( path != NULL ) {
      free( node_ptr->taskPath );
      if (node_ptr->taskPath = malloc( strlen(path) + 1 )){
          strcpy( node_ptr->taskPath, path );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setInternalPath()\n");
      }
   }
}

void SeqNode_setArgs ( SeqNodeDataPtr node_ptr, const char* args ) {
   if ( args != NULL ) {
      free( node_ptr->args );
      if (node_ptr->args = malloc( strlen(args) + 1 )){
          strcpy( node_ptr->args, args );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setArgs()\n");
      }
   }
}

void SeqNode_setWorkerPath ( SeqNodeDataPtr node_ptr, const char* workerPath ) {
   if ( workerPath != NULL ) {
      free( node_ptr->workerPath );
      if (node_ptr->workerPath = malloc( strlen(workerPath) + 1 )){
          strcpy( node_ptr->workerPath, workerPath );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setWorkerPath()\n");
      }
   }
}

void SeqNode_setSoumetArgs ( SeqNodeDataPtr node_ptr, char* soumetArgs ) {
   if ( soumetArgs != NULL ) {
      free( node_ptr->soumetArgs );
      if (node_ptr->soumetArgs = malloc( strlen(soumetArgs) + 1 )){
          strcpy( node_ptr->soumetArgs, soumetArgs );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setSoumetArgs()\n");
      }
   }
}

void SeqNode_setLoopArgs ( SeqNodeDataPtr node_ptr, SeqNameValuesPtr _loop_args ) {
   if ( _loop_args != NULL ) {
      SeqNameValues_deleteWholeList( &(node_ptr->loop_args) );
      node_ptr->loop_args = SeqNameValues_clone( _loop_args );

   }
}
void SeqNode_setAlias ( SeqNodeDataPtr node_ptr, const char* alias ) {
   if ( alias != NULL ) {
      free( node_ptr->alias );
      if (node_ptr->alias = malloc( strlen(alias) + 1 )){
          strcpy( node_ptr->alias, alias );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setAlias()\n");
      }
   }
}

void SeqNode_setDatestamp( SeqNodeDataPtr node_ptr, const char* datestamp) {
   if ( datestamp != NULL ) {
      free( node_ptr->datestamp );
      if (node_ptr->datestamp = malloc( strlen(datestamp) + 1 )){
          strcpy( node_ptr->datestamp, datestamp );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setDatestamp()\n");
      }
   }
}

void SeqNode_setSubmitOrigin( SeqNodeDataPtr node_ptr, const char* submitOrigin) {
   if ( submitOrigin != NULL ) {
      free( node_ptr->submitOrigin );
      if (node_ptr->submitOrigin = malloc( strlen(submitOrigin) + 1 )){
          strcpy( node_ptr->submitOrigin, submitOrigin );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setSubmitOrigin()\n");
      }
   }
}

void SeqNode_setWorkdir( SeqNodeDataPtr node_ptr, const char* workdir) {
   if ( workdir != NULL ) {
      free( node_ptr->workdir);
      if (node_ptr->workdir = malloc( strlen(workdir) + 1 )){
          strcpy( node_ptr->workdir, workdir);
      } else {
          raiseError("OutOfMemory exception in SeqNode_setWorkdir()\n");
      }
   }
}

void SeqNode_setExtension ( SeqNodeDataPtr node_ptr, const char* extension ) {
   if ( extension != NULL ) {
      free( node_ptr->extension );
      if (node_ptr->extension = malloc( strlen(extension) + 1 )){
          strcpy( node_ptr->extension, extension );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setExtension()\n");
      }
   }
}

/* returns ptr to newly allocated memory structure
 to store a new name value link list */
SeqLoopsPtr SeqNode_allocateLoopsEntry ( SeqNodeDataPtr node_ptr ) {
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_allocateLoopsEntry()\n" );
   if ( node_ptr->loops == NULL ) {
      if (node_ptr->loops = malloc( sizeof (SeqLoops) )){
          loopsPtr = node_ptr->loops;
      } else {
          raiseError("OutOfMemory exception in SeqNode_allocateLoopsEntry()\n");
      }
   } else {
      loopsPtr = node_ptr->loops;
      /* position ourselves at the end of the list */
      while( loopsPtr->nextPtr != NULL ) {
         loopsPtr = loopsPtr->nextPtr;
      }
   
      /* allocate memory for new data */
      if (loopsPtr->nextPtr = malloc( sizeof(SeqLoops) )){
          /* go to memory for new data */
          loopsPtr = loopsPtr->nextPtr;
      } else {
          raiseError("OutOfMemory exception in SeqNode_allocateLoopsEntry()\n");
      }
   }
   loopsPtr->nextPtr = NULL;
   loopsPtr->values = NULL;
   loopsPtr->loop_name = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_allocateLoopsEntry() done\n" );
   return loopsPtr;
}

/* returns ptr to newly allocated memory structure 
   to store a new name value link list */
SeqDependenciesPtr SeqNode_allocateDepsEntry ( SeqNodeDataPtr node_ptr ) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_allocateDepsEntry()\n" );
   if ( node_ptr->depends == NULL ) {
      if (node_ptr->depends = malloc( sizeof (SeqDependencies) )){
          deps_ptr = node_ptr->depends;
      } else {
          raiseError("OutOfMemory exception in SeqNode_allocateDepsEntry()\n");
      }
   } else {
      deps_ptr = node_ptr->depends;
      /* position ourselves at the end of the list */
      while( deps_ptr->nextPtr != NULL ) {
         deps_ptr = deps_ptr->nextPtr;
      }

      /* allocate memory for new data */
      if (deps_ptr->nextPtr = malloc( sizeof(SeqDependencies) )){
          /* go to memory for new data */
          deps_ptr = deps_ptr->nextPtr;
      } else {
          raiseError("OutOfMemory exception in SeqNode_allocateDepsEntry()\n");
      }
   }
   deps_ptr->nextPtr = NULL;
   deps_ptr->dependencyItem = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_allocateDepsEntry() done\n" );
   return deps_ptr;
}

/* returns ptr to newly allocated memory structure
   to store a name value pair */
SeqNameValuesPtr SeqNode_allocateDepsNameValue ( SeqDependenciesPtr deps_ptr ) {
   SeqNameValuesPtr nameValuesPtr = NULL;

   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_allocateDepsNameValue()\n" );
   assert( deps_ptr != NULL );
   if ( deps_ptr->dependencyItem == NULL ) {
      deps_ptr->dependencyItem = malloc( sizeof (SeqNameValues) );
      nameValuesPtr = deps_ptr->dependencyItem;
   } else {
      nameValuesPtr = deps_ptr->dependencyItem;
      /* position ourselves at the end of the list */
      while( nameValuesPtr->nextPtr != NULL ) {
         nameValuesPtr = nameValuesPtr->nextPtr;
      }

      /* allocate memory for new data */
      if (nameValuesPtr->nextPtr = malloc( sizeof(SeqNameValues) )){
          /* go to memory for new data */
          nameValuesPtr = nameValuesPtr->nextPtr;
      } else {
          raiseError("OutOfMemory exception in SeqNode_allocatedDepsNameValue()\n");
      }
   }
   /* set end of list */
   nameValuesPtr->nextPtr = NULL;
   nameValuesPtr->name = NULL;
   nameValuesPtr->value = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_allocateDepsNameValue() done\n" );
   return nameValuesPtr;
}

/* allocates and store the name-value pair in the
   name-value structure */
void SeqNode_setDepsNameValue (SeqNameValuesPtr name_values_ptr, char* name, char* value ) {
   /* printf ("SeqNode_setDepsNameValue name=%s value=%s\n", name , value ); */
   if (name_values_ptr->name = malloc( sizeof(char) * ( strlen( name ) + 1) )){
       strcpy( name_values_ptr->name, (char*)name );
   } else {
       raiseError("OutOfMemory exception in SeqNode_setDepsNameValue()\n");
   }
   if ( value != NULL ) {
      if (name_values_ptr->value = malloc( sizeof(char) * ( strlen( value ) + 1) )){
         strcpy( name_values_ptr->value, (char*)value );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setDepsNameValue()\n");
      }
   } else {
      if (name_values_ptr->value = malloc( sizeof(char) * 2)){
          strcpy( name_values_ptr->value, "" );
      } else {
          raiseError("OutOfMemory exception in SeqNode_setDepsNameValue()\n");
      }
   }
}
/* add dependency of type node i.e. tasks/family  */
void SeqNode_addNodeDependency ( SeqNodeDataPtr node_ptr, SeqDepDataPtr dep) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL;
   /*
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_addNodeDependency() dep_node=%s, dep_node_path=%s, dep_exp=%s, dep_status=%s, dep_index=%s, local_index=%s, dep_Prot=%s,dep_ValidHour=%s, dep_ValidDOW=%s \n",
      dep_node_name, dep_node_path, dep_exp, dep_status, dep_index, local_index, dep_Prot, dep_ValidHour, dep_ValidDOW);
   */
   SeqDep_printDep(TL_FULL_TRACE,dep);
   deps_ptr = SeqNode_allocateDepsEntry( node_ptr );
   deps_ptr->type = dep->type;
   nameValuesPtr = deps_ptr->dependencyItem;
   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "NAME", dep->node_name );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "INDEX", dep->index );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "EXP", dep->exp );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "STATUS", dep->status );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "LOCAL_INDEX", dep->local_index );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "HOUR", dep->hour );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "TIME_DELTA", dep->time_delta );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "PROT", dep->protocol );
 
   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "VALID_HOUR", dep->valid_hour );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "VALID_DOW", dep->valid_dow );

}

/* add dependency of type date i.e. not sure how we will implement this yet */
void SeqNode_addDateDependency ( SeqNodeDataPtr node_ptr, char* date ) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL;

   deps_ptr = SeqNode_allocateDepsEntry( node_ptr );
   deps_ptr->type = DateDependancy;
   nameValuesPtr = deps_ptr->dependencyItem;

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "DATE", date );
}

void SeqNode_addSubmit ( SeqNodeDataPtr node_ptr, char* data ) {
   SeqListNode_insertItem( &(node_ptr->submits), data );
}

void SeqNode_addSibling ( SeqNodeDataPtr node_ptr, char* data ) {
   SeqListNode_insertItem( &(node_ptr->siblings), data );
}

void SeqNode_addAbortAction ( SeqNodeDataPtr node_ptr, char* data ) {
   SeqListNode_insertItem( &(node_ptr->abort_actions), data );
}

/* default numerical loop with start, step, set, end or an expression with START1:END1:STEP1:SET1,STARTN:ENDN:STEPN:SETN,... */

void SeqNode_addNumLoop ( SeqNodeDataPtr node_ptr, char* loop_name, char* start, char* step, char* set, char* end, char* expression ) {
   SeqLoopsPtr loopsPtr = NULL;
   char newExpression[128] = {'\0'};
   char *tmpStart = start, *tmpStep = step, *tmpSet = set, *tmpEnd = end, *tmpExpression = expression;
   char *defFile = NULL, *value = NULL;
   
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_addNumLoop() input loop_name=%s, start=%s, step=%s, set=%s, end=%s, expression=%s, \n",loop_name, start, step, set, end, expression );

   defFile = malloc ( strlen ( node_ptr->expHome ) + strlen("/resources/resources.def") + 1 );
   sprintf( defFile, "%s/resources/resources.def", node_ptr->expHome );
   
   if (strcmp(tmpExpression, "") == 0) {
      if (strstr(start, "${") != NULL) {
         if ( (value = SeqUtil_keysub( start, defFile, NULL, node_ptr->expHome)) != NULL ){
            strcpy(tmpStart,value);
         }
      }
      if (strstr(step, "${") != NULL) {
         if ( (value = SeqUtil_keysub( step, defFile, NULL, node_ptr->expHome)) != NULL ){
            strcpy(tmpStep,value);
         }
      }
      if (strstr(set, "${") != NULL) {
         if ( (value = SeqUtil_keysub( set, defFile, NULL, node_ptr->expHome)) != NULL ){
            strcpy(tmpSet,value);
         }
      }
      if (strstr(end, "${") != NULL) {
         if ( (value = SeqUtil_keysub( end, defFile, NULL, node_ptr->expHome)) != NULL ){
            strcpy(tmpEnd,value);
         }
      }
      SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_addNumLoop() resulting loop_name=%s, start=%s, step=%s, set=%s, end=%s, \n",loop_name, tmpStart, tmpStep, tmpSet, tmpEnd );
      /* change to expression format for validation -> start:end:step:set */ 
      memset(newExpression,'\0', strlen(newExpression)+1);
      sprintf(newExpression,"%s:%s:%s:%s",tmpStart,tmpEnd,tmpStep,tmpSet); 
      SeqLoops_validateNumLoopExpression(newExpression);
   } else {
      if (strstr(expression, "${") != NULL) {
         if ( (value = SeqUtil_keysub( expression, defFile, NULL, node_ptr->expHome)) != NULL ){
            strcpy(tmpExpression,value);
         }
      }
      SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_addNumLoop() resulting loop_name=%s, expression:%s, \n",loop_name, tmpExpression );
      SeqLoops_validateNumLoopExpression(tmpExpression);
   }

   loopsPtr = SeqNode_allocateLoopsEntry( node_ptr );
   loopsPtr->type = Numerical;
   loopsPtr->loop_name = strdup( loop_name );
   SeqNameValues_insertItem( &loopsPtr->values, "TYPE", "Default");
   SeqNameValues_insertItem( &loopsPtr->values, "START", tmpStart );
   SeqNameValues_insertItem( &loopsPtr->values, "STEP", tmpStep );
   SeqNameValues_insertItem( &loopsPtr->values, "SET", tmpSet );
   SeqNameValues_insertItem( &loopsPtr->values, "END", tmpEnd );
   SeqNameValues_insertItem( &loopsPtr->values, "EXPRESSION", tmpExpression );
   free(defFile);
}

void SeqNode_addSwitch ( SeqNodeDataPtr _nodeDataPtr, const char* switchName, const char* switchType, const const char* returnValue) {
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode_addSwitch() switchName=%s switchType=%s returnValue=%s\n", switchName, switchType, returnValue);
   loopsPtr = SeqNode_allocateLoopsEntry( _nodeDataPtr );
   loopsPtr->type = SwitchType;
   loopsPtr->loop_name = strdup( switchName );
   SeqNameValues_insertItem( &loopsPtr->values, "TYPE", switchType);
   SeqNameValues_insertItem( &loopsPtr->values, "VALUE", returnValue );
}



void SeqNode_addSpecificData ( SeqNodeDataPtr node_ptr, const char* name, const char* value ) {
   char* tmp = NULL;
   int count = 0;
   /* to allow easy comparison, I convert everthing to upper case */
   tmp = strdup( name );
   while( name[count] != '\0' ) {
      tmp[count] = toupper(name[count]);
      count++;
   }
   tmp[count] = '\0';
   /* SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_addSpecificData() called name:%s value:%s\n", tmp, value ); */
   SeqNameValues_insertItem( &(node_ptr->data), tmp, value );
   free( tmp );
}

void SeqNode_setError ( SeqNodeDataPtr node_ptr, const char* message ) {
   node_ptr->error = 1;
   if (node_ptr->errormsg = malloc( strlen( message ) + 1 )){
       strcpy( node_ptr->errormsg, message );
   } else {
       raiseError("OutOfMemory exception in SeqNode_setError()\n");
   }
}

void SeqNode_initForEachTarget( SeqForEachTargetPtr target ) {
   target->index = NULL;
   target->exp = NULL;
   target->hour = NULL;
   target->node = NULL;
}

void SeqNode_freeForEachTarget( SeqForEachTargetPtr target) {
   if (target != NULL) {
      free(target->index); 
      free(target->exp); 
      free(target->hour); 
      free(target->node); 
      free(target); 
   }
} 

void SeqNode_setForEachTarget(SeqNodeDataPtr nodePtr, const char * t_node,  const char * t_index,  const char * t_exp,  const char * t_hour) {

   SeqForEachTargetPtr forEachTargetPtr = NULL; 
   if (nodePtr->forEachTarget != NULL) {
      SeqNode_freeForEachTarget(nodePtr->forEachTarget);
   }

   if (forEachTargetPtr = malloc( sizeof( SeqForEachTarget ) )){
      nodePtr->forEachTarget = forEachTargetPtr;
      SeqNode_initForEachTarget ( nodePtr->forEachTarget );
   }  else {
      raiseError("OutOfMemory exception in SeqNode_setForEachTarget()\n");
   }

   if (t_node != NULL) nodePtr->forEachTarget->node=strdup(t_node); 
   if (t_index != NULL) nodePtr->forEachTarget->index=strdup(t_index); 
   if (t_exp != NULL) nodePtr->forEachTarget->exp=strdup(t_exp); 
   if (t_hour != NULL) nodePtr->forEachTarget->hour=strdup(t_hour); 

} 

void SeqNode_init ( SeqNodeDataPtr nodePtr ) {
   nodePtr->type = Task;
   nodePtr->name = NULL;
   nodePtr->nodeName = NULL;
   nodePtr->container = NULL;
   nodePtr->intramodule_container = NULL;
   nodePtr->module = NULL;
   nodePtr->cpu = NULL;
   nodePtr->npex = NULL;
   nodePtr->npey = NULL;
   nodePtr->omp = NULL; 
   nodePtr->queue = NULL;
   nodePtr->machine = NULL;
   nodePtr->memory = NULL;
   nodePtr->silent = 0;
   nodePtr->catchup = 4;
   nodePtr->wallclock = 5;
   nodePtr->mpi = 0;
   nodePtr->isLastArg = 0;
   nodePtr->immediateMode = 0;
   nodePtr->cpu_multiplier = NULL;
   nodePtr->alias = NULL;
   nodePtr->args = NULL;
   nodePtr->soumetArgs = NULL;
   nodePtr->depends = NULL;
   nodePtr->submits = NULL;
   nodePtr->abort_actions = NULL;
   nodePtr->siblings = NULL;
   nodePtr->loops = NULL;
   nodePtr->loop_args = NULL;
   nodePtr->data = NULL;
   nodePtr->taskPath = NULL;
   nodePtr->pathToModule = NULL;
   nodePtr->suiteName = NULL;
   nodePtr->expHome = NULL;
   nodePtr->extension = NULL;
   nodePtr->datestamp = NULL;
   nodePtr->submitOrigin = NULL;
   nodePtr->switchAnswers = NULL;
   nodePtr->forEachTarget = NULL;
   nodePtr->workdir = NULL;
   nodePtr->workerPath= NULL;
   nodePtr->shell= NULL;
   SeqNode_setName( nodePtr, "" );
   SeqNode_setContainer( nodePtr, "" );
   SeqNode_setIntramoduleContainer( nodePtr, "" );
   SeqNode_setModule( nodePtr, "" );
   SeqNode_setPathToModule( nodePtr, "" );
   SeqNode_setCpu( nodePtr, "1" );
   SeqNode_setCpuMultiplier( nodePtr, "1" );
   SeqNode_setQueue( nodePtr, "null" );
   SeqNode_setMachine( nodePtr, "" );
   SeqNode_setMemory( nodePtr, "500M" );
   SeqNode_setArgs( nodePtr, "" );
   SeqNode_setSoumetArgs( nodePtr, "" );
   SeqNode_setWorkerPath( nodePtr, "");
   SeqNode_setSubmitOrigin( nodePtr, "");
   SeqNode_setAlias( nodePtr, "" );
   SeqNode_setInternalPath( nodePtr, "" );
   SeqNode_setExtension( nodePtr, "" );
   SeqNode_setDatestamp( nodePtr, "" );
   SeqNode_setWorkdir( nodePtr, "" );
   SeqNode_setShell( nodePtr, "" );
   nodePtr->error = 0;
   nodePtr->errormsg = NULL;
}
void SeqNode_showLoops(SeqLoopsPtr loopsPtr,int trace_level){
   SeqLoopsPtr current;
   SeqUtil_TRACE(trace_level,"SeqNode_printLoops():\n");
   for(current = loopsPtr; current != NULL; current = current->nextPtr){
      SeqUtil_TRACE(trace_level, "====loop_name=%s, type=%d, values:\n", current->loop_name, current->type);
      SeqNameValues_printList(current->values);
   }
   SeqUtil_TRACE(TL_FULL_TRACE,"SeqNode_printLoops() end\n");
}

void SeqNode_printForEachTargets(FILE *file, SeqNodeDataPtr node_ptr)
{
   if (node_ptr->forEachTarget->node != NULL)
      SeqUtil_printOrWrite(file,"node.ForEachTarget.node=%s\n", node_ptr->forEachTarget->node );
   if (node_ptr->forEachTarget->index != NULL)
      SeqUtil_printOrWrite(file,"node.ForEachTarget.index=%s\n", node_ptr->forEachTarget->index );
   if (node_ptr->forEachTarget->exp != NULL)
      SeqUtil_printOrWrite(file,"node.ForEachTarget.exp=%s\n", node_ptr->forEachTarget->exp );
   if (node_ptr->forEachTarget->hour != NULL)
      SeqUtil_printOrWrite(file,"node.ForEachTarget.hour=%s\n", node_ptr->forEachTarget->hour );
}
void SeqNode_printNodeSpecifics(FILE *file, SeqNodeDataPtr node_ptr)
{
   /*SeqUtil_printOrWrite(filename,"************ Node Specific Data \n"); */
   SeqNameValuesPtr nodeData = node_ptr->data;
   while (nodeData != NULL ) {
      SeqUtil_printOrWrite(file,"node.specific.%s=%s\n", nodeData->name, nodeData->value );
      nodeData = nodeData->nextPtr;
   }
}
void SeqNode_printSubmits(FILE *file,SeqNodeDataPtr node_ptr )
{
   /*SeqUtil_printOrWrite(filename,"************ Node Submits \n"); */
   LISTNODEPTR submitsPtr = node_ptr->submits;
   while (submitsPtr != NULL) {
      SeqUtil_printOrWrite(file,"node.submit=%s\n", submitsPtr->data);
      submitsPtr = submitsPtr->nextPtr;
   }
}
void SeqNode_printAborts( FILE * file, SeqNodeDataPtr node_ptr)
{
   /*SeqUtil_printOrWrite(filename,"************ Node Abort Actions \n"); */
   LISTNODEPTR abortsPtr = node_ptr->abort_actions;
   while (abortsPtr != NULL) {
      SeqUtil_printOrWrite(file,"node.abortaction=%s\n", abortsPtr->data);
      abortsPtr = abortsPtr->nextPtr;
   }
}
void SeqNode_printLoops( FILE* file , SeqNodeDataPtr node_ptr)
{
   /*SeqUtil_printOrWrite(filename,"************ Containing Loops \n"); */
   SeqLoopsPtr loopsPtr = node_ptr->loops;
   SeqNameValuesPtr nodeData = NULL;
   while (loopsPtr != NULL) {
      /*SeqUtil_printOrWrite(filename,"************ Loop \n"); */
      SeqUtil_printOrWrite(file,"node.loop_parent.name=%s\n", loopsPtr->loop_name);  
      nodeData = loopsPtr->values;
      while (nodeData != NULL ) {
         SeqUtil_printOrWrite(file,"node.loop_parent.%s=%s\n", nodeData->name, nodeData->value );
         nodeData = nodeData->nextPtr;
      }
      loopsPtr = loopsPtr->nextPtr;
   }
}
void SeqNode_printSiblings(FILE * file, SeqNodeDataPtr node_ptr )
{
   /*SeqUtil_printOrWrite(filename,"************ Node Siblings \n"); */
   LISTNODEPTR siblingsPtr = node_ptr->siblings;
   while (siblingsPtr != NULL) {
      SeqUtil_printOrWrite(file,"node.sibling=%s\n", siblingsPtr->data);
      siblingsPtr = siblingsPtr->nextPtr;
   }
}
const char *SeqNode_getCfgPath( SeqNodeDataPtr node_ptr){
   char cfgPath[SEQ_MAXFIELD] = {0};
   if( node_ptr->type == Task || node_ptr->type == NpassTask ) {
      sprintf(cfgPath,"/modules%s.cfg", node_ptr->taskPath);
   } else {
      sprintf(cfgPath,"/modules%s/%s/container.cfg",
                     node_ptr->intramodule_container,node_ptr->nodeName);
   }
   return (const char *)strdup(cfgPath);
}

void SeqNode_printCfgPath(FILE *file, SeqNodeDataPtr node_ptr)
{
   char *cfg_path = SeqNode_getCfgPath(node_ptr);
   SeqUtil_printOrWrite(file,"node.configpath=${SEQ_EXP_HOME}%s\n",cfg_path);
   free( cfg_path );
}

const char *SeqNode_getTaskPath(SeqNodeDataPtr node_ptr)
{
   char taskPath[SEQ_MAXFIELD] = {0};
   if( strcmp( node_ptr->taskPath,"") == 0){
      taskPath[0] = '\0';
   } else {
      sprintf(taskPath, "/modules%s.tsk", node_ptr->taskPath);
   }
   return (const char *)strdup(taskPath);
}

void SeqNode_printTaskPath(FILE *file, SeqNodeDataPtr node_ptr)
{
   const char * taskPath = SeqNode_getTaskPath(node_ptr);
   if( strlen(taskPath) == 0)
      SeqUtil_printOrWrite(file,"node.taskpath=\n");
   else
      SeqUtil_printOrWrite(file,"node.taskpath=${SEQ_EXP_HOME}%s\n",taskPath);

   free((char*)taskPath);
}

const char *SeqNode_getResourcePath(SeqNodeDataPtr node_ptr)
{
   char resPath[SEQ_MAXFIELD] = {'\0'};
   if( node_ptr->type == Task || node_ptr->type == NpassTask ){
      sprintf(resPath, "/resources%s.xml",node_ptr->name);
   } else {
      sprintf(resPath, "/resources%s/container.xml", node_ptr->name);
   }
   return (const char *)strdup(resPath);
}

void SeqNode_printResourcePath(FILE *file, SeqNodeDataPtr node_ptr)
{
   const char *resPath = SeqNode_getResourcePath(node_ptr);
   SeqUtil_printOrWrite(file, "node.resourcepath=${SEQ_EXP_HOME}%s\n",resPath);
   free((char*)resPath);
}

void SeqNode_printBatchResources(FILE *file, SeqNodeDataPtr node_ptr)
{
   SeqUtil_printOrWrite(file,"node.catchup=%d\n", node_ptr->catchup );
   SeqUtil_printOrWrite(file,"node.mpi=%d\n", node_ptr->mpi);
   SeqUtil_printOrWrite(file,"node.wallclock=%d\n", node_ptr->wallclock );
   SeqUtil_printOrWrite(file,"node.immediateMode=%d\n", node_ptr->immediateMode );
   SeqUtil_printOrWrite(file,"node.cpu=%s\n", node_ptr->cpu );
   SeqUtil_printOrWrite(file,"node.cpu_multiplier=%s\n", node_ptr->cpu_multiplier );
   SeqUtil_printOrWrite(file,"node.machine=%s\n", node_ptr->machine );
   SeqUtil_printOrWrite(file,"node.queue=%s\n", node_ptr->queue );
   SeqUtil_printOrWrite(file,"node.memory=%s\n", node_ptr->memory );
   SeqUtil_printOrWrite(file,"node.workerPath=%s\n", node_ptr->workerPath );
   SeqUtil_printOrWrite(file,"node.soumetArgs=%s\n", node_ptr->soumetArgs );
}
void SeqNode_printPathInfo(FILE *file, SeqNodeDataPtr node_ptr)
{
   SeqUtil_printOrWrite(file,"node.name=%s\n", node_ptr->name );
   SeqUtil_printOrWrite(file,"node.extension=%s\n",  node_ptr->extension);
   SeqUtil_printOrWrite(file,"node.leaf=%s\n", node_ptr->nodeName );
   SeqUtil_printOrWrite(file,"node.module=%s\n", node_ptr->module );
   SeqUtil_printOrWrite(file,"node.container=%s\n", node_ptr->container );
   SeqUtil_printOrWrite(file,"node.intramodule_container=%s\n", node_ptr->intramodule_container );
}


void SeqNode_printNode ( SeqNodeDataPtr node_ptr, unsigned int filters, const char * filename ) {

   FILE * tmpFile = NULL ;
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_printNode() called\n" );

   if( filename != NULL ) {
      removeFile_nfs(filename,node_ptr->expHome);
      if ((tmpFile = fopen(filename,"a+")) == NULL) {
         raiseError( "Unable to write to file:%s\n",filename );
      }
   }

   if( filters == 0 ) {
      filters |= NI_SHOW_ALL;
   }

   if( filters & NI_SHOW_ALL ) {
      SeqNode_printPathInfo(tmpFile, node_ptr);
   }

   if (filters & NI_SHOW_ROOT_ONLY ) {
      SeqUtil_printOrWrite(tmpFile,"node.rootnode=%s\n",node_ptr->name);
   }

   if (filters & NI_SHOW_NODE ) {
      (node_ptr->extension == NULL || strlen(node_ptr->extension) == 0) ? SeqUtil_printOrWrite(tmpFile,"node.fullnode=%s\n",node_ptr->name) : SeqUtil_printOrWrite(tmpFile,"node.fullnode=%s.%s\n",node_ptr->name,node_ptr->extension);
   }

   if (filters & ( NI_SHOW_ALL|NI_SHOW_TYPE ) ) {
        SeqUtil_printOrWrite(tmpFile,"node.type=%s\n", SeqNode_getTypeString( node_ptr->type ) );
   }

   if( filters & (NI_SHOW_ALL|NI_SHOW_RESOURCE ) ) {
      SeqNode_printBatchResources(tmpFile, node_ptr);
   }
   if( filters & ( NI_SHOW_ALL|NI_SHOW_CFGPATH )){
      SeqNode_printCfgPath(tmpFile,node_ptr);
   }

   if( (filters & (NI_SHOW_ALL|NI_SHOW_TASKPATH)) && (node_ptr->type == Task || node_ptr->type == NpassTask) ) {
      SeqNode_printTaskPath(tmpFile,node_ptr);
   }

   if (filters & (NI_SHOW_ALL|NI_SHOW_RESPATH)) {
      SeqNode_printResourcePath(tmpFile,node_ptr);
   }

   if(filters & NI_SHOW_ALL ) {

      SeqUtil_printOrWrite(tmpFile, "node.flow=${SEQ_EXP_HOME}/modules/%s/flow.xml\n", node_ptr->module );

      SeqNode_printNodeSpecifics(tmpFile,node_ptr);

      SeqNode_printDependencies(node_ptr, tmpFile, 0);

      SeqNode_printSubmits(tmpFile,node_ptr);

      SeqNode_printAborts(tmpFile,node_ptr);

      SeqNode_printLoops(tmpFile, node_ptr);

      SeqNode_printSiblings(tmpFile, node_ptr);
      if( node_ptr->type == ForEach ) {
         SeqNode_printForEachTargets(tmpFile,node_ptr);
      }
   }
   if (filters & NI_SHOW_DEP) SeqNode_printDependencies(node_ptr, tmpFile, 1);

   if (tmpFile != NULL) fclose(tmpFile);

   if (filters & NI_SHOW_VAR ) {
      /* function opens the file and writes in it */ 
       SeqNode_generateConfig( node_ptr,"continue", filename );
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_printNode() done\n" );
}

void SeqNode_printDependencies( SeqNodeDataPtr _nodeDataPtr, FILE * tmpFile, int isPrettyPrint ){

   SeqDependenciesPtr depsPtr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL;
   char *extraString=NULL;
   int count=1;

   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_printDependencies() started\n" );
   depsPtr = _nodeDataPtr->depends;
   if (isPrettyPrint) {
       SeqUtil_stringAppend( &extraString, "");
   } else {
       SeqUtil_stringAppend( &extraString, "node.depend.");
   }

   while( depsPtr != NULL ) {
      nameValuesPtr =  depsPtr->dependencyItem;
      if (isPrettyPrint) SeqUtil_printOrWrite(tmpFile,"Dependency #%d\n", count);
      /*
       * SeqNode_printDependency(FILE *fp, SeqDepDataPtr dep)
       * {
       *    if( dep->node_name != NULL )
       *       SeqUtil_printOrWrite(tmpFile,"%s%s=%s\n", extraString ,"NAME" ,dep->node_name );
       *    if( dep->exp != NULL )
       *       SeqUtil_printOrWrite(tmpFile,"%s%s=%s\n", extraString ,"EXP" ,dep->exp );
       *    ...
       * }
       *
       * This will replace the while(nameValuesPtr != NULL){ } loop.
       * The rest of the function will stay the same.
       * Same as before, selecting which fields to print based on whether it's
       * NULL instead of based on whether it's the empty string.
       */

      while (nameValuesPtr != NULL ) {
         if( strlen( nameValuesPtr->value ) > 0 ) 
            SeqUtil_printOrWrite(tmpFile,"%s%s=%s\n", extraString ,nameValuesPtr->name, nameValuesPtr->value );
         nameValuesPtr = nameValuesPtr->nextPtr;
      }
      ++count; 
      if (isPrettyPrint) SeqUtil_printOrWrite(tmpFile,"\n", count);

      depsPtr  = depsPtr->nextPtr;
   }
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_printDependencies() ended\n" );
   free(extraString);
} 

SeqNodeDataPtr SeqNode_createNode ( char* name ) {
   SeqNodeDataPtr nodeDataPtr = NULL;
   char * pathLeaf = SeqUtil_getPathLeaf(name);
   char * pathBase = SeqUtil_getPathBase(name);
   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_createNode() started\n" );
   if (nodeDataPtr = malloc( sizeof( SeqNodeData ) )){
       SeqNode_init ( nodeDataPtr );
   } else {
       raiseError("OutOfMemory exception in SeqNode_createNode()\n");
   }
   SeqNode_setName( nodeDataPtr, name );
   SeqNode_setNodeName ( nodeDataPtr, pathLeaf );
   SeqNode_setContainer ( nodeDataPtr, pathBase );

   SeqUtil_TRACE(TL_FULL_TRACE, "SeqNode.SeqNode_createNode() done\n" );
   free(pathLeaf);
   free(pathBase);
   return nodeDataPtr;
}

void SeqNode_freeNameValues ( SeqNameValuesPtr _nameValuesPtr ) {
   SeqNameValuesPtr nameValuesNextPtr = NULL;
   /* free a link-list of name-value pairs */
   while (_nameValuesPtr != NULL ) {
      /*SeqUtil_TRACE(TL_FULL_TRACE,"   SeqNode_freeNameValues %s=%s\n", _nameValuesPtr->name, _nameValuesPtr->value ); */

      /* load a copy of the next to be freed */
      nameValuesNextPtr = _nameValuesPtr->nextPtr;

      /* free the current node */
      free( _nameValuesPtr->name );
      free( _nameValuesPtr->value );
      free( _nameValuesPtr );

      /* go to the next to be freed */ 
      _nameValuesPtr = nameValuesNextPtr;
   }
}


void SeqNode_freeNode ( SeqNodeDataPtr seqNodeDataPtr ) {
   SeqDependenciesPtr depsPtr, depsNextPtr;

   if ( seqNodeDataPtr != NULL ) {
      free( seqNodeDataPtr->name ) ;
      free( seqNodeDataPtr->expHome );
      free( seqNodeDataPtr->nodeName );
      free( seqNodeDataPtr->container ) ;
      free( seqNodeDataPtr->intramodule_container ) ;
      free( seqNodeDataPtr->module ) ;
      free( seqNodeDataPtr->alias ) ;
      free( seqNodeDataPtr->args ) ;
      free( seqNodeDataPtr->soumetArgs ) ;
      free( seqNodeDataPtr->errormsg ) ;
      free( seqNodeDataPtr->cpu ) ;
      free( seqNodeDataPtr->npex ) ;
      free( seqNodeDataPtr->npey ) ;
      free( seqNodeDataPtr->omp ) ;
      free( seqNodeDataPtr->cpu_multiplier );
      free( seqNodeDataPtr->taskPath ) ;
      free( seqNodeDataPtr->suiteName ) ;
      free( seqNodeDataPtr->memory ) ;
      free( seqNodeDataPtr->machine ) ;
      free( seqNodeDataPtr->queue ) ;
      free( seqNodeDataPtr->datestamp) ;
      free( seqNodeDataPtr->workdir) ;
      free( seqNodeDataPtr->pathToModule) ;
      free( seqNodeDataPtr->submitOrigin) ;
      free( seqNodeDataPtr->extension) ;
      free( seqNodeDataPtr->workerPath) ;
      free( seqNodeDataPtr->shell);
  
      depsPtr = seqNodeDataPtr->depends;
      /* free a link-list of dependency items */
      while( depsPtr != NULL ) {

         /* make a copy of the next dependency item to be freed */
         depsNextPtr = depsPtr->nextPtr;

         SeqNode_freeNameValues( depsPtr->dependencyItem );
         free( depsPtr );
         depsPtr  = depsNextPtr;
      }
      SeqListNode_deleteWholeList( &(seqNodeDataPtr->submits) );
      SeqListNode_deleteWholeList( &(seqNodeDataPtr->abort_actions) );
      SeqListNode_deleteWholeList( &(seqNodeDataPtr->siblings) );
      SeqNameValues_deleteWholeList( &(seqNodeDataPtr->switchAnswers)) ;
      SeqNameValues_deleteWholeList( &(seqNodeDataPtr->data ));
      SeqNameValues_deleteWholeList( &(seqNodeDataPtr->loop_args ));
      /* SeqLoops_deleteWholeList( SeqLoopsPtr* list_head) */
      {
         SeqLoopsPtr current = seqNodeDataPtr->loops;
         for( current = seqNodeDataPtr->loops; current != NULL;){
            SeqLoopsPtr tmp_next = current->nextPtr;
            SeqNameValues_deleteWholeList(&(current->values));
            free(current->loop_name);
            free(current);
            current = tmp_next;
         }
         seqNodeDataPtr->loops = NULL;
      }
      SeqNode_freeForEachTarget(seqNodeDataPtr->forEachTarget);
      free( seqNodeDataPtr );
   }
}

/* 
SeqNode_generateConfig

Generates a config file that will be passed to ord_soumet so that the
exported variables are available for the tasks

Inputs:
  _nodeDataPtr - pointer to the node targetted by the execution
  flow - pointer to the value of the flow given to the binary ( -f option)
  filename - char * pointer to where the file must be generated, if null will be output to stdout

*/
void SeqNode_generateConfig (const SeqNodeDataPtr _nodeDataPtr, const char* flow, const char * filename ) {
   char *extName = NULL;
   int stringLength = 0;
   /* The following three variables are unused, maybe this indicates a mistake */
   /* int isRerun = 0; */
   /* char lockFile[SEQ_MAXFIELD]; */
   /* char pidbuf[100]; */
   char shortdate[11] = {'\0'};
   char *tmpdir = NULL, *loopArgs = NULL, *containerLoopArgs = NULL, *containerLoopExt = NULL, *tmpValue = NULL, *tmp2Value = NULL;
   FILE * tmpFile = NULL; 
   SeqNameValuesPtr loopArgsPtr=NULL , containerLoopArgsList = NULL;
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }
   
   if ( filename !=NULL ) {
      if ((tmpFile = fopen(filename,"a+")) == NULL) {
         raiseError( "Unable to write to file:%s\n",filename );
      }
   }

   SeqUtil_printOrWrite( tmpFile, "%s \n", getenv("SEQ_MAESTRO_SHORTCUT"));
   SeqUtil_printOrWrite( tmpFile, "export SEQ_MAESTRO_SHORTCUT=\"%s\" \n", getenv("SEQ_MAESTRO_SHORTCUT"));
   SeqUtil_printOrWrite( tmpFile, "export SEQ_EXP_HOME=%s\n",  _nodeDataPtr->expHome);
   SeqUtil_printOrWrite( tmpFile, "export SEQ_EXP_NAME=%s\n", _nodeDataPtr->suiteName); 
   SeqUtil_printOrWrite( tmpFile, "export SEQ_TRACE_LEVEL=%s\n", SeqUtil_getTraceLevelString());
   SeqUtil_printOrWrite( tmpFile, "export SEQ_MODULE=%s\n", _nodeDataPtr->module);
   SeqUtil_printOrWrite( tmpFile, "export SEQ_CONTAINER=%s\n", _nodeDataPtr->container); 
   SeqUtil_printOrWrite( tmpFile, "export SEQ_CURRENT_MODULE=%s\n", _nodeDataPtr->module);

   if( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
       SeqUtil_printOrWrite( tmpFile, "export SEQ_CURRENT_CONTAINER=%s\n", _nodeDataPtr->container); 
   } else {
       SeqUtil_printOrWrite( tmpFile, "export SEQ_CURRENT_CONTAINER=%s\n", _nodeDataPtr->name); 
   }
   if ( _nodeDataPtr-> npex != NULL ) {
   SeqUtil_printOrWrite( tmpFile, "export SEQ_NPEX=%s\n", _nodeDataPtr->npex);
   } 
   if ( _nodeDataPtr-> npey != NULL ) {
   SeqUtil_printOrWrite( tmpFile, "export SEQ_NPEY=%s\n", _nodeDataPtr->npey);
   }
   if ( _nodeDataPtr-> omp != NULL ) {
   SeqUtil_printOrWrite( tmpFile, "export SEQ_OMP=%s\n", _nodeDataPtr->omp);
   }
   SeqUtil_printOrWrite( tmpFile, "export SEQ_NODE=%s\n", _nodeDataPtr->name );
   SeqUtil_printOrWrite( tmpFile, "export SEQ_NAME=%s\n", _nodeDataPtr->nodeName );
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );
   if( strlen( loopArgs ) > 0 ) {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_LOOP_ARGS=\"-l %s\"\n", loopArgs );
   } else {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_LOOP_ARGS=\"\"\n" );
   }
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_LOOP_EXT=\"%s\"\n", _nodeDataPtr->extension );
   } else {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_LOOP_EXT=\"\"\n" );
   }

   /*container arguments, used in npass tasks mostly*/
   containerLoopArgsList = (SeqNameValuesPtr) SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
   if ( containerLoopArgsList != NULL) {
      containerLoopArgs = (char*) SeqLoops_getLoopArgs(containerLoopArgsList);
      containerLoopExt =  (char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList);
   }
   if ( containerLoopArgs != NULL ) {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_CONTAINER_LOOP_ARGS=\"-l %s\"\n", containerLoopArgs );
      free(containerLoopArgs);
   } else {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_CONTAINER_LOOP_ARGS=\"\"\n" );
   }
   if ( containerLoopExt != NULL ) {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_CONTAINER_LOOP_EXT=\"%s\"\n", containerLoopExt);
      free(containerLoopExt);
   } else {
      SeqUtil_printOrWrite( tmpFile, "export SEQ_CONTAINER_LOOP_EXT=\"\"\n" );
   } 

   loopArgsPtr = _nodeDataPtr->loop_args;
   /* Check for ^last arg */
   if (_nodeDataPtr->isLastArg){
      tmpValue=SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName); 
      /*remove the ^last, raise flag that node has a ^last*/
      stringLength=strlen(tmpValue)-5;
      if (tmp2Value=malloc(stringLength+1)) {
          memset(tmp2Value,'\0', stringLength+1);
      } else {
          raiseError("OutOfMemory exception in SeqNode_generateConfig()\n");
      }
      strncpy(tmp2Value, tmpValue, stringLength); 
      SeqUtil_stringAppend( &tmp2Value, "" );
      SeqUtil_TRACE(TL_FULL_TRACE,"SeqLoops_GenerateConfig Found ^last argument, replacing %s for %s for node %s \n", tmpValue, tmp2Value, _nodeDataPtr->nodeName); 
      SeqNameValues_setValue( &loopArgsPtr, _nodeDataPtr->nodeName, tmp2Value);
      SeqLoops_printLoopArgs(_nodeDataPtr->loop_args,"test"); 
   }

   /* Loop args exported as env variables */
   while (loopArgsPtr != NULL) {
      SeqUtil_printOrWrite( tmpFile, "export %s=%s \n", loopArgsPtr->name, loopArgsPtr->value );
      loopArgsPtr=loopArgsPtr->nextPtr;
   }
   
   if (flow != NULL ) {
       SeqUtil_printOrWrite( tmpFile, "export SEQ_XFER=%s\n", flow );
   } else {
       SeqUtil_printOrWrite( tmpFile, "export SEQ_XFER=continue\n");
   }
       
   SeqUtil_printOrWrite( tmpFile, "export SEQ_WORKER_PATH=%s\n", _nodeDataPtr->workerPath );
   if (filename != NULL) {
       SeqUtil_printOrWrite( tmpFile, "export SEQ_TMP_CFG=%s\n", filename);
   }
   SeqUtil_printOrWrite( tmpFile, "export SEQ_DATE=%s\n", _nodeDataPtr->datestamp); 
   memset(shortdate,'\0', strlen(shortdate)+1);
   if (strlen(_nodeDataPtr->datestamp) > 10) {
      strncpy(shortdate,_nodeDataPtr->datestamp,10);
      shortdate[10]='\0';
   } else {
      strcpy(shortdate,_nodeDataPtr->datestamp);
   }
   SeqUtil_printOrWrite( tmpFile, "export SEQ_SHORT_DATE=%s\n", shortdate); 

   /* check for the presence of a "rerun" file to determine rerun status */
   /* TODO find a way for nodeinfo to figure out whether this access is going through the server or not..., check function pointers else it will return a memfault  
   memset(lockFile,'\0',sizeof lockFile);
   sprintf(lockFile,"%s/%s/%s.abort.rerun",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName);
      if ( _access(lockFile, R_OK) == 0 ) {
	      isRerun = 1;
      }
   SeqUtil_printOrWrite( filename, "export SEQ_RERUN=%d\n", isRerun );
   */

   if (tmpFile != NULL) fclose(tmpFile);
   free(tmpdir);
   free(tmpValue);
   free(tmp2Value);
   free(loopArgs);
   free(loopArgsPtr);
   free(extName);
   SeqNameValues_deleteWholeList( &containerLoopArgsList);
}

/* return node extension with ^last extension stripped */
char* SeqNode_extension( const SeqNodeDataPtr _nodeDataPtr ) {
  char  *extName=NULL;

  SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
  if( strlen( SeqUtil_striplast( _nodeDataPtr->extension ) ) > 0 ) {
    SeqUtil_stringAppend( &extName, "." );
    SeqUtil_stringAppend( &extName, SeqUtil_striplast( _nodeDataPtr->extension ) );
  }
  return extName;
}
