#ifndef SEQ_DEPENDS_H
#define SEQ_DEPENDS_H


typedef enum _SeqDependsType {
   NodeDependancy,
   DateDependancy
} SeqDependsType;

typedef enum _SeqDependsScope {
   IntraSuite,
   IntraUser,
   InterUser,
} SeqDependsScope;


/********************************************************************************
 * Notes: At commit 5c6f031300f54434c39ef06801315264104b0c6f and before,
 * dependency data was stored as a SeqNameValuesPtr.  To facilitate working with
 * dependencies and making the code pertaining to them easier to modify, they
 * are now stored as a struct.  Some fields are the fields that were registered
 * in the nodeDataPtr's ndp->depends linked list of dependencies.  These are
 * node_name, index, exp, status, index, local_index, hour, time_delta,
 * protocol, valid_hour, valid_dow.  These are the properties read from the XML
 * file when doing parseDepends() (nodeinfo.c).  These fields are set to the
 * property retrieved from the XML or to a dynamically allocated empty string if
 * the property is not present in the XML.
 *
 * The other ones are fields that directly pertain to the dependency but are
 * calculated or set later.
 * datestamp: calculated based on the nodeDataPtr's datestamp and the hour or
 *            time_delta fields (formerly depDatestamp in validateDependencies()).
 * ext, local_ext: are the extensions corresponding to the index and local_index
 *                 fields respectively.
 * isInScope: Is calculated in processDepStatus to decide whether the dependency
 *            is on a node that exists in the current context. (formerly
 *            isDepInScope in validateDependencies())
 * exp_scope: Formerly depScope in validateDependencies(), this is the scope in
 *            terms of Linux users : IntraSuite, IntraUser, InterUser.
 *
 * node_path was read from xmlGetProp(..., "path"), passed to
 * SeqNode_addNodeDependency() who did nothing with it.  I'll delete it later.
********************************************************************************/
typedef struct _SeqDependencyData {
   SeqDependsType type;
   SeqDependsScope exp_scope;
   int isInScope;
   char *node_name;
   char *node_path;
   char *exp;
   char *status;
   char *index;
   char *ext;
   char *local_index;
   char *local_ext;
   char *hour;
   char *time_delta;
   char *datestamp;
   char *valid_hour;
   char *valid_dow;
   char *protocol;
} SeqDependencyData;
typedef SeqDependencyData* SeqDepDataPtr;

SeqDepDataPtr SeqDep_newDep(void);
void SeqDep_deleteDep(SeqDepDataPtr * dep);
void SeqDep_printDep(int trace_level, SeqDepDataPtr dep);

typedef struct _SeqDependsNode {
   struct _SeqDependsList *nextPtr;
   SeqDepDataPtr depData;
} SeqDependsNode;
typedef SeqDependsNode* SeqDepNodePtr;

SeqDepNodePtr newDepNode(SeqDepDataPtr depData);
void SeqDep_addDepNode (SeqDepNodePtr* list_head, SeqDepDataPtr depData);
void SeqDep_deleteDepList (SeqDepNodePtr* list_head);

#endif /* SEQ_DEPENDS_H */
