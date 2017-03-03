/* logreader.c - log and statistics reader/creator used by the Maestro sequencer software package.
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


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include "logreader.h"
#include "SeqUtil.h"
#include "SeqDatesUtil.h" 

/* global */
struct _ListListNodes MyListListNodes = { -1 , NULL , NULL };
struct _NodeLoopList MyNodeLoopList = {"first",NULL,NULL};
struct _StatsNode *rootStatsNode;

/* read_type: see LR defines*/
int read_type=LR_SHOW_ALL;
struct stat pt;

void insert_node(char S, char *node, char *loop, char *stime, char *btime, char *etime , char *atime , char *itime, char *wtime, char *dtime, char * exectime, char * submitdelay, char * waitmsg ) {

      char ComposedNode[512];
      struct _ListNodes     *ptr_lhead,  *ptr_Ltrotte, *ptr_Lpreced;
      struct _ListListNodes *ptr_LLtrotte, *ptr_LLpreced;
      int found_len=0 , found_node=0, len=0; 
       
      /*if init state clean statuses of the branch*/
      if (S == 'i') {
         reset_branch(node, loop);
         return;
      }
      SeqUtil_TRACE(TL_FULL_TRACE,"logreader inserting node %s loop %s state %c\n", node, loop, S);

      /* must easier to work like this */
      snprintf(ComposedNode,sizeof(ComposedNode),"%s%s",node,loop);
      len =strlen(ComposedNode);
      
      if ( MyListListNodes.Nodelength == -1 ) {
           /* first time */
           MyListListNodes.Nodelength=len;
	   if ( (ptr_lhead=(struct _ListNodes *) malloc(sizeof(struct _ListNodes))) != NULL ) {
		strcpy(ptr_lhead->PNode.Node,ComposedNode);
	        strcpy(ptr_lhead->PNode.TNode,node);
		strcpy(ptr_lhead->PNode.loop,loop);
		      
		strcpy(ptr_lhead->PNode.atime,"");
		strcpy(ptr_lhead->PNode.btime,"");
		strcpy(ptr_lhead->PNode.etime,"");
		strcpy(ptr_lhead->PNode.itime,"");
		strcpy(ptr_lhead->PNode.stime,"");
		strcpy(ptr_lhead->PNode.wtime,"");
		strcpy(ptr_lhead->PNode.dtime,"");
		      
		switch ( S ) 
		      {
		        case 'a':
	                         strcpy(ptr_lhead->PNode.atime,atime);
				 break;
		        case 'b':
	                         strcpy(ptr_lhead->PNode.btime,btime);
				 break;
		        case 'e':
	                         strcpy(ptr_lhead->PNode.etime,etime);
				 break;
		        case 'i':
	                         strcpy(ptr_lhead->PNode.itime,itime);
				 break;
		        case 's':
	                         strcpy(ptr_lhead->PNode.stime,stime);
				 break;
			case 'w':
				strcpy(ptr_lhead->PNode.wtime,wtime);
                  		strcpy(ptr_lhead->PNode.waitmsg,waitmsg); 
				 break;
			case 'd':
			         strcpy(ptr_lhead->PNode.dtime,dtime);
				 break;
			case 'c':
			         strcpy(ptr_lhead->PNode.dtime,dtime);
				 break;
		      }
		      ptr_lhead->PNode.LastAction = S;
		      ptr_lhead->PNode.ignoreNode = 0;
		      ptr_lhead->next = NULL;
		      MyListListNodes.Ptr_LNode = ptr_lhead;
		      MyListListNodes.next = NULL;
			 
           } else {
	       fprintf(stderr,"cannot malloc \n");
	       exit(1);
	   }
           /* execTime submitDelay */
      } else {
       /* parcouriri la liste for (  ptr_trotte = ptrHEADLLN; ptr_trotte != NULL ; ptr_trotte = ptr_trotte->next)  {  ptr_preced = ptr_trotte; } */
       /* go to end of list for ( ptr_LLtrotte = ptrHEADLLN; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next)  { ptr_LLpreced = ptr_LLtrotte; } */
       /* find node with the same length */
       for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next)
       { 
           if ( ptr_LLtrotte->Nodelength == len ) {
	        found_len=1;
                /* found same length, find  exact node if any  */
                for ( ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode; ptr_Ltrotte != NULL ; ptr_Ltrotte = ptr_Ltrotte->next) {
                               if ( strcmp(ptr_Ltrotte->PNode.Node,ComposedNode) == 0 ) {
                                    found_node=1;
				    /* complete insertion of parameters */
				    switch (S) 
				    {
		                       case 'a':
	                                        strcpy(ptr_Ltrotte->PNode.atime,atime);
				                break;
		                       case 'b':
	                                        strcpy(ptr_Ltrotte->PNode.btime,btime);
				                break;
		                       case 'e':
	                                        strcpy(ptr_Ltrotte->PNode.etime,etime);
				                break;
		                       case 'i':
	                                        strcpy(ptr_Ltrotte->PNode.itime,itime);
				                break;
		                       case 's':
						/*resetting node values in submit state*/
	                                        strcpy(ptr_Ltrotte->PNode.stime,stime);
	                                        strcpy(ptr_Ltrotte->PNode.atime,"");
	                                        strcpy(ptr_Ltrotte->PNode.btime,"");
	                                        strcpy(ptr_Ltrotte->PNode.etime,"");
	                                        strcpy(ptr_Ltrotte->PNode.itime,"");
	                                        strcpy(ptr_Ltrotte->PNode.wtime,"");
	                                        strcpy(ptr_Ltrotte->PNode.dtime,"");
				                break;
				       case 'w':
						strcpy(ptr_Ltrotte->PNode.wtime,wtime);
                  strcpy(ptr_Ltrotte->PNode.waitmsg,waitmsg); 
   
						break;
				       case 'd':
						strcpy(ptr_Ltrotte->PNode.dtime,dtime);
						break;
				       case 'c':
						strcpy(ptr_Ltrotte->PNode.dtime,dtime);
						break;
				    }
		                    ptr_Ltrotte->PNode.LastAction = S;
				    ptr_Ltrotte->PNode.ignoreNode = 0;
				    break;
			       }
			       ptr_Lpreced = ptr_Ltrotte;
		}
                if ( found_node == 0 && (ptr_Lpreced->next = (struct _ListNodes *) malloc(sizeof(struct _ListNodes ))) != NULL ) {
                      
	              strcpy(ptr_Lpreced->next->PNode.Node,ComposedNode);
	              strcpy(ptr_Lpreced->next->PNode.TNode,node);
	              strcpy(ptr_Lpreced->next->PNode.loop,loop);
		      
		      strcpy(ptr_Lpreced->next->PNode.atime,"");
		      strcpy(ptr_Lpreced->next->PNode.btime,"");
		      strcpy(ptr_Lpreced->next->PNode.etime,"");
		      strcpy(ptr_Lpreced->next->PNode.itime,"");
		      strcpy(ptr_Lpreced->next->PNode.stime,"");
		      strcpy(ptr_Lpreced->next->PNode.wtime,"");
		      strcpy(ptr_Lpreced->next->PNode.dtime,"");
		      
		      switch (S) 
		      {
		         case 'a':
	                          strcpy(ptr_Lpreced->next->PNode.atime,atime);
				  break;
                         case 'b':
	                          strcpy(ptr_Lpreced->next->PNode.btime,btime);
				  break;
                         case 'e':
	                          strcpy(ptr_Lpreced->next->PNode.etime,etime);
				  break;
                         case 'i':
	                          strcpy(ptr_Lpreced->next->PNode.itime,itime);
				  break;
                         case 's':
	                          strcpy(ptr_Lpreced->next->PNode.stime,stime);
				  break;
			 case 'w':
			          strcpy(ptr_Lpreced->next->PNode.wtime,wtime);
                   strcpy(ptr_Lpreced->next->PNode.waitmsg,waitmsg); 
			          break;
			 case 'd':
			          strcpy(ptr_Lpreced->next->PNode.dtime,dtime);
			          break;
			 case 'c':
			          strcpy(ptr_Lpreced->next->PNode.dtime,dtime);
			          break;
		      }
		      ptr_Lpreced->next->PNode.LastAction = S;
		      ptr_Lpreced->next->PNode.ignoreNode = 0;
		      ptr_Lpreced->next->next = NULL;
                }
	        break;	
	   } 
	   ptr_LLpreced = ptr_LLtrotte;
       } 


       if ( found_len == 0 && (ptr_LLpreced->next = (struct _ListListNodes *) malloc(sizeof(struct _ListListNodes ))) != NULL ) {
	       ptr_LLpreced->next->Nodelength=len;
	       ptr_LLpreced->next->next= NULL;
	       
	       if ( (ptr_Ltrotte = (struct _ListNodes *) malloc(sizeof(struct _ListNodes ))) != NULL ) {
	               strcpy(ptr_Ltrotte->PNode.Node,ComposedNode);
	               strcpy(ptr_Ltrotte->PNode.TNode,node);
	               strcpy(ptr_Ltrotte->PNode.loop,loop);
		       
		       strcpy(ptr_Ltrotte->PNode.atime,"");
		       strcpy(ptr_Ltrotte->PNode.btime,"");
		       strcpy(ptr_Ltrotte->PNode.etime,"");
		       strcpy(ptr_Ltrotte->PNode.itime,"");
		       strcpy(ptr_Ltrotte->PNode.stime,"");
		       strcpy(ptr_Ltrotte->PNode.wtime,"");
		       strcpy(ptr_Ltrotte->PNode.dtime,"");
		      
		       switch (S) 
		       {
		         case 'a':
	                          strcpy(ptr_Ltrotte->PNode.atime,atime);
				  break;
                         case 'b':
	                          strcpy(ptr_Ltrotte->PNode.btime,btime);
				  break;
                         case 'e':
	                          strcpy(ptr_Ltrotte->PNode.etime,etime);
				  break;
                         case 'i':
	                          strcpy(ptr_Ltrotte->PNode.itime,itime);
				  break;
                         case 's':
	                          strcpy(ptr_Ltrotte->PNode.stime,stime);
				  break;
			 case 'w':
			          strcpy(ptr_Ltrotte->PNode.wtime,wtime);
                   strcpy(ptr_Ltrotte->PNode.waitmsg,waitmsg); 

			          break;
			 case 'd':
			          strcpy(ptr_Ltrotte->PNode.dtime,dtime);
			          break;
			 case 'c':
			          strcpy(ptr_Ltrotte->PNode.dtime,dtime);
			          break;
		       }
		       ptr_Ltrotte->PNode.LastAction = S;
		       ptr_Ltrotte->PNode.ignoreNode = 0;
	               ptr_Ltrotte->next=NULL;
	               ptr_LLpreced->next->Ptr_LNode= ptr_Ltrotte;
	       }
	       
             
        }
       
      }
}

