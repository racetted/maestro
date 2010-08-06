#include "SeqListNode.h"
#include "SeqNameValues.h"
#ifndef _SEQ_NODE
#define _SEQ_NODE

typedef enum _SeqDependsType {
   NodeDependancy,
   NpassDependancy,
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
   Case,
   CaseItem,
} SeqNodeType;

typedef struct _SeqDependencies {
   SeqNameValuesPtr dependencyItem;
   /* the type is a discriminant to
   process the generic name-value structure */
   SeqDependsType type;
   struct _SeqDependencies *nextPtr;
} SeqDependencies;

typedef SeqDependencies *SeqDependenciesPtr;

/* not used now*/
typedef struct _SeqDependencyLocator {
   char* suite_path;
   char* node_taskPath;
} SeqDependencyLocator;
typedef SeqDependencyLocator *SeqDependencyLocatorPtr;

typedef enum _SeqLoopType {
   Numerical,
   Date
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
   int   silent;
   int   catchup;
   int   mpi;
   int   wallclock;
   char* queue;
   char* machine;
   char* memory;
   char* cpu;
   char* npex;
   char* npey;
   char* omp;
   char* masterfile;
   char* alias;
   char* args;
   char* datestamp;
   char* workdir;

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

   /* relative node path within suite task dir structure */
   char* taskPath;

   /* relative path to the config file */
   char* cfg_file;

   /* loop_extensions */
   char* extension;

   int error;
   char* errormsg;
} SeqNodeData;

typedef SeqNodeData *SeqNodeDataPtr;

SeqNodeDataPtr SeqNode_createNode ( char* name );


#endif
