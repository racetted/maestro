#ifndef SEQ_DEPENDS_H
#define SEQ_DEPENDS_H


typedef enum _SeqDependsType {
   NodeDependancy,
   DateDependancy
} SeqDependsType;

typedef struct _SeqDependencyData {
   SeqDependsType type;
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
