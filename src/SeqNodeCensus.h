#ifndef _SEQ_NODE_CENSUS_H_
#define _SEQ_NODE_CENSUS_H_

/********************************************************************************
 * DOCUMENTATION: Interface.
 * These are functions used to get a list of the nodes of an experiment.
 *
 * getNodeList() must receive a seq_exp_home path to designate which experiment
 * we want a nodelist for.  Optionally, a datestamp can be provided in which
 * case we will get a list consisting only of nodes that exist at the current
 * datestamp.
 *
 * The function returns a linked list of PathArgNode objects which can be
 * iterated on with the macro for_pap(itr,list_head).  The macro automatically
 * declares the iteration variable.  An example usage can be found in the
 * function write_db_file() in tsvinfo.c.
 *
 * For inner workings, please see the file SeqNodeCensus.c.
********************************************************************************/



/********************************************************************************
 * Struct for making a list of node paths with accompanying switch arguments.
 * These switch arguments are used to know which switch branch to take since the
 * context (datestamp hour or day etc.) is not necessarily the one that
 * corresponds to the node whose path we are looking at.
********************************************************************************/
typedef struct _PathArgNode{
   struct _PathArgNode *nextPtr;
   const char * path;
   const char * switch_args;
   SeqNodeType type;
} PathArgNode;
typedef PathArgNode *PathArgNodePtr;

#define for_pap_list(iterator, list_head)                                      \
   PathArgNodePtr iterator;                                                    \
   for( iterator = list_head;                                                  \
        iterator != NULL;                                                      \
        iterator = iterator->nextPtr                                           \
      )

/********************************************************************************
 * Returns a list of nodes for a given experiment.
 * The format of the list is as a pair consisting of
 * path : The path to the node
 * switch_args : The branches of switches that must be taken to get to the node.
 * NOTE: The caller is responsible for deleting the list.
********************************************************************************/
PathArgNodePtr getNodeList(const char * seq_exp_home, const char *datestamp);
int PathArgNode_deleteList(PathArgNodePtr *list_head);
void PathArgNode_printList(PathArgNodePtr list_head, int trace_level);
#endif
