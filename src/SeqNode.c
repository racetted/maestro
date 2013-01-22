#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include "SeqNode.h"
#include "SeqUtil.h"
static char* FamilyTypeString = "Family";
static char* TaskTypeString = "Task";
static char* NpassTaskTypeString = "NpassTask";
static char* LoopTypeString = "Loop";
static char* CaseTypeString = "Case";
static char* CaseItemTypeString = "CaseItem";
static char* ModuleTypeString = "Module";
static char* SwitchTypeString = "Switch"; 

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
      case Case:
         typePtr = CaseTypeString;
         break;
      case CaseItem:
         typePtr = CaseItemTypeString;
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
      node_ptr->name = malloc( strlen(name) + 1 );
      strcpy( node_ptr->name, name );
   }
}

void SeqNode_setNodeName ( SeqNodeDataPtr node_ptr, const char* nodeName ) {
   if ( nodeName != NULL ) {
      free( node_ptr->nodeName );
      node_ptr->nodeName = malloc( strlen(nodeName) + 1 );
      strcpy( node_ptr->nodeName, nodeName );
   }
}

void SeqNode_setModule ( SeqNodeDataPtr node_ptr, const char* module ) {
   if ( module != NULL ) {
      free( node_ptr->module );
      node_ptr->module = malloc( strlen(module) + 1 );
      strcpy( node_ptr->module, module );
   }
}

void SeqNode_setPathToModule ( SeqNodeDataPtr node_ptr, const char* pathToModule ) {
   if ( pathToModule != NULL ) {
      free( node_ptr->pathToModule );
      node_ptr->pathToModule = malloc( strlen(pathToModule) + 1 );
      strcpy( node_ptr->pathToModule, pathToModule );
   }
}

void SeqNode_setIntramoduleContainer ( SeqNodeDataPtr node_ptr, const char* intramodule_container ) {
   if( intramodule_container != NULL ) {
      free( node_ptr->intramodule_container );
      node_ptr->intramodule_container = malloc( strlen(intramodule_container) + 1 );
      strcpy( node_ptr->intramodule_container, intramodule_container );
   }
}

void SeqNode_setContainer ( SeqNodeDataPtr node_ptr, const char* container ) {
   if( container != NULL ) {
      free( node_ptr->container );
      node_ptr->container = malloc( strlen(container) + 1 );
      strcpy( node_ptr->container, container );
   }
}

void SeqNode_setCpu ( SeqNodeDataPtr node_ptr, const char* cpu ) {
   char *tmpstrtok=NULL;
   char *tmpCpu=NULL;
   if ( cpu != NULL ) {
      free( node_ptr->cpu );
      node_ptr->cpu = malloc( strlen(cpu) + 1 );
      strcpy( node_ptr->cpu, cpu );
      tmpCpu=strdup(cpu);
  
      /* parse NPEX */
      tmpstrtok = (char*) strtok( tmpCpu, "x" );
      if ( tmpstrtok != NULL ) {
          free( node_ptr->npex );
	  node_ptr->npex=malloc( strlen(tmpstrtok) +1); 
	  strcpy(node_ptr->npex, tmpstrtok);
      }
      /* NPEY */
      tmpstrtok = (char*) strtok( NULL, "x" );
      if ( tmpstrtok != NULL ) {
          free( node_ptr->npey );
	  node_ptr->npey=malloc( strlen(tmpstrtok) +1); 
	  strcpy(node_ptr->npey, tmpstrtok);
      }
      /* OMP */
      tmpstrtok = (char*) strtok( NULL, "x" );
      if ( tmpstrtok != NULL ) {
          free( node_ptr->omp );
	  node_ptr->omp=malloc( strlen(tmpstrtok) +1); 
	  strcpy(node_ptr->omp, tmpstrtok);
      }
   free (tmpCpu);
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
    node_ptr->cpu_multiplier = malloc( strlen(cpu_multiplier) + 1 );
    strcpy( node_ptr->cpu_multiplier, tmpMult );
    free (tmpMultTok);
  }
   free (tmpMult);
}

void SeqNode_setMachine ( SeqNodeDataPtr node_ptr, const char* machine ) {
   if ( machine != NULL ) {
      free( node_ptr->machine );
      node_ptr->machine = malloc( strlen(machine) + 1 );
      strcpy( node_ptr->machine, machine );
   }
}

void SeqNode_setMemory ( SeqNodeDataPtr node_ptr, const char* memory ) {
   if ( memory != NULL ) {
      free( node_ptr->memory );
      node_ptr->memory = malloc( strlen(memory) + 1 );
      strcpy( node_ptr->memory, memory );
   }
}