void read_file (char *base)
{
   char *ptr, *qq, *pp;
   char dstamp[18];
   char node[256], signal[16],loop[32], waitmsg[256];
   int size=0;
   for ( ptr = base ; ptr < &base[pt.st_size]; ptr++) {
      memset(dstamp,'\0',sizeof(dstamp));
      memset(node,'\0',sizeof(node));
      memset(signal,'\0',sizeof(signal));
      memset(loop,'\0',sizeof(loop));

      /* Time stamp */
      strncpy(dstamp,ptr+10,17);
          
      /* Node */
      qq = strchr(ptr+28,':');
      size=qq-(ptr+28);
      strncpy(node,ptr+28, size);

      /* signal */
      pp = strchr(qq+1,':');
      size=pp-(qq+9);
      strncpy(signal,qq+9, size);
  
      /* loop */ 
      qq = strchr(pp+1,':');
      size=qq-(pp+1);
      strncpy(loop,pp+1, size);
      switch (signal[0]) {
         case 'a': /* [a]bort */
             insert_node('a', &node[9], &loop[8], "", "", "", dstamp, "", "", "", "", "",""); 
             break;
         case 's': /* [s]ubmit */
             insert_node('s', &node[9], &loop[8], dstamp, "", "", "", "", "", "", "", "",""); 
             break;
         case 'b': /* [b]egin */
             insert_node('b', &node[9], &loop[8], "", dstamp, "", "", "", "", "", "", "",""); 
             break;
         case 'e': /* [e]nd */
            if (signal[1] == 'n') {
               insert_node('e', &node[9], &loop[8], "",  "", dstamp, "", "", "", "", "", "",""); 
            }
            break;
         case 'i': /* [i]nit */
            if (signal[2] == 'i'){
               insert_node('i', &node[9], &loop[8], "",  "", "", "", dstamp, "", "", "", "",""); 
            }
            break;
         case 'w': /* [w]ait */
            /* msg */ 
            memset(waitmsg,'\0',sizeof(waitmsg));
            qq = strchr(qq+1,'=');  
            pp = strchr(ptr,'\n');
            size=pp-qq-1;
            strncpy(waitmsg,qq+1, size);  
            insert_node('w', &node[9], &loop[8], "",  "", "", "", "", dstamp, "", "", "",waitmsg); 
            break;
         case 'd': /* [d]iscret */
            insert_node('d', &node[9], &loop[8], "",  "", "", "", "", "", dstamp, "", "",""); 
            break;
         case 'c': /* [c]atchup */
            insert_node('c', &node[9], &loop[8], "",  "", "", "", "", "", dstamp, "", "",""); 
            break;

      }

      qq = strchr(ptr,'\n');
      ptr=qq;
  
   }

}

