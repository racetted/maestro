#ifndef _SEQ_LOOP_UTIL
#define _SEQ_LOOP_UTIL
#include "SeqNode.h"


void SeqLoops_setLoopAttribute( SeqNameValuesPtr* loop_attr_ptr, char* attr_name, char* attr_value );
int SeqLoops_parseArgs( SeqNameValuesPtr* nameValuesPtr, const char* cmd_args );
char* SeqLoops_getLoopAttribute( SeqNameValuesPtr loop_attr_ptr, char* attr_name );
char* SeqLoops_getExtensionBase ( SeqNodeDataPtr _nodeDataPtr );
SeqNameValuesPtr SeqLoops_convertExtension ( SeqLoopsPtr loops_ptr, char* extension );
char* SeqLoops_ContainerExtension( SeqLoopsPtr loops_ptr, SeqNameValuesPtr loop_args_ptr ) ;
char* SeqLoops_NodeExtension( const char* node_name, SeqNameValuesPtr loop_args_ptr );
LISTNODEPTR SeqLoops_childExtensions( SeqNodeDataPtr _nodeDataPtr );
int SeqLoops_isParentLoopContainer ( const SeqNodeDataPtr _nodeDataPtr );
char* SeqLoops_getExtPattern ( char* extension ) ;
char* SeqLoops_getLoopArgs( SeqNameValuesPtr _loop_args );
void SeqLoops_printLoopArgs( SeqNameValuesPtr _loop_args, const char* _caller );
SeqNameValuesPtr SeqLoops_submitLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
int SeqLoops_isLastIteration( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
SeqNameValuesPtr SeqLoops_nextLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
int SeqLoops_validateLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
char* SeqLoops_getExtFromLoopArgs( SeqNameValuesPtr _loop_args );
SeqNameValuesPtr SeqLoops_getLoopSetArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
SeqNameValuesPtr SeqLoops_getContainerArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
LISTNODEPTR SeqLoops_getLoopContainerExtensions( SeqNodeDataPtr _nodeDataPtr, const char * depIndex ); 

#endif