void SeqNode_setQueue ( SeqNodeDataPtr node_ptr, const char* queue ) {
   if ( queue != NULL ) {
      free( node_ptr->queue );
      node_ptr->queue = malloc( strlen(queue) + 1 );
      strcpy( node_ptr->queue, queue );
   }
}

void SeqNode_setSuiteName ( SeqNodeDataPtr node_ptr, const char* suiteName ) {
   if ( suiteName != NULL ) {
      free( node_ptr->suiteName );
      node_ptr->suiteName = malloc( strlen(suiteName) + 1 );
      strcpy( node_ptr->suiteName, suiteName );
   }
}

void SeqNode_setInternalPath ( SeqNodeDataPtr node_ptr, const char* path ) {
   if ( path != NULL ) {
      free( node_ptr->taskPath );
      node_ptr->taskPath = malloc( strlen(path) + 1 );
      strcpy( node_ptr->taskPath, path );
   }
}

void SeqNode_setArgs ( SeqNodeDataPtr node_ptr, const char* args ) {
   if ( args != NULL ) {
      free( node_ptr->args );
      node_ptr->args = malloc( strlen(args) + 1 );
      strcpy( node_ptr->args, args );
   }
}

void SeqNode_setWorkerPath ( SeqNodeDataPtr node_ptr, const char* workerPath ) {
   if ( workerPath != NULL ) {
      free( node_ptr->workerPath );
      node_ptr->workerPath = malloc( strlen(workerPath) + 1 );
      strcpy( node_ptr->workerPath, workerPath );
   }
}

void SeqNode_setSoumetArgs ( SeqNodeDataPtr node_ptr, char* soumetArgs ) {
   if ( soumetArgs != NULL ) {
      free( node_ptr->soumetArgs );
      node_ptr->soumetArgs = malloc( strlen(soumetArgs) + 1 );
      strcpy( node_ptr->soumetArgs, soumetArgs );
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
      node_ptr->alias = malloc( strlen(alias) + 1 );
      strcpy( node_ptr->alias, alias );
   }
}

void SeqNode_setDatestamp( SeqNodeDataPtr node_ptr, const char* datestamp) {
   if ( datestamp != NULL ) {
      free( node_ptr->datestamp );
      node_ptr->datestamp = malloc( strlen(datestamp) + 1 );
      strcpy( node_ptr->datestamp, datestamp );
   }
}

void SeqNode_setWorkdir( SeqNodeDataPtr node_ptr, const char* workdir) {
   if ( workdir != NULL ) {
      free( node_ptr->workdir);
      node_ptr->workdir = malloc( strlen(workdir) + 1 );
      strcpy( node_ptr->workdir, workdir);
   }
}

void SeqNode_setExtension ( SeqNodeDataPtr node_ptr, const char* extension ) {
   if ( extension != NULL ) {
      free( node_ptr->extension );
      node_ptr->extension = malloc( strlen(extension) + 1 );
      strcpy( node_ptr->extension, extension );
   }
}

/* returns ptr to newly allocated memory structure
 to store a new name value link list */
SeqLoopsPtr SeqNode_allocateLoopsEntry ( SeqNodeDataPtr node_ptr ) {
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE( "SeqNode_allocateLoopsEntry()\n" );
   if ( node_ptr->loops == NULL ) {
      node_ptr->loops = malloc( sizeof (SeqLoops) );
      loopsPtr = node_ptr->loops;
   } else {
      loopsPtr = node_ptr->loops;
      /* position ourselves at the end of the list */
      while( loopsPtr->nextPtr != NULL ) {
         loopsPtr = loopsPtr->nextPtr;
      }
   
      /* allocate memory for new data */
      loopsPtr->nextPtr = malloc( sizeof(SeqLoops) );
      /* go to memory for new data */
      loopsPtr = loopsPtr->nextPtr;
   }
   loopsPtr->nextPtr = NULL;
   loopsPtr->values = NULL;
   loopsPtr->loop_name = NULL;
   SeqUtil_TRACE( "SeqNode_allocateLoopsEntry() done\n" );
   return loopsPtr;
}

/* returns ptr to newly allocated memory structure 
   to store a new name value link list */
