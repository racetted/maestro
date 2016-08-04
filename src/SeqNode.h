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


#ifndef _SEQ_NODE
#define _SEQ_NODE

#include "SeqListNode.h"
#include "SeqNameValues.h"
#include "SeqDepends.h"
#include <stdio.h>

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
   int   immediateMode; 
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
   SeqDepNodePtr dependencies;

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
void SeqNode_setCpu ( SeqNodeDataPtr node_ptr, const char* cpu );
void SeqNode_setCpuMultiplier ( SeqNodeDataPtr node_ptr, const char* cpu_multiplier );
void SeqNode_setMachine ( SeqNodeDataPtr node_ptr, const char* machine );
void SeqNode_setQueue ( SeqNodeDataPtr node_ptr, const char* queue );
void SeqNode_setMemory ( SeqNodeDataPtr node_ptr, const char* memory );
void SeqNode_setShell ( SeqNodeDataPtr node_ptr, const char* shell );
void SeqNode_addNumLoop ( SeqNodeDataPtr node_ptr, char* loop_name, char* start, char* step, char* set, char* end, char* expression );
void SeqNode_addSubmit ( SeqNodeDataPtr node_ptr, char* data );
void SeqNode_setWorkerPath ( SeqNodeDataPtr node_ptr, const char* workerPath );
void SeqNode_addAbortAction ( SeqNodeDataPtr node_ptr, char* data );
char* SeqNode_getTypeString( SeqNodeType _node_type );
void SeqNode_addSpecificData ( SeqNodeDataPtr node_ptr, const char* name, const char* value );
void SeqNode_addSibling ( SeqNodeDataPtr node_ptr, char* data );
void SeqNode_setName ( SeqNodeDataPtr node_ptr, const char* name );
void SeqNode_setSoumetArgs ( SeqNodeDataPtr node_ptr, char* soumetArgs );
void SeqNode_setForEachTarget(SeqNodeDataPtr nodePtr, const char * t_node,  const char * t_index,  const char * t_exp,  const char * t_hour);
void SeqNode_setArgs ( SeqNodeDataPtr node_ptr, const char* args );
void SeqNode_freeNode ( SeqNodeDataPtr seqNodeDataPtr );
void SeqNode_setDatestamp( SeqNodeDataPtr node_ptr, const char* datestamp);
void SeqNode_setSeqExpHome ( SeqNodeDataPtr node_ptr, const char* expHome );
void SeqNode_setWorkdir( SeqNodeDataPtr node_ptr, const char* workdir);
void SeqNode_setLoopArgs ( SeqNodeDataPtr node_ptr, SeqNameValuesPtr _loop_args );
void SeqNode_setIntramoduleContainer ( SeqNodeDataPtr node_ptr, const char* intramodule_container );
void SeqNode_setPathToModule ( SeqNodeDataPtr node_ptr, const char* pathToModule );
void SeqNode_setSuiteName ( SeqNodeDataPtr node_ptr, const char* suiteName );
void SeqNode_setInternalPath ( SeqNodeDataPtr node_ptr, const char* path );
void SeqNode_setModule ( SeqNodeDataPtr node_ptr, const char* module );
void SeqNode_addSwitch ( SeqNodeDataPtr _nodeDataPtr, const char* switchName, const char* switchType, const char* returnValue);

const char *SeqNode_getCfgPath( SeqNodeDataPtr node_ptr);
const char *SeqNode_getTaskPath(SeqNodeDataPtr node_ptr);
const char *SeqNode_getResourcePath(SeqNodeDataPtr node_ptr);

void SeqNode_printForEachTargets(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printNodeSpecifics(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printSubmits(FILE *file,SeqNodeDataPtr node_ptr );
void SeqNode_printAborts( FILE * file, SeqNodeDataPtr node_ptr);
void SeqNode_printLoops( FILE* file , SeqNodeDataPtr node_ptr);
void SeqNode_printSiblings(FILE * file, SeqNodeDataPtr node_ptr );
void SeqNode_printCfgPath(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printTaskPath(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printResourcePath(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printBatchResources(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printPathInfo(FILE *file, SeqNodeDataPtr node_ptr);
void SeqNode_printNode ( SeqNodeDataPtr node_ptr, unsigned int filters, const char * filename );

void SeqNode_addNodeDependency ( SeqNodeDataPtr node_ptr, SeqDepDataPtr dep);

extern void SeqNode_generateConfig (const SeqNodeDataPtr _nodeDataPtr, const char* flow, const char * filename );
extern char * SeqNode_extension (const SeqNodeDataPtr _nodeDataPtr);


#endif
