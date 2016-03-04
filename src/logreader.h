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
   char deltafromstart[10];
   char LastAction;
   int ignoreNode;
   struct _LoopExt *next;
} LoopExt;

typedef struct _NodeLoopList {
   char Node[256];
   struct _LoopExt *ptr_LoopExt;
   struct _NodeLoopList *next;
} NodeLoopList;

typedef struct _PastTimes {
   char *begin;
   char *submit;
   char *end;
   char *submitdelay;
   char *exectime;
   char *deltafromstart;
   struct _PastTimes *next;
} PastTimes;

typedef struct _StatsNode {
   char *node;
   char *member;
   int times_counter;
   struct _PastTimes *times;
   struct _StatsNode *next;
} StatsNode;

/* global */
extern struct _ListListNodes MyListListNodes;
extern struct _NodeLoopList MyNodeLoopList;

extern struct _StatsNode *rootStatsNode;

/* read_type: 0=statuses & stats, 1=statuses, 2=stats, 3=averages*/
extern int read_type;

extern struct stat pt;
extern FILE *stats;

extern void read_file  ( char *base);
extern void insert_node( char S, char *node, char *loop, char *stime, char *btime, char *etime , char *atime, char *itime , char *wtime, char *dtime, char * exectime, char * submitdelay); 
extern void print_LListe ( struct _ListListNodes MyListListNodes, FILE *outputFile);

extern void getAverage(char *exp, char *datestamp);
extern char *getNodeAverageLine(char *node, char *member);
extern void computeAverage(char *exp, char *datestamp, int stats_days);
extern int getStats(FILE *_stats);
extern int parseStatsLine(char line[1024]);
extern int addStatsNode(char *node, char *member, char *stime, char *btime, char *etime, char *exectime, char *submitdelay, char *deltafromstart);
extern int processStats(char *exp, char *datestamp);
extern char *secondsToChar (int seconds);
extern int charToSeconds (char *timestamp);
extern void reset_node (char *node, char *ext);
extern char * previousDay(char today[9]);
extern char * sconcat(char *ptr1,char *ptr2);
extern void delete_node(struct _ListNodes *node, struct _ListListNodes *list);

void logreader(char * inputFilePath, char * outputFilePath, char * exp, char * datestamp, char * type, int statWindow, int clobberFile); 

#endif