SeqDependenciesPtr SeqNode_allocateDepsEntry ( SeqNodeDataPtr node_ptr ) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqUtil_TRACE( "SeqNode_allocateDepsEntry()\n" );
   if ( node_ptr->depends == NULL ) {
      node_ptr->depends = malloc( sizeof (SeqDependencies) );
      deps_ptr = node_ptr->depends;
   } else {
      deps_ptr = node_ptr->depends;
      /* position ourselves at the end of the list */
      while( deps_ptr->nextPtr != NULL ) {
         deps_ptr = deps_ptr->nextPtr;
      }

      /* allocate memory for new data */
      deps_ptr->nextPtr = malloc( sizeof(SeqDependencies) );
      /* go to memory for new data */
      deps_ptr = deps_ptr->nextPtr;
   }
   deps_ptr->nextPtr = NULL;
   deps_ptr->dependencyItem = NULL;
   SeqUtil_TRACE( "SeqNode_allocateDepsEntry() done\n" );
   return deps_ptr;
}

/* returns ptr to newly allocated memory structure
   to store a name value pair */
SeqNameValuesPtr SeqNode_allocateDepsNameValue ( SeqDependenciesPtr deps_ptr ) {
   SeqNameValuesPtr nameValuesPtr = NULL;

   SeqUtil_TRACE( "SeqNode_allocateDepsNameValue()\n" );
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
      nameValuesPtr->nextPtr = malloc( sizeof(SeqNameValues) );
      /* go to memory for new data */
      nameValuesPtr = nameValuesPtr->nextPtr;
   }
   /* set end of list */
   nameValuesPtr->nextPtr = NULL;
   nameValuesPtr->name = NULL;
   nameValuesPtr->value = NULL;
   SeqUtil_TRACE( "SeqNode_allocateDepsNameValue() done\n" );
   return nameValuesPtr;
}

/* allocates and store the name-value pair in the
   name-value structure */
void SeqNode_setDepsNameValue (SeqNameValuesPtr name_values_ptr, char* name, char* value ) {
   /* printf ("SeqNode_setDepsNameValue name=%s value=%s\n", name , value ); */
   name_values_ptr->name = malloc( sizeof(char) * ( strlen( name ) + 1) );
   strcpy( name_values_ptr->name, (char*)name );
   if ( value != NULL ) {
      name_values_ptr->value = malloc( sizeof(char) * ( strlen( value ) + 1) );
      strcpy( name_values_ptr->value, (char*)value );
   } else {
      name_values_ptr->value = malloc( sizeof(char) * 2);
      strcpy( name_values_ptr->value, "" );
   }
}

/* add dependency of type node i.e. tasks/family  */
void SeqNode_addNodeDependency ( SeqNodeDataPtr node_ptr, SeqDependsType type, char* dep_node_name, char* dep_node_path,
                         char* dep_user, char* dep_exp, char* dep_status, char* dep_index, char* local_index, char* dep_hour) {
   SeqDependenciesPtr deps_ptr = NULL;
   SeqNameValuesPtr nameValuesPtr = NULL;
   SeqUtil_TRACE( "SeqNode_addNodeDependency() dep_node=%s, dep_node_path=%s, dep_user=%s, dep_exp=%s, dep_status=%s, dep_index=%s, local_index=%s\n",
      dep_node_name, dep_node_path, dep_user, dep_exp, dep_status, dep_index, local_index );
   deps_ptr = SeqNode_allocateDepsEntry( node_ptr );
   deps_ptr->type = type;
   nameValuesPtr = deps_ptr->dependencyItem;
   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "NAME", dep_node_name );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "INDEX", dep_index );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "USER", dep_user );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "EXP", dep_exp );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "STATUS", dep_status );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "LOCAL_INDEX", local_index );

   nameValuesPtr = SeqNode_allocateDepsNameValue (deps_ptr);
   SeqNode_setDepsNameValue ( nameValuesPtr, "HOUR", dep_hour );
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

/* default numerical loop with start, step, set, end */

void SeqNode_addNumLoop ( SeqNodeDataPtr node_ptr, 
   char* loop_name, char* start, char* step, char* set, char* end ) {
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE( "SeqNode_addNumLoop() loop_name=%s, start=%s, step=%s, set=%s, end=%s, \n",loop_name, start, step, set, end );

   loopsPtr = SeqNode_allocateLoopsEntry( node_ptr );
   loopsPtr->type = Numerical;
   loopsPtr->loop_name = strdup( loop_name );
   SeqNameValues_insertItem( &loopsPtr->values, "TYPE", "Default");
   SeqNameValues_insertItem( &loopsPtr->values, "START", start );
   SeqNameValues_insertItem( &loopsPtr->values, "STEP", step );
   SeqNameValues_insertItem( &loopsPtr->values, "SET", set );
   SeqNameValues_insertItem( &loopsPtr->values, "END", end );
}

