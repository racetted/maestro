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
   char tmpstrtok[10];
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

      SeqUtil_TRACE(TL_MINIMAL, "SeqNode_setCpu() cpu=%s, x-separator count=%d\n",cpu,x_count);

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
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_allocateLoopsEntry()\n" );
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
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_allocateLoopsEntry() done\n" );
   return loopsPtr;
}

/* returns ptr to newly allocated memory structure 
   to store a new name value link list */
SeqDependenciesPtr SeqNode_allocateDepsEntry ( SeqNodeDataPtr node_ptr ) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_allocateDepsEntry()\n" );
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
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_allocateDepsEntry() done\n" );
   return deps_ptr;
}

/* returns ptr to newly allocated memory structure
   to store a name value pair */
SeqNameValuesPtr SeqNode_allocateDepsNameValue ( SeqDependenciesPtr deps_ptr ) {
   SeqNameValuesPtr nameValuesPtr = NULL;

   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_allocateDepsNameValue()\n" );
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
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_allocateDepsNameValue() done\n" );
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
void SeqNode_addNodeDependency ( SeqNodeDataPtr node_ptr, SeqDependsType type, char* dep_node_name, char* dep_node_path,
                         char* dep_exp, char* dep_status, char* dep_index, char* local_index, char* dep_hour, char* dep_Prot, char* dep_ValidHour, char* dep_ValidDOW) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL;
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_addNodeDependency() dep_node=%s, dep_node_path=%s, dep_exp=%s, dep_status=%s, dep_index=%s, local_index=%s, dep_Prot=%s,dep_ValidHour=%s, dep_ValidDOW=%s \n",
      dep_node_name, dep_node_path, dep_exp, dep_status, dep_index, local_index, dep_Prot, dep_ValidHour, dep_ValidDOW);
   deps_ptr = SeqNode_allocateDepsEntry( node_ptr );
   deps_ptr->type = type;
   nameValuesPtr = deps_ptr->dependencyItem;
   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "NAME", dep_node_name );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "INDEX", dep_index );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "EXP", dep_exp );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "STATUS", dep_status );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "LOCAL_INDEX", local_index );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "HOUR", dep_hour ); 

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "PROT", dep_Prot );
 
   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "VALID_HOUR", dep_ValidHour );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "VALID_DOW", dep_ValidDOW );

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
   char *tmpStart = start, *tmpStep = step, *tmpSet = set, *tmpEnd = end, *tmpExpression = expression;
   char *defFile = NULL, *value = NULL;
   
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_addNumLoop() input loop_name=%s, start=%s, step=%s, set=%s, end=%s, expression=%s, \n",loop_name, start, step, set, end, expression );

   defFile = malloc ( strlen ( node_ptr->expHome ) + strlen("/resources/resources.def") + 1 );
   sprintf( defFile, "%s/resources/resources.def", node_ptr->expHome );
   
   if (strcmp(tmpExpression, "") == 0) {
      if (strstr(start, "${") != NULL) {
         if ( (value = SeqUtil_keysub( start, defFile, NULL)) != NULL ){
            strcpy(tmpStart,value);
         }
      }
      if (strstr(step, "${") != NULL) {
         if ( (value = SeqUtil_keysub( step, defFile, NULL)) != NULL ){
            strcpy(tmpStep,value);
         }
      }
      if (strstr(set, "${") != NULL) {
         if ( (value = SeqUtil_keysub( set, defFile, NULL)) != NULL ){
            strcpy(tmpSet,value);
         }
      }
      if (strstr(end, "${") != NULL) {
         if ( (value = SeqUtil_keysub( end, defFile, NULL)) != NULL ){
            strcpy(tmpEnd,value);
         }
      }
   } else {
      if (strstr(expression, "${") != NULL) {
         if ( (value = SeqUtil_keysub( expression, defFile, NULL)) != NULL ){
            strcpy(tmpExpression,value);
         }
      }
   }
   
   if (strcmp(tmpExpression, "") == 0) {
      SeqUtil_TRACE(TL_MINIMAL, "SeqNode_addNumLoop() resulting loop_name=%s, start=%s, step=%s, set=%s, end=%s, \n",loop_name, tmpStart, tmpStep, tmpSet, tmpEnd );
   } else {
      SeqUtil_TRACE(TL_MINIMAL, "SeqNode_addNumLoop() resulting loop_name=%s, expression:%s, \n",loop_name, expression );
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

void SeqNode_addSwitch ( SeqNodeDataPtr _nodeDataPtr, char* switchName, char* switchType, char* returnValue) {
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode_addSwitch() switchName=%s switchType=%s returnValue=%s\n", switchName, switchType, returnValue);
   loopsPtr = SeqNode_allocateLoopsEntry( _nodeDataPtr );
   loopsPtr->type = SwitchType;
   loopsPtr->loop_name = strdup( switchName );
   SeqNameValues_insertItem( &loopsPtr->values, "TYPE", switchType);
   SeqNameValues_insertItem( &loopsPtr->values, "VALUE", returnValue );
}



void SeqNode_addSpecificData ( SeqNodeDataPtr node_ptr, char* name, char* value ) {
   char* tmp = NULL;
   int count = 0;
   /* to allow easy comparison, I convert everthing to upper case */
   tmp = strdup( name );
   while( name[count] != '\0' ) {
      tmp[count] = toupper(name[count]);
      count++;
   }
   tmp[count] = '\0';
   /* SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_addSpecificData() called name:%s value:%s\n", tmp, value ); */
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
   SeqNode_setCpu( nodePtr, "1x1x1" );
   SeqNode_setCpuMultiplier( nodePtr, "1" );
   SeqNode_setQueue( nodePtr, "null" );
   SeqNode_setMachine( nodePtr, "" );
   SeqNode_setMemory( nodePtr, "300M" );
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

void SeqNode_printNode ( SeqNodeDataPtr node_ptr, const char* filters, const char * filename ) {

   char *tmpstrtok = NULL, *tmpFilters ;
   int showAll = 0, showCfgPath = 0, showTaskPath = 0, showRessource = 0; 
   int showType = 0, showNode = 0, showRootOnly = 0, showResPath = 0, showVar=0, showDependencies=0;
   SeqNameValuesPtr nameValuesPtr = NULL ;
   SeqDependenciesPtr depsPtr = NULL;
   LISTNODEPTR submitsPtr = NULL, siblingsPtr = NULL, abortsPtr = NULL;
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_printNode() called\n" );

   if( filename != NULL ) {
      removeFile_nfs(filename);
   }
   if( filters == NULL ) {
      showAll = 1;
   } else {
      tmpFilters = strdup( filters );
      tmpstrtok = (char*) strtok( tmpFilters, "," );
      while ( tmpstrtok != NULL ) {
         if ( strcmp( tmpstrtok, "all" ) == 0 ) showAll = 1;
         if ( strcmp( tmpstrtok, "cfg" ) == 0 ) showCfgPath = 1;
         if ( strcmp( tmpstrtok, "var" ) == 0 ) showVar = 1;
         if ( strcmp( tmpstrtok, "task" ) == 0 ) showTaskPath = 1;
         if ( strcmp( tmpstrtok, "res" ) == 0 ) showRessource = 1;
         if ( strcmp( tmpstrtok, "dep" ) == 0 ) showDependencies = 1;
         if ( strcmp( tmpstrtok, "res_path" ) == 0 ) showResPath = 1;
         if ( strcmp( tmpstrtok, "type" ) == 0 ) showType = 1;
         if ( strcmp( tmpstrtok, "node" ) == 0 ) showNode= 1;
         if ( strcmp( tmpstrtok, "root" ) == 0 ) showRootOnly = 1;

         tmpstrtok = (char*) strtok(NULL,",");
      }

      if  (( showAll || showType || showCfgPath || showRessource || showTaskPath || showNode || showRootOnly || showResPath || showVar || showDependencies ) == 0) {
         raiseError("Filters %s unrecognized\n", filters);
      }

   }

   /*printf("************ Seq Node Information \n"); */
   if( showAll ) {
      SeqUtil_printOrWrite(filename,"node.name=%s\n", node_ptr->name );
      SeqUtil_printOrWrite(filename,"node.extension=%s\n",  node_ptr->extension);
      SeqUtil_printOrWrite(filename,"node.leaf=%s\n", node_ptr->nodeName );
      SeqUtil_printOrWrite(filename,"node.module=%s\n", node_ptr->module );
      SeqUtil_printOrWrite(filename,"node.container=%s\n", node_ptr->container );
      SeqUtil_printOrWrite(filename,"node.intramodule_container=%s\n", node_ptr->intramodule_container );
      /*
      SeqUtil_printOrWrite(filename,"alias=%s\n", node_ptr->alias );
      SeqUtil_printOrWrite(filename,"args=%s\n", node_ptr->args );
      */
   }
   if (showRootOnly) {
      SeqUtil_printOrWrite(filename,"node.rootnode=%s\n",node_ptr->name);
   }

   if (showNode) {
      (node_ptr->extension == NULL || strlen(node_ptr->extension) == 0) ? SeqUtil_printOrWrite(filename,"node.fullnode=%s\n",node_ptr->name) : SeqUtil_printOrWrite(filename,"node.fullnode=%s.%s\n",node_ptr->name,node_ptr->extension);
   }

   if ( showAll || showType ) {
        SeqUtil_printOrWrite(filename,"node.type=%s\n", SeqNode_getTypeString( node_ptr->type ) );
   } 

   if( showAll || showRessource ) {
      SeqUtil_printOrWrite(filename,"node.catchup=%d\n", node_ptr->catchup );
      SeqUtil_printOrWrite(filename,"node.mpi=%d\n", node_ptr->mpi);
      SeqUtil_printOrWrite(filename,"node.wallclock=%d\n", node_ptr->wallclock );
      SeqUtil_printOrWrite(filename,"node.cpu=%s\n", node_ptr->cpu );
      SeqUtil_printOrWrite(filename,"node.cpu_multiplier=%s\n", node_ptr->cpu_multiplier );
      SeqUtil_printOrWrite(filename,"node.machine=%s\n", node_ptr->machine );
      SeqUtil_printOrWrite(filename,"node.queue=%s\n", node_ptr->queue );
      SeqUtil_printOrWrite(filename,"node.memory=%s\n", node_ptr->memory );
      SeqUtil_printOrWrite(filename,"node.workerPath=%s\n", node_ptr->workerPath );
      SeqUtil_printOrWrite(filename,"node.soumetArgs=%s\n", node_ptr->soumetArgs );
   }
   if( showAll || showCfgPath ) {
      if( node_ptr->type == Task || node_ptr->type == NpassTask ) {
         SeqUtil_printOrWrite(filename,"node.configpath=${SEQ_EXP_HOME}/modules%s.cfg\n", node_ptr->taskPath );
      } else {
         SeqUtil_printOrWrite(filename,"node.configpath=${SEQ_EXP_HOME}/modules%s/%s/container.cfg\n", node_ptr->intramodule_container, node_ptr->nodeName );
      }
   }

   if( (showAll || showTaskPath) && (node_ptr->type == Task || node_ptr->type == NpassTask) ) {
      if ( strcmp( node_ptr->taskPath, "" ) == 0 )
         SeqUtil_printOrWrite(filename,"node.taskpath=\n");
      else
         SeqUtil_printOrWrite(filename,"node.taskpath=${SEQ_EXP_HOME}/modules%s.tsk\n", node_ptr->taskPath );
   }
   if ( showAll || showResPath ) {
      if( node_ptr->type == Task || node_ptr->type == NpassTask ) {
         SeqUtil_printOrWrite(filename,"node.resourcepath=${SEQ_EXP_HOME}/resources%s.xml\n", node_ptr->name);
      } else {
         SeqUtil_printOrWrite(filename,"node.resourcepath=${SEQ_EXP_HOME}/resources%s/container.xml\n", node_ptr->name );
      }
   }

   if( showAll ) {

      SeqUtil_printOrWrite(filename, "node.flow=${SEQ_EXP_HOME}/modules/%s/flow.xml\n", node_ptr->module );
      /*SeqUtil_printOrWrite(filename,"************ Node Specific Data \n"); */
      nameValuesPtr = node_ptr->data;
      while (nameValuesPtr != NULL ) {
         SeqUtil_printOrWrite(filename,"node.specific.%s=%s\n", nameValuesPtr->name, nameValuesPtr->value );
         nameValuesPtr = nameValuesPtr->nextPtr;
      }

      /* Dependencies */
      SeqNode_printDependencies(node_ptr, filename, 0);
   
      /*SeqUtil_printOrWrite(filename,"************ Node Submits \n"); */
      submitsPtr = node_ptr->submits;
      while (submitsPtr != NULL) {
         SeqUtil_printOrWrite(filename,"node.submit=%s\n", submitsPtr->data);
         submitsPtr = submitsPtr->nextPtr;
      }
   
      /*SeqUtil_printOrWrite(filename,"************ Node Abort Actions \n"); */
      abortsPtr = node_ptr->abort_actions;
      while (abortsPtr != NULL) {
         SeqUtil_printOrWrite(filename,"node.abortaction=%s\n", abortsPtr->data);
         abortsPtr = abortsPtr->nextPtr;
      }
      /*SeqUtil_printOrWrite(filename,"************ Containing Loops \n"); */
      loopsPtr = node_ptr->loops;
      while (loopsPtr != NULL) {
         /*SeqUtil_printOrWrite(filename,"************ Loop \n"); */
         SeqUtil_printOrWrite(filename,"node.loop_parent.name=%s\n", loopsPtr->loop_name);  
         nameValuesPtr = loopsPtr->values;
         while (nameValuesPtr != NULL ) {
            SeqUtil_printOrWrite(filename,"node.loop_parent.%s=%s\n", nameValuesPtr->name, nameValuesPtr->value );
            nameValuesPtr = nameValuesPtr->nextPtr;
         }
         loopsPtr = loopsPtr->nextPtr;
      }
      /*SeqUtil_printOrWrite(filename,"************ Node Siblings \n"); */
      siblingsPtr = node_ptr->siblings;
      while (siblingsPtr != NULL) {
         SeqUtil_printOrWrite(filename,"node.sibling=%s\n", siblingsPtr->data);
         siblingsPtr = siblingsPtr->nextPtr;
      }

      if( node_ptr->type == ForEach ) {
         if (node_ptr->forEachTarget->node != NULL)  SeqUtil_printOrWrite(filename,"node.ForEachTarget.node=%s\n", node_ptr->forEachTarget->node );
         if (node_ptr->forEachTarget->index != NULL)  SeqUtil_printOrWrite(filename,"node.ForEachTarget.index=%s\n", node_ptr->forEachTarget->index );
         if (node_ptr->forEachTarget->exp != NULL)  SeqUtil_printOrWrite(filename,"node.ForEachTarget.exp=%s\n", node_ptr->forEachTarget->exp );
         if (node_ptr->forEachTarget->hour != NULL)  SeqUtil_printOrWrite(filename,"node.ForEachTarget.hour=%s\n", node_ptr->forEachTarget->hour );
      }
   }
   if (showDependencies) SeqNode_printDependencies(node_ptr, filename, 1);

   if (showVar) {
       SeqNode_generateConfig( node_ptr,"continue", filename );
   }

   free( tmpFilters );
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_printNode() done\n" );
}

void SeqNode_printDependencies( SeqNodeDataPtr _nodeDataPtr, const char * filename, int isPrettyPrint ){

   SeqDependenciesPtr depsPtr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL;
   char *extraString=NULL;
   int count=1;

   SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_printDependencies() started\n" );
   depsPtr = _nodeDataPtr->depends;
   if (isPrettyPrint) {
       SeqUtil_stringAppend( &extraString, "");
   } else {
       SeqUtil_stringAppend( &extraString, "node.depend.");
   }

   while( depsPtr != NULL ) {
      nameValuesPtr =  depsPtr->dependencyItem;
      if (isPrettyPrint) SeqUtil_printOrWrite(filename,"Dependency #%d\n", count);
      while (nameValuesPtr != NULL ) {
         if( strlen( nameValuesPtr->value ) > 0 ) 
            SeqUtil_printOrWrite(filename,"%s%s=%s\n", extraString ,nameValuesPtr->name, nameValuesPtr->value );
         nameValuesPtr = nameValuesPtr->nextPtr;
      }
      ++count; 
      if (isPrettyPrint) SeqUtil_printOrWrite(filename,"\n", count);

      depsPtr  = depsPtr->nextPtr;
   }
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_printDependencies() ended\n" );
   free(extraString);
} 

SeqNodeDataPtr SeqNode_createNode ( char* name ) {
   SeqNodeDataPtr nodeDataPtr = NULL;
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_createNode() started\n" );
   if (nodeDataPtr = malloc( sizeof( SeqNodeData ) )){
       SeqNode_init ( nodeDataPtr );
   } else {
       raiseError("OutOfMemory exception in SeqNode_createNode()\n");
   }
   SeqNode_setName( nodeDataPtr, name );
   SeqNode_setNodeName ( nodeDataPtr, (char*) SeqUtil_getPathLeaf(name));
   SeqNode_setContainer ( nodeDataPtr, (char*) SeqUtil_getPathBase(name));
   SeqUtil_TRACE(TL_MINIMAL, "SeqNode.SeqNode_createNode() done\n" );
   return nodeDataPtr;
}

void SeqNode_freeNameValues ( SeqNameValuesPtr _nameValuesPtr ) {
   SeqNameValuesPtr nameValuesNextPtr = NULL;
   /* free a link-list of name-value pairs */
   while (_nameValuesPtr != NULL ) {
      /*SeqUtil_TRACE(TL_MINIMAL,"   SeqNode_freeNameValues %s=%s\n", _nameValuesPtr->name, _nameValuesPtr->value ); */

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
   int stringLength = 0, isRerun = 0; 
   char lockFile[SEQ_MAXFIELD];
   char pidbuf[100];
   char shortdate[11];
   char *tmpdir = NULL, *loopArgs = NULL, *containerLoopArgs = NULL, *containerLoopExt = NULL, *tmpValue = NULL, *tmp2Value = NULL;
   SeqNameValuesPtr loopArgsPtr=NULL , containerLoopArgsList = NULL;
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }
   SeqUtil_printOrWrite( filename, "%s \n", getenv("SEQ_MAESTRO_SHORTCUT"));
   SeqUtil_printOrWrite( filename, "export SEQ_MAESTRO_SHORTCUT=\"%s\" \n", getenv("SEQ_MAESTRO_SHORTCUT"));
   SeqUtil_printOrWrite( filename, "export SEQ_EXP_HOME=%s\n",  getenv("SEQ_EXP_HOME"));
   SeqUtil_printOrWrite( filename, "export SEQ_EXP_NAME=%s\n", _nodeDataPtr->suiteName); 
   SeqUtil_printOrWrite( filename, "export SEQ_TRACE_LEVEL=%s\n", getenv("SEQ_TRACE_LEVEL"));
   SeqUtil_printOrWrite( filename, "export SEQ_MODULE=%s\n", _nodeDataPtr->module);
   SeqUtil_printOrWrite( filename, "export SEQ_CONTAINER=%s\n", _nodeDataPtr->container); 
   SeqUtil_printOrWrite( filename, "export SEQ_CURRENT_MODULE=%s\n", _nodeDataPtr->module);
   if( _nodeDataPtr->type == Task || _nodeDataPtr->type == NpassTask ) {
       SeqUtil_printOrWrite( filename, "export SEQ_CURRENT_CONTAINER=%s\n", _nodeDataPtr->container); 
   } else {
       SeqUtil_printOrWrite( filename, "export SEQ_CURRENT_CONTAINER=%s\n", _nodeDataPtr->name); 
   }
   if ( _nodeDataPtr-> npex != NULL ) {
   SeqUtil_printOrWrite( filename, "export SEQ_NPEX=%s\n", _nodeDataPtr->npex);
   } 
   if ( _nodeDataPtr-> npey != NULL ) {
   SeqUtil_printOrWrite( filename, "export SEQ_NPEY=%s\n", _nodeDataPtr->npey);
   }
   if ( _nodeDataPtr-> omp != NULL ) {
   SeqUtil_printOrWrite( filename, "export SEQ_OMP=%s\n", _nodeDataPtr->omp);
   }
   SeqUtil_printOrWrite( filename, "export SEQ_NODE=%s\n", _nodeDataPtr->name );
   SeqUtil_printOrWrite( filename, "export SEQ_NAME=%s\n", _nodeDataPtr->nodeName );
   loopArgs = (char*) SeqLoops_getLoopArgs( _nodeDataPtr->loop_args );
   if( strlen( loopArgs ) > 0 ) {
      SeqUtil_printOrWrite( filename, "export SEQ_LOOP_ARGS=\"-l %s\"\n", loopArgs );
   } else {
      SeqUtil_printOrWrite( filename, "export SEQ_LOOP_ARGS=\"\"\n" );
   }
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_printOrWrite( filename, "export SEQ_LOOP_EXT=\"%s\"\n", _nodeDataPtr->extension );
   } else {
      SeqUtil_printOrWrite( filename, "export SEQ_LOOP_EXT=\"\"\n" );
   }

   /*container arguments, used in npass tasks mostly*/
   containerLoopArgsList = (SeqNameValuesPtr) SeqLoops_getContainerArgs(_nodeDataPtr, _nodeDataPtr->loop_args);
   if ( containerLoopArgsList != NULL) {
      containerLoopArgs = (char*) SeqLoops_getLoopArgs(containerLoopArgsList);
      containerLoopExt =  (char*) SeqLoops_getExtFromLoopArgs(containerLoopArgsList);
   }
   if ( containerLoopArgs != NULL ) {
      SeqUtil_printOrWrite( filename, "export SEQ_CONTAINER_LOOP_ARGS=\"-l %s\"\n", containerLoopArgs );
      free(containerLoopArgs);
   } else {
      SeqUtil_printOrWrite( filename, "export SEQ_CONTAINER_LOOP_ARGS=\"\"\n" );
   }
   if ( containerLoopExt != NULL ) {
      SeqUtil_printOrWrite( filename, "export SEQ_CONTAINER_LOOP_EXT=\"%s\"\n", containerLoopExt);
      free(containerLoopExt);
   } else {
      SeqUtil_printOrWrite( filename, "export SEQ_CONTAINER_LOOP_EXT=\"\"\n" );
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
      SeqUtil_TRACE(TL_MINIMAL,"SeqLoops_GenerateConfig Found ^last argument, replacing %s for %s for node %s \n", tmpValue, tmp2Value, _nodeDataPtr->nodeName); 
      SeqNameValues_setValue( &loopArgsPtr, _nodeDataPtr->nodeName, tmp2Value);
      SeqLoops_printLoopArgs(_nodeDataPtr->loop_args,"test"); 
   }

   /* Loop args exported as env variables */
   while (loopArgsPtr != NULL) {
      SeqUtil_printOrWrite( filename, "export %s=%s \n", loopArgsPtr->name, loopArgsPtr->value );
      loopArgsPtr=loopArgsPtr->nextPtr;
   }
   
   if (flow != NULL ) {
       SeqUtil_printOrWrite( filename, "export SEQ_XFER=%s\n", flow );
   } else {
       SeqUtil_printOrWrite( filename, "export SEQ_XFER=continue\n");
   }
       
   SeqUtil_printOrWrite( filename, "export SEQ_WORKER_PATH=%s\n", _nodeDataPtr->workerPath );
   if (filename != NULL) {
       SeqUtil_printOrWrite( filename, "export SEQ_TMP_CFG=%s\n", filename);
   }
   SeqUtil_printOrWrite( filename, "export SEQ_DATE=%s\n", _nodeDataPtr->datestamp); 
   memset(shortdate,'\0', strlen(shortdate)+1);
   if (strlen(_nodeDataPtr->datestamp) > 10) {
      strncpy(shortdate,_nodeDataPtr->datestamp,10);
      shortdate[10]='\0';
   } else {
      strcpy(shortdate,_nodeDataPtr->datestamp);
   }
   SeqUtil_printOrWrite( filename, "export SEQ_SHORT_DATE=%s\n", shortdate); 

   /* check for the presence of a "rerun" file to determine rerun status */
   /* TODO find a way for nodeinfo to figure out whether this access is going through the server or not..., check function pointers else it will return a memfault  
   memset(lockFile,'\0',sizeof lockFile);
   sprintf(lockFile,"%s/%s/%s.abort.rerun",_nodeDataPtr->workdir, _nodeDataPtr->datestamp, extName);
      if ( _access(lockFile, R_OK) == 0 ) {
	      isRerun = 1;
      }
   SeqUtil_printOrWrite( filename, "export SEQ_RERUN=%d\n", isRerun );
   */

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
