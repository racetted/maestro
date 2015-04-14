#ifndef LOGREADER_H
#define LOGREADER_H

typedef struct  _Node_prm {
   char Node[256];
   char TNode[256];
   char loop[64];
   char stime[18];
   char btime[18];
   char etime[18];
   char itime[18];
   char atime[18];
   char wtime[18];
   char dtime[18];
   char LastAction;
   int ignoreNode;
} Node_prm;

typedef struct _ListNodes {
   struct _Node_prm PNode;
   struct _ListNodes *next;
} ListNodes;

typedef struct _ListListNodes {
   int Nodelength;
   struct _ListNodes *Ptr_LNode;
   struct _ListListNodes *next;
} ListListNodes;

typedef struct _LoopExt {
   char Lext[32];
   char lstime[18];
   char lbtime[18];
   char letime[18];
   char litime[18];
   char latime[18];
   char lwtime[18];
   char ldtime[18];
   char exectime[10];
   char submitdelay[10];
   char LastAction;
   struct _LoopExt *next;
} LoopExt;

typedef struct _NodeLoopList {
   char Node[256];
   struct _LoopExt *ptr_LoopExt;
   struct _NodeLoopList *next;
} NodeLoopList;

/* global */
extern struct _ListListNodes MyListListNodes;
extern struct _NodeLoopList MyNodeLoopList;
/* read_type: 0=all, 1=statuses, 2=stats*/
extern int read_type;

extern struct stat pt;
extern FILE *log;

extern void read_file  ( char *base, FILE *log);
extern void insert_node( char S, char *node, char *loop, char *stime, char *btime, char *etime , char *atime, char *itime , char *wtime, char *dtime, char * exectime, char * submitdelay); 
extern void print_LListe ( struct _ListListNodes MyListListNodes , FILE *log);
extern void reset_node (char *node, char *ext);
extern char * sconcat(char *ptr1,char *ptr2);
extern void delete_node(struct _ListNodes *node, struct _ListListNodes *list);
#endif