void SeqNode_addSwitch ( SeqNodeDataPtr _nodeDataPtr, char* switchName, char* switchType, char* returnValue) {
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE( "SeqNode_addSwitch() switchName=%s switchType=%s returnValue=%s\n", switchName, switchType, returnValue);
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
   /* SeqUtil_TRACE( "SeqNode.SeqNode_addSpecificData() called name:%s value:%s\n", tmp, value ); */
   SeqNameValues_insertItem( &(node_ptr->data), tmp, value );
   free( tmp );
}

void SeqNode_setError ( SeqNodeDataPtr node_ptr, const char* message ) {
   node_ptr->error = 1;
   node_ptr->errormsg = malloc( strlen( message ) + 1 );
   strcpy( node_ptr->errormsg, message );
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
   nodePtr->wallclock = 3;
   nodePtr->mpi = 0;
   nodePtr->isLastNPTArg = 0;
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
   nodePtr->extension = NULL;
   nodePtr->datestamp = NULL;
   nodePtr->switchAnswers = NULL;
   nodePtr->workdir = NULL;
   nodePtr->workerPath= NULL;
   SeqNode_setName( nodePtr, "" );
   SeqNode_setContainer( nodePtr, "" );
   SeqNode_setIntramoduleContainer( nodePtr, "" );
   SeqNode_setModule( nodePtr, "" );
   SeqNode_setPathToModule( nodePtr, "" );
   SeqNode_setCpu( nodePtr, "1" );
   SeqNode_setCpuMultiplier( nodePtr, "1" );
   SeqNode_setQueue( nodePtr, "null" );
   SeqNode_setMachine( nodePtr, "dorval-ib" );
   SeqNode_setMemory( nodePtr, "40M" );
   SeqNode_setArgs( nodePtr, "" );
   SeqNode_setSoumetArgs( nodePtr, "" );
   SeqNode_setWorkerPath( nodePtr, "");
   SeqNode_setAlias( nodePtr, "" );
   SeqNode_setInternalPath( nodePtr, "" );
   SeqNode_setExtension( nodePtr, "" );
   SeqNode_setDatestamp( nodePtr, "" );
   SeqNode_setWorkdir( nodePtr, "" );
   nodePtr->error = 0;
   nodePtr->errormsg = NULL;
}

void SeqNode_printNode ( SeqNodeDataPtr node_ptr, const char* filters, const char * filename ) {

   char *tmpstrtok = NULL, *tmpFilters ;
   int showAll = 0, showCfgPath = 0, showTaskPath = 0, showRessource = 0; 
   int showType = 0, showNode = 0, showRootOnly = 0, showResPath = 0, showVar=0;
   SeqNameValuesPtr nameValuesPtr = NULL ;
   SeqDependenciesPtr depsPtr = NULL;
   LISTNODEPTR submitsPtr = NULL, siblingsPtr = NULL, abortsPtr = NULL;
   SeqLoopsPtr loopsPtr = NULL;
   SeqUtil_TRACE( "SeqNode.SeqNode_printNode() called\n" );

   if( filename != NULL ) {
      removeFile(filename);
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
         if ( strcmp( tmpstrtok, "res_path" ) == 0 ) showResPath = 1;
         if ( strcmp( tmpstrtok, "type" ) == 0 ) showType = 1;
         if ( strcmp( tmpstrtok, "node" ) == 0 ) showNode= 1;
         if ( strcmp( tmpstrtok, "root" ) == 0 ) showRootOnly = 1;

         tmpstrtok = (char*) strtok(NULL,",");
      }

      if  (( showAll || showType || showCfgPath || showRessource || showTaskPath || showNode || showRootOnly || showResPath || showVar ) == 0) {
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
      /*SeqUtil_printOrWrite(filename,"************ Node Dependencies \n"); */
      depsPtr = node_ptr->depends;
   
      while( depsPtr != NULL ) {
         nameValuesPtr =  depsPtr->dependencyItem;
   
         /*SeqUtil_printOrWrite(filename,"********* Dependency Item \n"); */
         if ( depsPtr->type == NodeDependancy ) {
            SeqUtil_printOrWrite(filename,"node.depend.type=Node\n");
         } else if ( depsPtr->type == DateDependancy ) { 
            SeqUtil_printOrWrite(filename,"node.depend.type=Date\n");
         }
         while (nameValuesPtr != NULL ) {
            if( strlen( nameValuesPtr->value ) > 0 ) 
               SeqUtil_printOrWrite(filename,"node.depend.%s=%s\n", nameValuesPtr->name, nameValuesPtr->value );
            nameValuesPtr = nameValuesPtr->nextPtr;
         }
         depsPtr  = depsPtr->nextPtr;
      }
   
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
   }

   if (showVar) {
        SeqNode_generateConfig( node_ptr,"continue", filename); 
   }

   free( tmpFilters );
   SeqUtil_TRACE( "SeqNode.SeqNode_printNode() done\n" );
}

