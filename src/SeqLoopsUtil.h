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


#ifndef _SEQ_LOOP_UTIL
#define _SEQ_LOOP_UTIL
#include "SeqNode.h"


/*
 * The attributes should be set to a default value when they are read from the
 * xml but this will do.
 */
#define DEFAULT_LOOP_START 0
#define DEFAULT_LOOP_START_STR strdup("0")

#define DEFAULT_LOOP_END 1
#define DEFAULT_LOOP_END_STR   strdup("1")

#define DEFAULT_LOOP_STEP 1
#define DEFAULT_LOOP_STEP_STR  strdup("1")

#define DEFAULT_LOOP_SET 1
#define DEFAULT_LOOP_SET_STR   strdup("1")


void SeqLoops_setLoopAttribute( SeqNameValuesPtr* loop_attr_ptr, char* attr_name, char* attr_value );
int SeqLoops_parseArgs( SeqNameValuesPtr* nameValuesPtr, const char* cmd_args );
char* SeqLoops_getLoopAttribute( SeqNameValuesPtr loop_attr_ptr, char* attr_name );
char* SeqLoops_getExtensionBase ( SeqNodeDataPtr _nodeDataPtr );
SeqNameValuesPtr SeqLoops_convertExtension ( SeqLoopsPtr loops_ptr, char* extension );
char* SeqLoops_ContainerExtension( SeqLoopsPtr loops_ptr, SeqNameValuesPtr loop_args_ptr ) ;
char* SeqLoops_NodeExtension( const char* node_name, SeqNameValuesPtr loop_args_ptr );
LISTNODEPTR SeqLoops_childExtensions( SeqNodeDataPtr _nodeDataPtr );
LISTNODEPTR SeqLoops_childExtensionsInReverse( SeqNodeDataPtr _nodeDataPtr );
int SeqLoops_isParentLoopContainer ( const SeqNodeDataPtr _nodeDataPtr );
char* SeqLoops_getExtPattern ( char* extension ) ;
char* SeqLoops_getLoopArgs( SeqNameValuesPtr _loop_args );
void SeqLoops_printLoopArgs( SeqNameValuesPtr _loop_args, const char* _caller );
SeqNameValuesPtr SeqLoops_submitLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
int SeqLoops_isLastIteration( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
SeqNameValuesPtr SeqLoops_nextLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args , int* newDefNumber);
int SeqLoops_validateLoopArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
char* SeqLoops_getExtFromLoopArgs( SeqNameValuesPtr _loop_args );
SeqNameValuesPtr SeqLoops_getLoopSetArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args, int defNumber );
SeqNameValuesPtr SeqLoops_getContainerArgs( const SeqNodeDataPtr _nodeDataPtr, SeqNameValuesPtr _loop_args );
LISTNODEPTR SeqLoops_getLoopContainerExtensions( SeqNodeDataPtr _nodeDataPtr, const char * depIndex ); 
LISTNODEPTR SeqLoops_getLoopContainerExtensionsInReverse( SeqNodeDataPtr _nodeDataPtr, const char * depIndex ); 
void SeqLoops_validateNumLoopExpression( char * _expression);
char * SeqLoops_indexToExt( const char * index);
char* SeqLoops_getLoopArgs( SeqNameValuesPtr _loop_args );
SeqLoopsPtr SeqLoops_findLoopByName( SeqLoopsPtr loopsPtr, char * name);



#endif
