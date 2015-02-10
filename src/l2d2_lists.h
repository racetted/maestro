#include <time.h>
#ifndef L2D2_LISTS_H
#define L2D2_LISTS_H

typedef struct _dpnode 
{
	char sxp[1024];
	char snode[1024];
	char depOnXp[1024];
	char depOnNode[1024];
	char slargs[512];
	char depOnLargs[512];
	char link[256];
	char waitfile[1024]; 
	char sdstmp[32];
	char depOnDstmp[32];
	char key[64];
        struct _dpnode *next;
} dpnode;

/* forward function declarations */
/* insert list_ptr , dependent|dependee xp , */
int  insert (dpnode **pointer, char *sxp , char *snode , char *depOnxp , char *depOnnode, char * sdtsmp, char * depstmp, char *slargs, char *depOnlargs , char *depkey , char *link , char *wfile );
int  find   (dpnode *pointer, char *key);
int  delete (dpnode *pointer, char *key);
void free_list ( dpnode *ptr);

#endif
