#ifndef SEQ_DEPENDS_H
#define SEQ_DEPENDS_H

#include "SeqNode.h"
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
#endif /* SEQ_DEPENDS_H */