SeqNodeDataPtr SeqNode_createNode ( char* name ) {
   SeqNodeDataPtr nodeDataPtr = NULL;
   SeqUtil_TRACE( "SeqNode.SeqNode_createNode() started\n" );
   nodeDataPtr = malloc( sizeof( SeqNodeData ) );
   SeqNode_init ( nodeDataPtr );
   SeqNode_setName( nodeDataPtr, name );
   SeqNode_setNodeName ( nodeDataPtr, (char*) SeqUtil_getPathLeaf(name));
   SeqNode_setContainer ( nodeDataPtr, (char*) SeqUtil_getPathBase(name));
   SeqUtil_TRACE( "SeqNode.SeqNode_createNode() done\n" );
   return nodeDataPtr;
}

void SeqNode_freeNameValues ( SeqNameValuesPtr _nameValuesPtr ) {
   SeqNameValuesPtr nameValuesNextPtr = NULL;
   /* free a link-list of name-value pairs */
   while (_nameValuesPtr != NULL ) {
      /*SeqUtil_TRACE("   SeqNode_freeNameValues %s=%s\n", _nameValuesPtr->name, _nameValuesPtr->value ); */

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
void SeqNode_generateConfig (const SeqNodeDataPtr _nodeDataPtr, const char* flow, const char * filename) {
   char *extName = NULL;
   int stringLength = 0; 
   char pidbuf[100];
   char shortdate[11];
   char *tmpdir = NULL, *loopArgs = NULL, *containerLoopArgs = NULL, *containerLoopExt = NULL, *tmpValue = NULL, *tmp2Value = NULL;
   SeqNameValuesPtr loopArgsPtr=NULL , containerLoopArgsList = NULL;
   SeqUtil_stringAppend( &extName, _nodeDataPtr->name );
   if( strlen( _nodeDataPtr->extension ) > 0 ) {
      SeqUtil_stringAppend( &extName, "." );
      SeqUtil_stringAppend( &extName, _nodeDataPtr->extension );
   }
   SeqUtil_printOrWrite( filename, "eval $(ssmuse sh -d %s -p maestro_%s)\n", getenv("SEQ_MAESTRO_DOMAIN"), getenv("SEQ_MAESTRO_VERSION"));
   SeqUtil_printOrWrite( filename, "eval $(ssmuse sh -d %s -p maestro-utils_%s)\n", getenv("SEQ_UTILS_DOMAIN"), getenv("SEQ_UTILS_VERSION"));
   SeqUtil_printOrWrite( filename, "export SEQ_EXP_HOME=%s\n",  getenv("SEQ_EXP_HOME"));
   SeqUtil_printOrWrite( filename, "export SEQ_EXP_NAME=%s\n", _nodeDataPtr->suiteName); 
   SeqUtil_printOrWrite( filename, "export SEQ_WRAPPER=%s\n", getenv("SEQ_WRAPPER"));
   SeqUtil_printOrWrite( filename, "export SEQ_TRACE_LEVEL=%d\n", SeqUtil_getTraceLevel());
   SeqUtil_printOrWrite( filename, "export SEQ_MODULE=%s\n", _nodeDataPtr->module);
   SeqUtil_printOrWrite( filename, "export SEQ_CONTAINER=%s\n", _nodeDataPtr->container); 
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
   /* Check for :last NPT arg */
   if (_nodeDataPtr->isLastNPTArg){
      tmpValue=SeqNameValues_getValue(loopArgsPtr, _nodeDataPtr->nodeName); 
      /*remove the :last, raise flag that node has a :last*/
      stringLength=strlen(tmpValue)-5;
      tmp2Value=malloc(stringLength+1); 
      memset(tmp2Value,'\0', stringLength+1);
      strncpy(tmp2Value, tmpValue, stringLength); 
      SeqUtil_stringAppend( &tmp2Value, "" );
      SeqUtil_TRACE("SeqLoops_GenerateConfig Found ^last argument, replacing %s for %s for node %s \n", tmpValue, tmp2Value, _nodeDataPtr->nodeName); 
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
   if (strlen(_nodeDataPtr->datestamp) > 10) {
      strncpy(shortdate,_nodeDataPtr->datestamp,10);
   } else {
      strcpy(shortdate,_nodeDataPtr->datestamp);
   }
   SeqUtil_printOrWrite( filename, "export SEQ_SHORT_DATE=%s\n", shortdate); 


   free(tmpdir);
   free(tmpValue);
   free(tmp2Value);
   free(loopArgs);
   free(loopArgsPtr);
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
