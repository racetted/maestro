/* Part of the Maestro sequencer software package.
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


#include "SeqListNode.h"
#include "SeqNameValues.h"
#include <stdio.h>
#ifndef _SEQ_NODE
#define _SEQ_NODE

typedef enum _SeqDependsType {
   NodeDependancy,
   DateDependancy
} SeqDependsType;

typedef enum _SeqDependsScope {
   IntraSuite,
   IntraUser,
   InterUser,
} SeqDependsScope;

typedef enum _SeqNodeType {
   Family,
   Task,
   NpassTask,
   Module,
   Loop,
   Switch,
   ForEach,
} SeqNodeType;

typedef struct _SeqDependencies {
   SeqNameValuesPtr dependencyItem;
   /* the type is a discriminant to
   process the generic name-value structure */
   SeqDependsType type;
   struct _SeqDependencies *nextPtr;
} SeqDependencies;

typedef SeqDependencies *SeqDependenciesPtr;

typedef struct _SeqForEachTarget {
   char* index;
   char* exp;
   char* node;
   char* hour;
} SeqForEachTarget;

typedef SeqForEachTarget *SeqForEachTargetPtr;


/* not used now*/
typedef struct _SeqDependencyLocator {
   char* suite_path;
   char* node_taskPath;
} SeqDependencyLocator;
typedef SeqDependencyLocator *SeqDependencyLocatorPtr;

typedef enum _SeqLoopType {
   Numerical,
   Date,
   SwitchType 
} SeqLoopType;

typedef struct _SeqLoops {
   SeqNameValuesPtr values;
   /* the type is a discriminant to
   process the generic name-value structure */
   SeqLoopType type;
   char* loop_name;
   struct _SeqLoops *nextPtr;
} SeqLoops;

typedef SeqLoops *SeqLoopsPtr;

typedef struct _SeqNodeData {
   SeqNodeType type;
   char* name;
   char* nodeName;
   char* suiteName;
   char* container;
   char* intramodule_container;
   char* module;
   char* pathToModule;
   int   silent;
   int   catchup;
   int   mpi;
   int   wallclock;
   int   isLastArg;
   char* queue;
   char* machine;
   char* memory;
   char* cpu;
   char* cpu_multiplier;
   char* npex;
   char* npey;
   char* omp;
   /*extra soumet arguments provided via command-line, resource file or interface*/
   char* soumetArgs;
   char* workerPath;
   char* alias;
   char* args;
   char* datestamp;
   char* submitOrigin;
   char* workdir;
   char* expHome;
   char* shell;

   /* Switch container */ 
   SeqNameValuesPtr switchAnswers; 
 
   /* ForEach container*/
   SeqForEachTargetPtr forEachTarget; 

   /* list of dependencies */
   SeqDependenciesPtr depends;

   /* next nodes to submit */
   LISTNODEPTR submits;

   /* abort_actions */
   LISTNODEPTR abort_actions;

   /* siblings to the node*/
   LISTNODEPTR siblings;

   /* parent loops */
   SeqLoopsPtr loops;

   /* name-value pairs to store node specific attribute data */
   SeqNameValuesPtr data;

   /* name-value pairs to store node specific loop arguments */
   SeqNameValuesPtr loop_args;

   /* relative node path within suite task dir structure */
   char* taskPath;

   /* loop_extensions */
   char* extension;

   int error;
   char* errormsg;
} SeqNodeData;

typedef SeqNodeData *SeqNodeDataPtr;

SeqNodeDataPtr SeqNode_createNode ( char* name );
void SeqNode_printDependencies( SeqNodeDataPtr _nodeDataPtr, FILE * filename, int isPrettyPrint );

extern void SeqNode_generateConfig (const SeqNodeDataPtr _nodeDataPtr, const char* flow, const char * filename );
extern char * SeqNode_extension (const SeqNodeDataPtr _nodeDataPtr);


#endif
