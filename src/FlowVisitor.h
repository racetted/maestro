#ifndef _FLOW_VISITOR_H_
#define _FLOW_VISITOR_H_


#define FLOW_SUCCESS 0
#define FLOW_FAILURE -1
#define FLOW_TRUE 1
#define FLOW_FALSE 0

/********************************************************************************
 * This structure contains information that is passed along through the various
 * functions used in getFlowInfo.  Since various parts of the code modify, add
 * to and use these fields, they are passed through this struct to most of the
 * functions.
 *
 * There is possibly more in here than is needed but this will be dealt with as
 * work progresses.
********************************************************************************/
typedef struct _FlowVisitor{
   const char * expHome;
   char * currentFlowNode;
   char * taskPath;
   char * suiteName;
   char * module;
   char * intramodulePath;
   int currentNodeType;
   xmlDocPtr doc;
   xmlXPathContextPtr context;
   xmlDocPtr previousDoc;
   xmlXPathContextPtr previousContext;
} FlowVisitor;

typedef FlowVisitor* FlowVisitorPtr;

/********************************************************************************
 * Initializes the flow_visitor to the entry module;
 * Caller should check if the return pointer is NULL.
********************************************************************************/
FlowVisitorPtr Flow_newVisitor(const char * _seq_exp_home);

/********************************************************************************
 * Destructor for the flow_visitor object.
********************************************************************************/
int Flow_deleteVisitor(FlowVisitorPtr _flow_visitor);

/********************************************************************************
 * Parses the given nodePath while gathering information and adding attributes
 * to the nodeDataPtr;
 * returns FLOW_SUCCESS if it is able to completely parse the path
 * returns FLOW_FAILURE otherwise.
********************************************************************************/
int Flow_parsePath(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr, const char * _nodePath);

/********************************************************************************
 * Tries to get to the node specified by nodePath.
 * Returns FLOW_FAILURE if it can't get to it,
 * Returns FLOW_SUCCESS if it can go through the whole path without a query
 * failing.
 * Note that if the last query was successful, the path walk is considered
 * successful and we don't have to move to the next switch item.
********************************************************************************/
int Flow_walkPath(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr, const char * nodePath);

/********************************************************************************
 * Sets the XML XPath context's current node to the child of the current node
 * whose name attribute matches the nodeName parameter.
********************************************************************************/
int Flow_doNodeQuery(FlowVisitorPtr _flow_visitor, const char * nodeName, int isFirst);

/********************************************************************************
 * Changes the current module of the flow visitor.
 * returns FLOW_FAILURE if either the XML file cannot be parsed or an initial
 *                      "MODULE" query fails to return a result.
 * returns FLOW_SUCCESS oterwise.
********************************************************************************/
int Flow_changeModule(FlowVisitorPtr _flow_visitor, const char * module);

/********************************************************************************
 * Changes the current xml document and context to a new one specified by
 * xmlFilename and saves the previous document and context.
 * Returns FLOW_SUCCESS if the XML file can be successfuly parsed and
 * Returns FLOW_FAILURE otherwise.
********************************************************************************/
int Flow_changeXmlFile(FlowVisitorPtr _flow_visitor, const char * xmlFilename);

/********************************************************************************
 * Appends things to the various paths we are constructing in getFlowInfo.
********************************************************************************/
int Flow_updatePaths(FlowVisitorPtr _flow_visitor, const char * pathToken, const int container);

/********************************************************************************
 * Parses the attributes of a switch node into the nodeDataPtr.
 * Returns FLOW_SUCCESS if the right switch item (or default) is found.
 * Returns FLOW_FAILURE otherwise.
********************************************************************************/
int Flow_parseSwitchAttributes(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr, int isLast );

/********************************************************************************
 * This function returns the switch type of the current node in the XML XPath
 * context
********************************************************************************/
const char * Flow_findSwitchType(const FlowVisitorPtr _flow_visitor );

/********************************************************************************
 * Moves the flow visitor to the switch item of the current node whose name
 * contains switchValue or to the default if no switch item matches switchValue.
 * Returns FLOW_SUCCESS if a valid switch item is found.
 * Returns FLOW_FAILURE if no switch item is found.
********************************************************************************/
int Flow_findSwitchItem( FlowVisitorPtr _flow_visitor,const char *switchValue );

/********************************************************************************
 * Moves visitor to switch item child that has switchValue if it exists
 * Returns FLOW_FAILURE on failure
 * Returns FLOW_SUCCESS on success
********************************************************************************/
int Flow_findSwitchItemWithValue( FlowVisitorPtr _flow_visitor, const char * switchValue);

/********************************************************************************
 * Moves visitor to default switch item child of current node if it exists.
 * Returns FLOW_FAILURE on failure
 * Returns FLOW_SUCCESS on success
********************************************************************************/
int Flow_findDefaultSwitchItem( FlowVisitorPtr _flow_visitor);

/********************************************************************************
 * Returns FLOW_TRUE if the SWITCH_ITEM currentNodePtr has the argument switchValue as
 * one of the tokens in it's name attribute.
 * Note that we use the XML XPath context to perform the attribute search.  For
 * this, we need to change the context's current node temporarily and restore it
 * upon exiting the function.
********************************************************************************/
int Flow_switchItemHasValue(const FlowVisitorPtr _flow_visitor, xmlNodePtr currentNodePtr, const char*  switchValue);

/********************************************************************************
 * Parses the string name as a comma separated list of values and returns 1 if
 * the argument value is one of those values.
 * Returns FLOW_TRUE if the name does contain the switchValue and
 * returns FLOW_FALSE otherwise.
********************************************************************************/
int switchNameContains(const char * name, const char * switchValue);

/********************************************************************************
 * Parses worker path for current node if said node has a work_unit attribute.
********************************************************************************/
int Flow_checkWorkUnit(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Sets the nodeDataPtr's pathToModule, taskPath, suiteName, module and
 * intramodulePath from info gathered while parsing the xml files.
********************************************************************************/
int Flow_setPathData(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Sets the pathToModule of the node by subtracting the intramodulePath from
 * _nodeDataPtr->container.
********************************************************************************/
int Flow_setPathToModule(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Sets the suiteName based on the experiment home.
********************************************************************************/
int Flow_setSuiteName(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Adds flow dependencies to _nodeDataPtr
 * Returns FLOW_SUCCESS if there are dependencies,
 * Returns FLOW_FAILURE if there are none.  It should probably still return
 * FLOW_SUCCESS even if there are no dependencies.
********************************************************************************/
int Flow_parseDependencies(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Parses submits into _nodeDataPtr
********************************************************************************/
int Flow_parseSubmits(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Parses the preceding sibings and following siblings into _nodeDataPtr
 * We should get this tracinf info out of here, it clutters the code and nobody
 * is going to see it.  There should be a function to print this stuff.
********************************************************************************/
int Flow_parseSiblings(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Calls parseNodeSpecifics with the attributes of the current node of the flow
 * visitor.
********************************************************************************/
int Flow_parseSpecifics(FlowVisitorPtr _flow_visitor, SeqNodeDataPtr _nodeDataPtr);

/********************************************************************************
 * Prints the current information of the visitor
********************************************************************************/
void Flow_print_state(FlowVisitorPtr _flow_visitor, int trace_level);

#endif /* _FLOW_VISITOR_H */