void print_LListe ( struct _ListListNodes MyListListNodes, FILE *outputFile) 
{
      struct _ListNodes      *ptr_Ltrotte;
      struct _ListListNodes  *ptr_LLtrotte, *ptr_shortest_LLNode;
      struct _NodeLoopList  *ptr_NLHtrotte, *ptr_NLHpreced;
      struct _LoopExt       *ptr_LEXHead,   *ptr_LXHtrotte, *ptr_LXHpreced ;
      struct tm ti;
      time_t Sepoch,Bepoch,Eepoch,TopNodeSubmitTime;
      time_t ExeTime, SubDelay, RelativeEnd;
      int n, found_loopNode, shortestLength=256, submit_time_found=0, begin_time_found=0, end_time_found=0 ;
      static char sbuffer[10],ebuffer[10], rbuffer[10];
      char *stats_output = NULL, *tmp_output, *tmp_statstring =NULL;
      char output_buffer[256];

      /* Traverse the list to find the top node (shortest nodelength) to get its submit time to do the relative end time calculation */ 
      for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next) {
          if ( ptr_LLtrotte->Nodelength < shortestLength ) {
              ptr_shortest_LLNode=ptr_LLtrotte; 
              shortestLength=ptr_LLtrotte->Nodelength;
          } 
      }
      if ( ptr_shortest_LLNode->Ptr_LNode != NULL  ) {
         n=sscanf(ptr_shortest_LLNode->Ptr_LNode->PNode.stime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
         TopNodeSubmitTime=mktime(&ti);
      } else {
         time(&TopNodeSubmitTime);
      }

      for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next)
      {
	   /* printf all node having this length */
           for ( ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode ; ptr_Ltrotte != NULL ; ptr_Ltrotte = ptr_Ltrotte->next)
	         {
	       found_loopNode = 0;

	       /* timing */
	       n=sscanf(ptr_Ltrotte->PNode.stime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
          if (n == 6) {
            submit_time_found=1; 
	         Sepoch=mktime(&ti);
	         memset(sbuffer,'\0', sizeof(sbuffer));
            strftime (sbuffer,10,"%H:%M:%S",gmtime(&Sepoch));
          } else {    
            submit_time_found=0; 
          }

	       n=sscanf(ptr_Ltrotte->PNode.btime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
          if (n == 6) {
            begin_time_found=1; 
	         Bepoch=mktime(&ti);
	         memset(rbuffer,'\0', sizeof(rbuffer));
            strftime (rbuffer,10,"%H:%M:%S",gmtime(&Bepoch));
	       } else {    
            begin_time_found=0; 
          }

	       n=sscanf(ptr_Ltrotte->PNode.etime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
          if (n == 6) {
            end_time_found=1; 
	         Eepoch=mktime(&ti);
            memset(ebuffer,'\0', sizeof(ebuffer));
            strftime (ebuffer,10,"%H:%M:%S",gmtime(&Eepoch));
          } else {    
            end_time_found=0; 
          }

          SeqUtil_TRACE(TL_FULL_TRACE,"node:%s submit %s begin %s end %s \n",ptr_Ltrotte->PNode.Node,sbuffer,rbuffer,ebuffer ); 

	       memset(ebuffer,'\0', sizeof(ebuffer));
	       memset(sbuffer,'\0', sizeof(sbuffer));
	       memset(rbuffer,'\0', sizeof(rbuffer));
   
          if (begin_time_found && submit_time_found) {
	         SubDelay = Bepoch - Sepoch; /* check sign */
	         strftime (sbuffer,10,"%H:%M:%S",gmtime(&SubDelay));
          }
          if (begin_time_found && end_time_found) {
	         ExeTime = Eepoch - Bepoch;
	         strftime (ebuffer,10,"%H:%M:%S",gmtime(&ExeTime));
          }
          if (end_time_found) {
            RelativeEnd = Eepoch - TopNodeSubmitTime; 
	         strftime (rbuffer,10,"%H:%M:%S",gmtime(&RelativeEnd));
          } 
	       /* end timing */
   
	       /*if ( strcmp(ptr_Ltrotte->PNode.loop,"") != 0 ) {*/
			/* fprintf(stdout,"node:%s is a loop=%s node \n",ptr_Ltrotte->PNode.Node,ptr_Ltrotte->PNode.loop); */
			if ( strcmp(MyNodeLoopList.Node,"first") == 0 ) {
				 strcpy(MyNodeLoopList.Node,ptr_Ltrotte->PNode.TNode);
				 if ( (ptr_LEXHead=(struct _LoopExt *) malloc(sizeof(struct _LoopExt))) != NULL ) {
					  MyNodeLoopList.ptr_LoopExt = ptr_LEXHead;
					  if ( strcmp(ptr_Ltrotte->PNode.loop,"") != 0 ){
					     strcpy(ptr_LEXHead->Lext,ptr_Ltrotte->PNode.loop);
					  } else {
					     strcpy(ptr_LEXHead->Lext,"null");
					  }
					  strcpy(ptr_LEXHead->lstime,ptr_Ltrotte->PNode.stime);
					  strcpy(ptr_LEXHead->lbtime,ptr_Ltrotte->PNode.btime);
					  strcpy(ptr_LEXHead->letime,ptr_Ltrotte->PNode.etime);
					  strcpy(ptr_LEXHead->litime,ptr_Ltrotte->PNode.itime);
					  strcpy(ptr_LEXHead->latime,ptr_Ltrotte->PNode.atime);
					  strcpy(ptr_LEXHead->lwtime,ptr_Ltrotte->PNode.wtime);
					  strcpy(ptr_LEXHead->waitmsg,ptr_Ltrotte->PNode.waitmsg);
					  strcpy(ptr_LEXHead->exectime, ebuffer);
					  strcpy(ptr_LEXHead->submitdelay, sbuffer);
					  strcpy(ptr_LEXHead->deltafromstart, rbuffer);
					  ptr_LEXHead->LastAction=ptr_Ltrotte->PNode.LastAction;
					  ptr_LEXHead->ignoreNode=ptr_Ltrotte->PNode.ignoreNode;
					  ptr_LEXHead->next = NULL;
				 }
			} else {
			   for ( ptr_NLHtrotte = &MyNodeLoopList; ptr_NLHtrotte != NULL ; ptr_NLHtrotte = ptr_NLHtrotte->next) {
				 if ( strcmp(ptr_NLHtrotte->Node,ptr_Ltrotte->PNode.TNode) == 0 ) {
				    if(strcmp(ptr_Ltrotte->PNode.loop,"") != 0){
				       for ( ptr_LXHtrotte = ptr_NLHtrotte->ptr_LoopExt ; ptr_LXHtrotte != NULL ; ptr_LXHtrotte = ptr_LXHtrotte->next) {
					  if( strcmp(ptr_LXHtrotte->Lext, "null") == 0){
					     strcpy(ptr_LXHtrotte->Lext, "all");
					  }
				       }
				    }
				    /* node found , go to end and add loop member*/
				    for ( ptr_LXHtrotte = ptr_NLHtrotte->ptr_LoopExt ; ptr_LXHtrotte != NULL ; ptr_LXHtrotte = ptr_LXHtrotte->next) { ptr_LXHpreced = ptr_LXHtrotte; }
				    if ( (ptr_LXHpreced->next = (struct _LoopExt *) malloc(sizeof(struct _LoopExt ))) != NULL ) {
				       if ( strcmp(ptr_Ltrotte->PNode.loop,"") != 0 ){
					  strcpy(ptr_LXHpreced->next->Lext,ptr_Ltrotte->PNode.loop);
				       } else {
					  strcpy(ptr_LXHpreced->next->Lext,"all");
				       }
				       strcpy(ptr_LXHpreced->next->lstime,ptr_Ltrotte->PNode.stime);
				       strcpy(ptr_LXHpreced->next->lbtime,ptr_Ltrotte->PNode.btime);
				       strcpy(ptr_LXHpreced->next->letime,ptr_Ltrotte->PNode.etime);
				       strcpy(ptr_LXHpreced->next->litime,ptr_Ltrotte->PNode.itime);
				       strcpy(ptr_LXHpreced->next->latime,ptr_Ltrotte->PNode.atime);
				       strcpy(ptr_LXHpreced->next->lwtime,ptr_Ltrotte->PNode.wtime);
				       strcpy(ptr_LXHpreced->next->ldtime,ptr_Ltrotte->PNode.dtime);
				       strcpy(ptr_LXHpreced->next->waitmsg,ptr_Ltrotte->PNode.waitmsg);
				       strcpy(ptr_LXHpreced->next->exectime,ebuffer);
				       strcpy(ptr_LXHpreced->next->submitdelay,sbuffer);
				       strcpy(ptr_LXHpreced->next->deltafromstart,rbuffer);
				       ptr_LXHpreced->next->LastAction=ptr_Ltrotte->PNode.LastAction;
				       ptr_LXHpreced->next->ignoreNode=ptr_Ltrotte->PNode.ignoreNode;
				       ptr_LXHpreced->next->next = NULL;
				    }
				    found_loopNode = 1;
				 }
				 ptr_NLHpreced = ptr_NLHtrotte;
			   }
			   /* loop node not there create */
			   if ( found_loopNode == 0 && (ptr_NLHpreced->next = (struct _NodeLoopList *) malloc(sizeof(struct _NodeLoopList ))) != NULL ) {
				    strcpy(ptr_NLHpreced->next->Node,ptr_Ltrotte->PNode.TNode);
				    ptr_NLHpreced->next->next = NULL;
				    if ( (ptr_LEXHead = (struct _LoopExt *) malloc(sizeof(struct _LoopExt ))) != NULL ) {
					     ptr_NLHpreced->next->ptr_LoopExt = ptr_LEXHead;
					     if ( strcmp(ptr_Ltrotte->PNode.loop,"") != 0 ){
						strcpy(ptr_LEXHead->Lext,ptr_Ltrotte->PNode.loop);
					     } else {
						strcpy(ptr_LEXHead->Lext,"null");
					     }
					     strcpy(ptr_LEXHead->lstime,ptr_Ltrotte->PNode.stime);
					     strcpy(ptr_LEXHead->lbtime,ptr_Ltrotte->PNode.btime);
					     strcpy(ptr_LEXHead->letime,ptr_Ltrotte->PNode.etime);
					     strcpy(ptr_LEXHead->litime,ptr_Ltrotte->PNode.itime);
					     strcpy(ptr_LEXHead->latime,ptr_Ltrotte->PNode.atime);
					     strcpy(ptr_LEXHead->lwtime,ptr_Ltrotte->PNode.wtime);
					     strcpy(ptr_LEXHead->ldtime,ptr_Ltrotte->PNode.dtime);
					     strcpy(ptr_LEXHead->waitmsg,ptr_Ltrotte->PNode.waitmsg);
					     strcpy(ptr_LEXHead->exectime,ebuffer);
					     strcpy(ptr_LEXHead->submitdelay,sbuffer);
					     strcpy(ptr_LEXHead->deltafromstart,rbuffer);
					     ptr_LEXHead->LastAction=ptr_Ltrotte->PNode.LastAction;
					     ptr_LEXHead->ignoreNode=ptr_Ltrotte->PNode.ignoreNode;
					     ptr_LEXHead->next = NULL;
				    }
			   }
			}
	    
	   }
      }

      /* print loop task */
      for ( ptr_NLHtrotte = &MyNodeLoopList; ptr_NLHtrotte != NULL ; ptr_NLHtrotte = ptr_NLHtrotte->next) {
	       stats_output = "\\stats {";
	       fprintf(stdout, "\\/%s", ptr_NLHtrotte->Node);

	       if(read_type == LR_SHOW_ALL || read_type == LR_SHOW_STATUS ) {
	          fprintf(stdout, "\\statuses {");
	       }
          for ( ptr_LXHtrotte = ptr_NLHtrotte->ptr_LoopExt; ptr_LXHtrotte != NULL ; ptr_LXHtrotte = ptr_LXHtrotte->next) {
             if ( ptr_LXHtrotte->ignoreNode == 0 ) {
		         /*sprintf(output_buffer, "%s {exectime %s submitdelay %s submit %s begin %s end %s deltafromstart %s} ",
			         ptr_LXHtrotte->Lext, ptr_LXHtrotte->exectime, ptr_LXHtrotte->submitdelay, ptr_LXHtrotte->lstime,
			         ptr_LXHtrotte->lbtime,  ptr_LXHtrotte->letime, ptr_LXHtrotte->deltafromstart);
               */
               sprintf(output_buffer, "%s {",ptr_LXHtrotte->Lext);  
               tmp_statstring=strdup(output_buffer); 
               if (  ptr_LXHtrotte->exectime && strlen( ptr_LXHtrotte->exectime ) > 0 ) { 
                  tmp_statstring = sconcat (tmp_statstring," exectime ");  
                  tmp_statstring = sconcat (tmp_statstring,ptr_LXHtrotte->exectime);  
               } 
               if (  ptr_LXHtrotte->submitdelay && strlen( ptr_LXHtrotte->submitdelay ) > 0 ) { 
                  tmp_statstring = sconcat (tmp_statstring," submitdelay ");  
                  tmp_statstring = sconcat (tmp_statstring,ptr_LXHtrotte->submitdelay);  
               } 
               if (  ptr_LXHtrotte->lstime && strlen( ptr_LXHtrotte->lstime ) > 0 ) { 
                  tmp_statstring = sconcat (tmp_statstring," submit ");  
                  tmp_statstring = sconcat (tmp_statstring,ptr_LXHtrotte->lstime);  
               } 
               if (  ptr_LXHtrotte->lbtime && strlen( ptr_LXHtrotte->lbtime ) > 0 ) { 
                  tmp_statstring = sconcat (tmp_statstring," begin ");  
                  tmp_statstring = sconcat (tmp_statstring,ptr_LXHtrotte->lbtime);  
               } 
               if (  ptr_LXHtrotte->letime && strlen( ptr_LXHtrotte->letime ) > 0 ) { 
                  tmp_statstring = sconcat (tmp_statstring," end ");  
                  tmp_statstring = sconcat (tmp_statstring,ptr_LXHtrotte->letime);  
               } 
               if (  ptr_LXHtrotte->deltafromstart && strlen( ptr_LXHtrotte->deltafromstart ) > 0 ) { 
                  tmp_statstring = sconcat (tmp_statstring," deltafromstart ");  
                  tmp_statstring = sconcat (tmp_statstring,ptr_LXHtrotte->deltafromstart);  
               } 

               tmp_statstring = sconcat (tmp_statstring," } ");  

		         tmp_output=strdup(stats_output);
		         stats_output=sconcat(tmp_output, tmp_statstring);
                 free(tmp_statstring);
		         if (read_type == LR_SHOW_ALL || read_type == LR_SHOW_STATUS ) {
			         switch(ptr_LXHtrotte->LastAction){
			            case 'a':
			               fprintf(stdout,"%s {abort %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->latime);
			               break;
			            case 'b':
			               fprintf(stdout,"%s {begin %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->lbtime);
			               break;
			            case 'e':
			               fprintf(stdout,"%s {end %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->letime);
			               break;
			            case 'i':
			               fprintf(stdout,"%s {init %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->litime);
			               break;
			            case 's':
			               fprintf(stdout,"%s {submit %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->lstime);
			               break;
			            case 'w':
			               fprintf(stdout,"%s {wait %s {%s}} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->lwtime, ptr_LXHtrotte->waitmsg);
			               break;
			            case 'd':
			               fprintf(stdout,"%s {discret %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->ldtime);
			               break;
			            case 'c':
			               fprintf(stdout,"%s {catchup %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->ldtime);
			               break;
			         }
		        }
		      } else {
                 SeqUtil_TRACE(TL_FULL_TRACE,"logreader ignoring node %s %s\n",ptr_NLHtrotte->Node , ptr_LXHtrotte->Lext);
            } 
		 
		      if (outputFile != NULL) {
		         fprintf(outputFile, "SEQNODE=/%s:MEMBER=%s:SUBMIT=%s:BEGIN=%s:END=%s:EXECTIME=%s:SUBMITDELAY=%s:DELTAFROMSTART=%s\n",
			            ptr_NLHtrotte->Node, ptr_LXHtrotte->Lext, ptr_LXHtrotte->lstime, ptr_LXHtrotte->lbtime, ptr_LXHtrotte->letime,
			             ptr_LXHtrotte->exectime, ptr_LXHtrotte->submitdelay, ptr_LXHtrotte->deltafromstart );
		      }
	       }
	       sprintf(output_buffer, "}");
	       tmp_output=strdup(stats_output);
	       stats_output=sconcat(tmp_output, output_buffer);
	       if (read_type == LR_SHOW_ALL || read_type == LR_SHOW_STATUS) {
	          fprintf(stdout, "}");
	       }
	       if ((read_type == LR_SHOW_ALL || read_type == LR_SHOW_STATS)) {
		       fprintf(stdout, "%s", stats_output);  
	       }

      }
      fprintf(stdout, "\n");  
      
}

/*used to parse averages from avg file*/
void getAverage(char *exp, char *datestamp){
   char *avg_file_path=NULL, *tmp_file_path=NULL;
   char *full_path=NULL;
   char char_datestamp[15];
   char prev[15];
   
   FILE *f=NULL;
   prev[14] = '\0';
   char_datestamp[14]='\0';
   snprintf(char_datestamp, 15, "%s", datestamp);
   strcpy(prev, char_datestamp);
   snprintf(char_datestamp, 15, "%s", SeqDatesUtil_getPrintableDate(prev,-1,0,0,0));
   
   SeqUtil_TRACE(TL_FULL_TRACE,"logreader parsing averages on exp: %s for datestamp: %s\n", exp, datestamp);
   
   tmp_file_path=sconcat("/stats/", char_datestamp);
   avg_file_path=sconcat(tmp_file_path, "_avg");
   full_path=sconcat(exp, avg_file_path);
   
   SeqUtil_TRACE(TL_FULL_TRACE,"logreader collecting averages in file %s\n", full_path);
   
   f = fopen(full_path, "r");
   if(f != NULL) {
      if(getStats(f) != 0){
         SeqUtil_TRACE(TL_ERROR,"logreader error while parsing the following avg file:\n");
         SeqUtil_TRACE(TL_ERROR,"%s\n", full_path);
      }
   } else {
      SeqUtil_TRACE(TL_ERROR,"logreader cannot access file %s\n", full_path);
   }
}

/* Print out average values for all nodes */ 
int printAverage() {
   StatsNode *node_tmpptr=rootStatsNode;
   PastTimes *time_tmpptr;
   char * output_line = NULL, *prev_node=NULL;

   if(rootStatsNode == NULL ) {
      fprintf(stdout,"\n");
      return -1; 
   }
   node_tmpptr=rootStatsNode;

   while ( node_tmpptr != NULL ) { 
      if ((prev_node == NULL) || (strcmp (node_tmpptr->node, prev_node) != 0)){
         fprintf(stdout,"%s\\avg {%s {",node_tmpptr->node, node_tmpptr->member);
      } else {
         fprintf(stdout," %s {", node_tmpptr->member);
      }

      output_line=malloc(256);
      time_tmpptr=node_tmpptr->times;
      if (  time_tmpptr->exectime && strlen( time_tmpptr->exectime ) > 0 ) { 
         output_line = sconcat (output_line," exectime ");  
         output_line = sconcat (output_line,time_tmpptr->exectime);  
      } 
      if (  time_tmpptr->submitdelay && strlen( time_tmpptr->submitdelay ) > 0 ) { 
         output_line = sconcat (output_line," submitdelay ");  
         output_line = sconcat (output_line,time_tmpptr->submitdelay);  
      } 
      if (  time_tmpptr->submit && strlen( time_tmpptr->submit ) > 0 ) { 
         output_line = sconcat (output_line," submit ");  
         output_line = sconcat (output_line,time_tmpptr->submit);  
      } 
      if (  time_tmpptr->begin && strlen( time_tmpptr->begin ) > 0 ) { 
         output_line = sconcat (output_line," begin ");  
         output_line = sconcat (output_line,time_tmpptr->begin);  
      } 
      if (  time_tmpptr->end && strlen( time_tmpptr->end ) > 0 ) { 
         output_line = sconcat (output_line," end ");  
         output_line = sconcat (output_line,time_tmpptr->end);  
      } 
      if (  time_tmpptr->deltafromstart && strlen( time_tmpptr->deltafromstart ) > 0 ) { 
         output_line = sconcat (output_line," deltafromstart ");  
         output_line = sconcat (output_line,time_tmpptr->deltafromstart);  
      } 
      fprintf(stdout,"%s }",output_line);
      free(output_line);
      if ((node_tmpptr->next == NULL) || strcmp (node_tmpptr->next->node, node_tmpptr->node) != 0) {
         fprintf(stdout,"}"); 
      }
      prev_node=node_tmpptr->node; 
      node_tmpptr=node_tmpptr->next;

      if ((node_tmpptr != NULL) && (strcmp (node_tmpptr->node, prev_node) != 0)){
         fprintf(stdout,"\\"); 
      }
   }  
   fprintf(stdout,"\n");
   return 0;

}

/*outputs a node-specific line formatted to fit in the tsv structure for logreader.tcl*/
char *getNodeAverageLine(char *node, char *member){
   char * tmp_statstring; 
   StatsNode *node_tmpptr;
   PastTimes *time_tmpptr;
   
   if(rootStatsNode == NULL || node == NULL || member == NULL) {
      return "";
   }
   node_tmpptr=rootStatsNode;
   
   while((strcmp(node_tmpptr->node+1,node) != 0 || strcmp(node_tmpptr->member, member) != 0) &&
       node_tmpptr->next != NULL) {
      node_tmpptr=node_tmpptr->next;
   }
   if(strcmp(node_tmpptr->node+1,node) == 0 && strcmp(node_tmpptr->member, member) == 0) {
      tmp_statstring=malloc(256);
      time_tmpptr=node_tmpptr->times;
      if (  time_tmpptr->exectime && strlen( time_tmpptr->exectime ) > 0 ) { 
         tmp_statstring = sconcat (tmp_statstring," exectime ");  
         tmp_statstring = sconcat (tmp_statstring,time_tmpptr->exectime);  
      } 
      if (  time_tmpptr->submitdelay && strlen( time_tmpptr->submitdelay ) > 0 ) { 
         tmp_statstring = sconcat (tmp_statstring," submitdelay ");  
         tmp_statstring = sconcat (tmp_statstring,time_tmpptr->submitdelay);  
      } 
      if (  time_tmpptr->submit && strlen( time_tmpptr->submit ) > 0 ) { 
         tmp_statstring = sconcat (tmp_statstring," submit ");  
         tmp_statstring = sconcat (tmp_statstring,time_tmpptr->submit);  
      } 
      if (  time_tmpptr->begin && strlen( time_tmpptr->begin ) > 0 ) { 
         tmp_statstring = sconcat (tmp_statstring," begin ");  
         tmp_statstring = sconcat (tmp_statstring,time_tmpptr->begin);  
      } 
      if (  time_tmpptr->end && strlen( time_tmpptr->end ) > 0 ) { 
         tmp_statstring = sconcat (tmp_statstring," end ");  
         tmp_statstring = sconcat (tmp_statstring,time_tmpptr->end);  
      } 
      if (  time_tmpptr->deltafromstart && strlen( time_tmpptr->deltafromstart ) > 0 ) { 
         tmp_statstring = sconcat (tmp_statstring," deltafromstart ");  
         tmp_statstring = sconcat (tmp_statstring,time_tmpptr->deltafromstart);  
      } 
      return tmp_statstring;
   } else {
      return "";
   }
   
}

/*used to generate averages from stats*/
void computeAverage(char *exp, char *datestamp, int stats_days, FILE * output_file){
   char *stats_file_path = NULL;
   char *full_path = NULL;
   char char_datestamp[15];
   char prev[15];
   int i, count=0;
   
   FILE *f=NULL;
   prev[14] = '\0';
   char_datestamp[14] = '\0';
   snprintf(char_datestamp, 15, "%s", datestamp);
   
   SeqUtil_TRACE(TL_FULL_TRACE,"logreader computing averages on exp: %s for datestamp: %s since last %d days\n", exp, datestamp, stats_days);
   
   for (i=0; i < stats_days; i++){
      stats_file_path = sconcat("/stats/", char_datestamp);
      full_path = sconcat(exp, stats_file_path);
      
      SeqUtil_TRACE(TL_FULL_TRACE,"logreader collecting stats for datestamp %s in file %s\n", char_datestamp, full_path);
      
      f = fopen(full_path, "r");
      if (f != NULL) {
         if(getStats(f) != 0){
            SeqUtil_TRACE(TL_ERROR,"logreader error while parsing the following stats file:\n");
            SeqUtil_TRACE(TL_ERROR,"%s\n", full_path);
         }
         ++count;
         fclose(f);
      }
      
      strcpy(prev, char_datestamp);
      snprintf(char_datestamp, 15, "%s", SeqDatesUtil_getPrintableDate(prev,-1,0,0,0));
   }
   if (count == 0) {
      fprintf(stderr,"Unable to calculate average; missing required statistics files under %s/stats\n", exp);
      exit(1); 
   }
   processStats(exp, datestamp, output_file);
}

/*translate stats file in linked list*/
int getStats(FILE *_stats){
   char tmpline[1024];
   
   if(_stats == NULL) {
      return -1;
   }
   /*process lines one by one*/
   while ((fgets(tmpline, sizeof(tmpline), _stats) != NULL) && (strstr(tmpline, "SEQNODE") != NULL)) {
      if(parseStatsLine(tmpline) != 0) {
         SeqUtil_TRACE(TL_ERROR,"logreader parsing error at the following stats line:\n%s \n",tmpline);
      }
   }
   return 0;
}

/*parse stats line and add stats node*/
int parseStatsLine(char line[1024]){
   char *node, *member, *stime, *btime, *etime, *exectime, *submitdelay, *deltafromstart;
   int ret=0;
   
   char tmp_node[]="SEQNODE=", tmp_member[]="MEMBER=", tmp_stime[]="SUBMIT=", 
      tmp_btime[]="BEGIN=", tmp_etime[]="END=", tmp_exectime[]="EXECTIME=", 
      tmp_submitdelay[]="SUBMITDELAY=", tmp_deltafromstart[]="DELTAFROMSTART=";

   int node_str_len=strlen(tmp_node), member_str_len=strlen(tmp_member), stime_str_len=strlen(tmp_stime), 
      btime_str_len=strlen(tmp_btime), submitdelay_str_len=strlen(tmp_submitdelay), 
      deltafromstart_str_len=strlen(tmp_deltafromstart), etime_str_len=strlen(tmp_etime), 
      exectime_str_len=strlen(tmp_exectime);
   
   char *nodeptr, *memberptr, *stimeptr, *btimeptr, *etimeptr, *exectimeptr,
      *submitdelayptr, *deltafromstartptr;
   
   int nodelen, memberlen, stimelen, btimelen, etimelen, exectimelen, submitdelaylen, deltafromstartlen;
   
   nodeptr=strstr(line, tmp_node)+node_str_len;
   memberptr=strstr(line, tmp_member)+member_str_len;
   stimeptr=strstr(line, tmp_stime)+stime_str_len;
   btimeptr=strstr(line, tmp_btime)+btime_str_len;
   etimeptr=strstr(line, tmp_etime)+etime_str_len;
   exectimeptr=strstr(line, tmp_exectime)+exectime_str_len;
   submitdelayptr=strstr(line, tmp_submitdelay)+submitdelay_str_len;
   deltafromstartptr=strstr(line, tmp_deltafromstart)+deltafromstart_str_len;
   
   if (!(nodeptr || memberptr || stimeptr || btimeptr || etimeptr || exectimeptr || submitdelay || deltafromstartptr )) {
      SeqUtil_TRACE(TL_ERROR,"parseStatsLine error at the following stats line:\n%s \n",line);
      return -1; 
   }
   deltafromstartlen = strlen(deltafromstartptr)-1;
   submitdelaylen = strlen(submitdelayptr)-deltafromstartlen-deltafromstart_str_len-1-1;
   exectimelen = strlen(exectimeptr)-submitdelaylen-deltafromstartlen-deltafromstart_str_len-submitdelay_str_len-1-1-1;
   etimelen = strlen(etimeptr)-exectimelen-submitdelaylen-deltafromstartlen-deltafromstart_str_len-submitdelay_str_len-exectime_str_len-1-1-1-1;
   btimelen = strlen(btimeptr)-etimelen-exectimelen-submitdelaylen-deltafromstartlen-deltafromstart_str_len-submitdelay_str_len-exectime_str_len-etime_str_len-1-1-1-1-1;
   stimelen = strlen(stimeptr)-btimelen-etimelen-exectimelen-submitdelaylen-deltafromstartlen-deltafromstart_str_len-submitdelay_str_len-exectime_str_len-etime_str_len-btime_str_len-1-1-1-1-1-1;
   memberlen = strlen(memberptr)-stimelen-btimelen-etimelen-exectimelen-submitdelaylen-deltafromstartlen-deltafromstart_str_len-submitdelay_str_len-exectime_str_len-etime_str_len-btime_str_len-stime_str_len-1-1-1-1-1-1-1;
   nodelen = strlen(nodeptr)-memberlen-stimelen-btimelen-etimelen-exectimelen-submitdelaylen-deltafromstartlen-deltafromstart_str_len-submitdelay_str_len-exectime_str_len-etime_str_len-btime_str_len-stime_str_len-member_str_len-1-1-1-1-1-1-1-1;

   node=malloc(nodelen+1);
   member=malloc(memberlen+1);
   stime=malloc(stimelen+1);
   btime=malloc(btimelen+1);
   etime=malloc(etimelen+1);
   exectime=malloc(exectimelen+1);
   submitdelay=malloc(submitdelaylen+1);
   deltafromstart=malloc(deltafromstartlen+1);
    
   if(node == NULL || member == NULL || stime==NULL || btime == NULL || etime == NULL ||
      exectime == NULL || submitdelay == NULL || deltafromstart == NULL) {
      SeqUtil_TRACE(TL_ERROR,"parseStatsLine malloc problem \n");
      return -1;
   }
    
   snprintf(node, nodelen+1, "%s", nodeptr);
   snprintf(member, memberlen+1, "%s", memberptr);
   snprintf(stime, stimelen+1, "%s", stimeptr);
   snprintf(btime, btimelen+1, "%s", btimeptr);
   snprintf(etime, etimelen+1, "%s", etimeptr);
   snprintf(exectime, exectimelen+1, "%s", exectimeptr);
   snprintf(submitdelay, submitdelaylen+1, "%s", submitdelayptr);
   snprintf(deltafromstart, deltafromstartlen+1, "%s", deltafromstartptr);
   
   ret=addStatsNode(node, member, stime, btime, etime, exectime, submitdelay, deltafromstart);
   return ret;
}

/*add stat node to linked list plus set root node if the list is empty*/
int addStatsNode(char *node, char *member, char* stime, char *btime, char *etime, char *exectime, char *submitdelay, char *deltafromstart){
   StatsNode *stats_node_ptr;
   StatsNode *node_tmpptr;
   PastTimes *past_times_ptr = (PastTimes*)malloc(sizeof(PastTimes));
   PastTimes *time_tmpptr;
   
   if(past_times_ptr == NULL){
      return -1;
   }
   
   past_times_ptr->submit = stime; 
   past_times_ptr->begin = btime;
   past_times_ptr->end = etime;
   past_times_ptr->submitdelay = submitdelay;
   past_times_ptr->exectime = exectime;
   past_times_ptr->deltafromstart = deltafromstart;
   past_times_ptr->next=NULL;
   
   if(rootStatsNode == NULL) {
      stats_node_ptr = (StatsNode*)malloc(sizeof(StatsNode));
      
      if(stats_node_ptr == NULL) {
         return -1;
      }
      
      stats_node_ptr->node=node;
      stats_node_ptr->member=member;
      stats_node_ptr->times=past_times_ptr;
      stats_node_ptr->times_counter=1;
      stats_node_ptr->next=NULL;
      
      rootStatsNode = stats_node_ptr;
   } else {
      stats_node_ptr = rootStatsNode;
      
      while((strcmp(stats_node_ptr->node,node) != 0 || strcmp(stats_node_ptr->member, member) != 0) &&
          stats_node_ptr->next != NULL) {
         stats_node_ptr=stats_node_ptr->next;
      }
      
      if(strcmp(stats_node_ptr->node,node) == 0 && strcmp(stats_node_ptr->member, member) == 0) {
         time_tmpptr=stats_node_ptr->times;
      
         while(time_tmpptr->next != NULL){
            time_tmpptr=time_tmpptr->next;
         }
         
         time_tmpptr->next=past_times_ptr;
         stats_node_ptr->times_counter=stats_node_ptr->times_counter+1;
      } else {
         node_tmpptr = (StatsNode*)malloc(sizeof(StatsNode));
         
         if(node_tmpptr == NULL) {
            return -1;
         }
         
         node_tmpptr->node=node;
         node_tmpptr->member=member;
         node_tmpptr->times=past_times_ptr;
         node_tmpptr->times_counter=1;
         node_tmpptr->next=NULL;

         stats_node_ptr->next=node_tmpptr;
      }
   }
   
   SeqUtil_TRACE(TL_FULL_TRACE,"logreader addStatsNode node:%s member:%s\n", node, member);
   
   return 0;
}

/*final step to the computation of averages; it calculates from the linked list and outputs results to avg file*/
int processStats(char *exp, char *datestamp, FILE * output_file){
   StatsNode *node_tmpptr;
   PastTimes *time_tmpptr;
   char *begin=NULL, *end=NULL, *submitdelay=NULL, *exectime=NULL, *deltafromstart=NULL, *submit=NULL;
   int int_begin[30], int_end[30], int_submitdelay[30], int_exectime[30], int_deltafromstart[30], int_submit[30];
   int avg_counter, iter_counter,i,truncate_amount=0;
   
   if(rootStatsNode == NULL) {
      return 0;
   }
   node_tmpptr=rootStatsNode;
   
   while(node_tmpptr != NULL){
      time_tmpptr=node_tmpptr->times;
      iter_counter=0;
      for(i=0; i<30; ++i) {
        int_submit[i]=0;
        int_begin[i]=0;
        int_end[i]=0;
        int_submitdelay[i]=0;
        int_exectime[i]=0;
        int_deltafromstart[i]=0;
      }
      
      avg_counter=node_tmpptr->times_counter;
      if (avg_counter > 30) {
          raiseError("ERROR: Maximum average count is 30. Please reduce the amount of days you wish to calculate the mean. \n"); 
      }
       
      while(time_tmpptr != NULL){
         int_submit[iter_counter] = charToSeconds(time_tmpptr->submit);
         int_begin[iter_counter] = charToSeconds(time_tmpptr->begin);
         int_end[iter_counter] = charToSeconds(time_tmpptr->end);
         int_submitdelay[iter_counter] = charToSeconds(time_tmpptr->submitdelay);
         int_exectime[iter_counter] = charToSeconds(time_tmpptr->exectime);
         int_deltafromstart[iter_counter] = charToSeconds(time_tmpptr->deltafromstart);
         iter_counter++; 
         time_tmpptr=time_tmpptr->next;
      }
      
      /* truncating at least 1 extreme on each side per 10 elements, starting at 5 */
      if (avg_counter < 4) {
          truncate_amount=0;
      } else {
          truncate_amount=(avg_counter + 9) / 10;
      }
      submit=secondsToChar(SeqUtil_basicTruncatedMean(&int_submit, avg_counter, truncate_amount));
      begin=secondsToChar(SeqUtil_basicTruncatedMean(&int_begin, avg_counter, truncate_amount));
      end=secondsToChar(SeqUtil_basicTruncatedMean(&int_end, avg_counter, truncate_amount));
      submitdelay=secondsToChar(SeqUtil_basicTruncatedMean(&int_submitdelay, avg_counter,truncate_amount));
      exectime=secondsToChar(SeqUtil_basicTruncatedMean(&int_exectime, avg_counter,truncate_amount));
      deltafromstart=secondsToChar(SeqUtil_basicTruncatedMean(&int_deltafromstart, avg_counter,truncate_amount));
 
      SeqUtil_printOrWrite(output_file, "SEQNODE=%s:MEMBER=%s:SUBMIT=%s:BEGIN=%s:END=%s:EXECTIME=%s:SUBMITDELAY=%s:DELTAFROMSTART=%s\n",
             node_tmpptr->node, node_tmpptr->member,submit, begin, end, exectime, submitdelay, deltafromstart);
      node_tmpptr=node_tmpptr->next;
   }
   
   return 0;
}

/*format a number of seconds to timestamp HH:MM:SS*/
char *secondsToChar (int seconds) {
   int result_hour, result_minute, result_second;
   char buffer[9];
   
   result_hour=(seconds/3600);
   result_minute=(seconds%3600)/60;
   result_second=(seconds%3600)%60;
   
   buffer[8]='\0';
   snprintf(buffer, 9, "%02i:%02i:%02i", result_hour, result_minute, result_second);
   
   return strdup(buffer);
}

/*returns the number of seconds represented by timestamp*/
int charToSeconds (char *_timestamp) {
   char hour[3], minute[3], second[3];
   int total_seconds;
   char *timestamp=_timestamp;
   
   if(strstr(_timestamp, ".") != NULL) {
      timestamp=strstr(_timestamp, ".")+1;
   }
   
   hour[0]=timestamp[0];
   hour[1]=timestamp[1];
   hour[2]='\0';
   minute[0]=timestamp[3];
   minute[1]=timestamp[4];
   minute[2]='\0';
   second[0]=timestamp[6];
   second[1]=timestamp[7];
   second[2]='\0';
   total_seconds=(atoi(second)+(atoi(minute)*60)+(atoi(hour)*60*60));
   
   return total_seconds;
}

/*edited from http://www.c4learn.com/c-programming/c-concating-strings-dynamic-allocation */
char *sconcat(char *ptr1,char *ptr2){
   int len1,len2;
   int i,j;
   char *ptr3;
   
   len1 = strlen(ptr1);
   len2 = strlen(ptr2);
   
   ptr3 = (char *)malloc((len1+len2+1)*sizeof(char));
   
   for(i=0; ptr1[i] != (char) 0; i++)
      ptr3[i] = ptr1[i];
   
   for(j=0; ptr2[j] != (char) 0; j++, i++)
      ptr3[i] = ptr2[j];
   
   ptr3[i] = (char) 0;
   return ptr3;
}

/*in case of init, when parsing log file*/
void reset_branch (char *node, char *ext) {
   struct _ListNodes      *ptr_Ltrotte;
   struct _ListListNodes  *ptr_LLtrotte;
   struct _ListNodes      *tmp_prev_list;
   
   
   for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next) {
      ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode;
      while (ptr_Ltrotte != NULL){
         tmp_prev_list=ptr_Ltrotte;
         ptr_Ltrotte = ptr_Ltrotte->next;
         
         if (strcmp(node, tmp_prev_list->PNode.TNode) == 0 && strncmp(ext, tmp_prev_list->PNode.loop, strlen(ext)) == 0) {
            /*delete_node(tmp_prev_list, ptr_LLtrotte);*/
            tmp_prev_list->PNode.ignoreNode=1;
            SeqUtil_TRACE(TL_FULL_TRACE,"logreader reset branch done on node: %s ext: %s \n",node,ext);
         }
      }
   }
}

/*not used for now (creates memfault btw, ain't Antoine got time for that), ignoreNode attribute used instead*/
void delete_node(struct _ListNodes *node, struct _ListListNodes *list) {
   struct _ListNodes *todelete=NULL;
   struct _ListNodes *prev=NULL;
   
   if (node == NULL || list == NULL) {
      return;
   }
   
   todelete=list->Ptr_LNode;
   while((todelete != node) && (todelete->next != NULL)) {
      prev=todelete;
      todelete = todelete->next;
   }
   if(todelete == node) {
      if(prev != NULL) {
         prev->next=todelete->next;
      }
      if (list->Ptr_LNode == todelete) {
         list->Ptr_LNode=todelete->next;
      }
   }
   
   if (todelete != NULL) {
      free(todelete);
      todelete=NULL;
   }
}


/* logreader API call
*
*  inputFilePath -- reading target, default is $exp/logs/$datestamp_nodelog
*  outputFilePath -- writing target, default is $exp/stats/$datestamp[,_avg]
*  exp -- target experiment, default is $SEQ_EXP_HOME
*  datestamp -- target datestamp, default is $SEQ_DATE
*  statWindow -- size of the average window, in days. 
*  type -- what kind of statistics are being generated (avg, stats, statuses)
*  clobberFile -- is there a filecheck before writing to the output. 0 = don't clobber 
*
*/ 
void logreader(char * inputFilePath, char * outputFilePath, char * exp, char * datestamp, int type, int statWindow, int clobberFile ) {

   FILE *output_file = NULL;
   char * base; 
   int fp=-1, ret; 
   char input_file_path[512], optional_output_path[512], optional_output_dir[512];
   
   if (exp == NULL || datestamp==NULL) {
      raiseError("logreader: exp and datestamp must be defined to use logreader\n");
   }
   
   if (strlen(datestamp) > 14) raiseError("logreader: datestamp format error. Should be YYYYMMDDHHmmss, is %s\n",datestamp);

   if ( inputFilePath ) { 
      snprintf(input_file_path,512,"%s",inputFilePath);
      fp = open(inputFilePath, O_RDONLY, 0 );
   } else { 
      sprintf(input_file_path, "%s/logs/%s_nodelog" , exp,datestamp);  
   }
   fp = open(input_file_path, O_RDONLY, 0 );

   if ( outputFilePath ) { 
       output_file = fopen(outputFilePath, "w+");
	    if (output_file == NULL) {
	       fprintf(stderr, "Cannot open output file %s", outputFilePath );
	    }
   }

   rootStatsNode = NULL;
   
   read_type=type; 
   
   if (read_type == LR_SHOW_STATS) {
      if (outputFilePath == NULL){
         sprintf(optional_output_dir, "%s/stats" , exp);
         if( ! SeqUtil_isDirExists( optional_output_dir )) {
            SeqUtil_TRACE(TL_FULL_TRACE, "mkdir: creating dir %s\n", optional_output_dir );
            if ( mkdir( optional_output_dir, 0755 ) == -1 ) {
               fprintf ( stderr, "Cannot create: %s Reason: %s\n",optional_output_dir, strerror(errno) );
               exit(EXIT_FAILURE);
            }
         }
         sprintf(optional_output_path, "%s/stats/%s" , exp,datestamp);  
         if ( (clobberFile == 0) && (access(optional_output_path,R_OK)==0)) {
            SeqUtil_TRACE(TL_FULL_TRACE,"logreader: no clobber output flag set and file exists, writing to /dev/null\n");
            sprintf(optional_output_path, "/dev/null");  
         }
               
         if ( ( output_file = fopen(optional_output_path, "w+") ) == NULL) { 
            fprintf(stderr, "Cannot open output file %s \n", optional_output_path);
            exit(EXIT_FAILURE);
         }
      }
   }

   if ((read_type == LR_SHOW_ALL) || (read_type == LR_SHOW_STATUS) || (read_type == LR_SHOW_STATS))  {
      if ( fp < 0 ) {
         fprintf(stderr,"Cannot open input file %s\n",input_file_path );
         exit(1);
      }
   
      if ( fstat(fp,&pt) != 0 ) {
         fprintf(stderr,"Cannot fstat file %s\n", input_file_path);
         exit(1);
      }

      if ( ( base = mmap(NULL, pt.st_size, PROT_READ, MAP_SHARED, fp, (off_t)0) ) == (char *) MAP_FAILED ) {
         if (errno == EINVAL) {
            fprintf (stdout,"\n");
            close(fp);
            if (output_file != NULL) fclose(output_file);
            exit(EXIT_SUCCESS);
         } else { 
            fprintf(stderr,"Map failed \n");
            close(fp);
            if (output_file != NULL) fclose(output_file);
            exit(EXIT_FAILURE);
         }
      }
      read_file(base); 
   
      /* unmap */
      munmap(base, pt.st_size);  
      
      /*print node status and/or stats*/
      print_LListe ( MyListListNodes, output_file );

   }

   if ((read_type == LR_SHOW_ALL) || (read_type == LR_SHOW_AVG)) {   
      getAverage(exp, datestamp);
      ret=printAverage();  
   }

   if (read_type == LR_CALC_AVG) { 
      if (outputFilePath == NULL){
         sprintf(optional_output_dir, "%s/stats" , exp);
         if( ! SeqUtil_isDirExists( optional_output_dir )) {
            SeqUtil_TRACE(TL_FULL_TRACE, "mkdir: creating dir %s\n", optional_output_dir );
            if ( mkdir( optional_output_dir, 0755 ) == -1 ) {
               fprintf ( stderr, "Cannot create: %s Reason: %s\n",optional_output_dir, strerror(errno) );
               exit(EXIT_FAILURE);
            }
         }
         sprintf(optional_output_path, "%s/stats/%s_avg" , exp,datestamp);  
         if ( (clobberFile == 0) && (access(optional_output_path,R_OK)==0)) {
            SeqUtil_TRACE(TL_FULL_TRACE,"logreader: no clobber output flag set and file exists, writing to /dev/null\n");
            sprintf(optional_output_path, "/dev/null");  
         }

         if ( ( output_file = fopen(optional_output_path, "w+") ) == NULL) { 
            fprintf(stderr, "Cannot open output file %s", optional_output_path);
            exit(1);
         }
      }

      computeAverage(exp, datestamp, statWindow, output_file);
   }
   
   if (read_type == LR_SHOW_ALL) {
      SeqUtil_printOrWrite(output_file,"last_read_offset %d \n", pt.st_size);
   } 

   if (output_file != NULL) fclose(output_file);

}




