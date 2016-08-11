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
#ifndef _NODEINFO_H_
#define _NODEINFO_H_

#include "SeqNode.h"
#include <libxml/xpath.h>
#include "nodeinfo_filters.h"


extern SeqNodeDataPtr nodeinfo ( const char* node, unsigned int filters,
                                 SeqNameValuesPtr _loops, const char* _exp_home,
                                 char *extraArgs, char* datestamp, const char * switch_args );
extern int doesNodeExist(const char* node, const char* _exp_home, const char * datestamp);
char * switchReturn( SeqNodeDataPtr _nodeDataPtr, const char* switchType );
void parseLoopAttributes (xmlXPathObjectPtr _result, const char* _loop_node_path, SeqNodeDataPtr _nodeDataPtr);
void parseForEachTarget(xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr);
void parseBatchResources (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr);
void parseAbortActions (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr);
void parseSubmits (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr);
void parseNodeSiblings (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr);
void parseNodeSpecifics (SeqNodeType _nodeType, xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr);
void parseDepends (xmlXPathObjectPtr _result, SeqNodeDataPtr _nodeDataPtr, int isIntraDep );
void parseWorkerPath (char * pathToNode, const char * _seq_exp_home, SeqNodeDataPtr _nodeDataPtr );
SeqNodeType getNodeType ( const xmlChar *_node_name );
void getNodeLoopContainersAttr (  SeqNodeDataPtr _nodeDataPtr, const char *_loop_node_path, const char *_seq_exp_home );
void getFlowInfo ( SeqNodeDataPtr _nodeDataPtr, const char *_seq_exp_home,
                     const char *_nodePath,const char *switch_args, unsigned int filters);
extern const char * NODE_RES_XML_ROOT;
extern const char * NODE_RES_XML_ROOT_NAME;

#endif /* _NODEINFO_H_ */
