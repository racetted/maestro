#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "logreader.h"
#include "SeqUtil.h"


void insert_node(char S, char *node, char *loop, char *stime, char *btime, char *etime , char *atime , char *itime, char *wtime, char *dtime, char * exectime, char * submitdelay) {

      char ComposedNode[512];
      struct _ListNodes     *ptr_lhead,  *ptr_Ltrotte, *ptr_Lpreced;
      struct _ListListNodes *ptr_llhead, *ptr_LLtrotte, *ptr_LLpreced;
      int found_len=0 , found_node=0, len=0; 
      
      /*if init state clean statuses of the branch*/
      if (S == 'i') {
	 reset_node(node, loop);
	 return;
      }
      
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
				 break;
			case 'd':
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
				    /* complete insertion of parameteres */
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
						break;
				       case 'd':
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
			          break;
			 case 'd':
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
			          break;
			 case 'd':
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
   char *ptr, *q, *qq, *pp;
   char string[1024];
   char dstamp[18];
   char node[128], signal[16],loop[32];
   int n,len,indx=0;
   for ( ptr = base ; ptr < &base[pt.st_size]; ptr++)
   {
          memset(dstamp,'\0',sizeof(dstamp));
          memset(node,'\0',sizeof(node));
          memset(signal,'\0',sizeof(signal));
          memset(loop,'\0',sizeof(loop));

          /* Time stamp */
          strncpy(dstamp,ptr+10,17);
          
	  /* Node */
	  qq = strchr(ptr+28,':');
	  indx=qq-(ptr+28);
          strncpy(node,ptr+28, indx);

	  /* signal */
          pp = strchr(qq+1,':');
          indx=pp-(qq+9);
          strncpy(signal,qq+9, indx);
  
          /* loop */ 
	  qq = strchr(pp+1,':');
	  indx=qq-(pp+1);
          strncpy(loop,pp+1, indx);
  
	  switch (signal[0]) 
	  {
	     case 'a': /* [a]bort */
                     insert_node('a', &node[9], &loop[8], "", "", "", dstamp, "", "", "", "", ""); 
	             break;
	     case 's': /* [s]ubmit */
                     insert_node('s', &node[9], &loop[8], dstamp, "", "", "", "", "", "", "", ""); 
	             break;
	     case 'b': /* [b]egin */
                     insert_node('b', &node[9], &loop[8], "", dstamp, "", "", "", "", "", "", ""); 
	             break;
	     case 'e': /* [e]nd */
		     if (signal[1] == 'n') {
                        insert_node('e', &node[9], &loop[8], "",  "", dstamp, "", "", "", "", "", ""); 
		     }
	             break;
	     case 'i': /* [i]nit */
		     if (signal[2] == 'i'){
                        insert_node('i', &node[9], &loop[8], "",  "", "", "", dstamp, "", "", "", ""); 
		     }
	             break;
	     case 'w': /* [w]ait */
		     insert_node('w', &node[9], &loop[8], "",  "", "", "", "", dstamp, "", "", ""); 
		     break;
	     case 'd': /* [d]iscret */
		     insert_node('d', &node[9], &loop[8], "",  "", "", "", "", "", dstamp, "", ""); 
		     break;

	  }

          /*  n=sscanf(ptr,"%[^\n]",string);*/ 
          qq = strchr(ptr,'\n');
	  ptr=qq;
	  
   }

}

void print_LListe ( struct _ListListNodes MyListListNodes, FILE *stats) 
{
      struct _ListNodes      *ptr_Ltrotte;
      struct _ListListNodes  *ptr_LLtrotte, *ptr_shortest_LLNode;
      struct _NodeLoopList  *ptr_NLoopHead, *ptr_NLHtrotte, *ptr_NLHpreced;
      struct _LoopExt       *ptr_LEXHead,   *ptr_LXHtrotte, *ptr_LXHpreced ;
      struct tm ti;
      time_t Sepoch,Bepoch,Eepoch,TopNodeSubmitTime;
      time_t ExeTime, SubDelay, RelativeEnd;
      int hre,min,sec,n, found_loopNode, i, j, shortestLength=256;
      static char sbuffer[10],ebuffer[10], rbuffer[10];
      char *last_ext, *tmp_last_ext, *tmp_Lext, *stats_output = NULL, *avg_output=NULL, *tmp_output;
      char output_buffer[256], tmp_line[256];

      /* Traverse the list to find the top node (shortest nodelength) to get its submit time to do the relative end time calculation */ 
      for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next) {
          if ( ptr_LLtrotte->Nodelength < shortestLength ) {
              ptr_shortest_LLNode=ptr_LLtrotte; 
              shortestLength=ptr_LLtrotte->Nodelength;
          } 
      }
      n=sscanf(ptr_shortest_LLNode->Ptr_LNode->PNode.stime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
      TopNodeSubmitTime=mktime(&ti);

      for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next)
      {
	   /* printf all node having this length */
           for ( ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode ; ptr_Ltrotte != NULL ; ptr_Ltrotte = ptr_Ltrotte->next)
	 {
	       found_loopNode = 0;

	       /* timing */
	       n=sscanf(ptr_Ltrotte->PNode.stime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
	       Sepoch=mktime(&ti);
	       
	       n=sscanf(ptr_Ltrotte->PNode.btime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
	       Bepoch=mktime(&ti);
	       
	       n=sscanf(ptr_Ltrotte->PNode.etime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
	       Eepoch=mktime(&ti);


	       ExeTime = Eepoch - Bepoch;
	       SubDelay = Bepoch - Sepoch; /* check sign */
          RelativeEnd = Eepoch - TopNodeSubmitTime; 

	       memset(ebuffer,'\0', sizeof(ebuffer));
	       memset(sbuffer,'\0', sizeof(sbuffer));
	       memset(rbuffer,'\0', sizeof(rbuffer));

	       strftime (ebuffer,10,"%H:%M:%S",localtime(&ExeTime));
	       strftime (sbuffer,10,"%H:%M:%S",localtime(&SubDelay));
	       strftime (rbuffer,10,"%H:%M:%S",localtime(&RelativeEnd));
          
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
	       stats_output = "stats {";
	       avg_output = "avg {";
	       last_ext="\"\"";
               /*fprintf(stdout,"node=%s Loops=",ptr_NLHtrotte->Node);*/
	       if (read_type != 3) {
	          fprintf(stdout, "/%s\\", ptr_NLHtrotte->Node);
	       }
	       if(read_type == 0 || read_type == 1) {
	          fprintf(stdout, "display_infos {} statuses {");
	       }
               for ( ptr_LXHtrotte = ptr_NLHtrotte->ptr_LoopExt; ptr_LXHtrotte != NULL ; ptr_LXHtrotte = ptr_LXHtrotte->next) {
		  if ( ptr_LXHtrotte->ignoreNode == 0 ) {
		     sprintf(output_buffer, "%s {exectime %s submitdelay %s begintime %s endtime %s deltafromstart %s} ",
			   ptr_LXHtrotte->Lext, ptr_LXHtrotte->exectime, ptr_LXHtrotte->submitdelay,
			   ptr_LXHtrotte->lbtime,  ptr_LXHtrotte->letime, ptr_LXHtrotte->deltafromstart);
		     tmp_output=strdup(stats_output);
		     stats_output=sconcat(tmp_output, output_buffer);
		     strcpy(tmp_line, getNodeAverageLine(ptr_NLHtrotte->Node, ptr_LXHtrotte->Lext));
		     sprintf(output_buffer, "%s {%s} ", ptr_LXHtrotte->Lext, tmp_line);
		     tmp_output=strdup(avg_output);
		     avg_output=sconcat(tmp_output, output_buffer);
		     if (read_type == 0 || read_type == 1) {
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
			      fprintf(stdout,"%s {wait %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->lwtime);
			      break;
			   case 'd':
			      fprintf(stdout,"%s {discret %s} ",ptr_LXHtrotte->Lext, ptr_LXHtrotte->ldtime);
			      break;
			}
		     }
		  }
		  
		  if (stats != NULL) {
		     fprintf(stats, "SEQNODE=/%s:MEMBER=%s:START=%s:END=%s:EXECTIME=%s:SUBMITDELAY=%s:DELTAFROMSTART=%s\n",
			     ptr_NLHtrotte->Node, ptr_LXHtrotte->Lext, ptr_LXHtrotte->lbtime, ptr_LXHtrotte->letime,
			     ptr_LXHtrotte->exectime, ptr_LXHtrotte->submitdelay, ptr_LXHtrotte->deltafromstart );
		  }
	       }
	       sprintf(output_buffer, "}");
	       tmp_output=strdup(stats_output);
	       stats_output=sconcat(tmp_output, output_buffer);
	       tmp_output=strdup(avg_output);
	       avg_output=sconcat(tmp_output, output_buffer);
	       if (read_type == 0 || read_type == 1) {
	          fprintf(stdout, "} current latest\\");
	       }
	       if (read_type == 0 || read_type == 2) {
		  fprintf(stdout, "%s %s stats_info {}\\", stats_output, avg_output);  
	       }
      }
      
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
   
   SeqUtil_TRACE("logreader parsing averages on exp: %s for datestamp: %s\n", exp, datestamp);
   
   tmp_file_path=sconcat("/stats/", char_datestamp);
   avg_file_path=sconcat(tmp_file_path, "_avg");
   full_path=sconcat(exp, avg_file_path);
   
   SeqUtil_TRACE("logreader collecting averages in file %s\n", full_path);
   
   f = fopen(full_path, "r");
   if(f != NULL) {
      if(getStats(f) != 0){
         SeqUtil_TRACE("logreader error while parsing the following avg file:\n");
         SeqUtil_TRACE("%s\n", full_path);
      }
   } else {
      SeqUtil_TRACE("logreader cannot access file %s\n", full_path);
   }
}

/*outputs a node-specific line formatted to fit in the tsv structure for logreader.tcl*/
char *getNodeAverageLine(char *node, char *member){
   char line[256];
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
      time_tmpptr=node_tmpptr->times;
      snprintf(line, 256, "exectime %s submitdelay %s begintime %s endtime %s deltafromstart %s", time_tmpptr->exectime, time_tmpptr->submitdelay,
          time_tmpptr->start, time_tmpptr->end, time_tmpptr->deltafromstart);
   } else {
      return "";
   }
   
   return strdup(line);
}

/*used to generate averages from stats*/
void computeAverage(char *exp, char *datestamp){
   char *stats_file_path = NULL;
   char *full_path = NULL;
   char char_datestamp[15];
   char prev[15];
   int i, count=0;
   
   FILE *f=NULL;
   prev[14] = '\0';
   char_datestamp[14] = '\0';
   snprintf(char_datestamp, 15, "%s", datestamp);
   
   SeqUtil_TRACE("logreader computing averages on exp: %s for datestamp: %s since last %d days\n", exp, datestamp, stats_days);
   
   for (i=0; i < stats_days; i++){
      stats_file_path = sconcat("/stats/", char_datestamp);
      full_path = sconcat(exp, stats_file_path);
      
      SeqUtil_TRACE("logreader collecting stats for datestamp %s in file %s\n", char_datestamp, full_path);
      
      f = fopen(full_path, "r");
      if (f != NULL) {
         if(getStats(f) != 0){
            SeqUtil_TRACE("logreader error while parsing the following stats file:\n");
            SeqUtil_TRACE("%s\n", full_path);
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
   processStats(exp, datestamp);
}

/*translate stats file in linked list*/
int getStats(FILE *_stats){
   char tmpline[1024];
   
   char *tmp_node, *tmp_member, *tmp_btime, *tmp_etime, 
       *tmp_exectime, *tmp_submitdelay, *tmp_deltafromstart;
   
   if(_stats == NULL) {
      return -1;
   }
       
   /*process lines one by one*/
   while ((fgets(tmpline, sizeof(tmpline), _stats) != NULL) && (strstr(tmpline, "SEQNODE") != NULL)) {
      if(parseStatsLine(tmpline, tmp_node, tmp_member, tmp_btime, tmp_etime, 
          tmp_exectime, tmp_submitdelay, tmp_deltafromstart) != 0) {

         SeqUtil_TRACE("logreader parsing error at the following stats line:\n");
         SeqUtil_TRACE("%s\n", tmpline);
      
      }
   }
   
   return 0;
}

/*parse stats line and add stats node*/
int parseStatsLine(char line[1024], char *node, char *member, char *btime, char *etime, char *exectime, char *submitdelay, char *deltafromstart){
   char *tmp_node, *tmp_member, *tmp_btime, *tmp_etime, 
   *tmp_exectime, *tmp_submitdelay, *tmp_deltafromstart;
   
   char *nodeptr, *memberptr, *btimeptr, *etimeptr, *exectimeptr,
   *submitdelayptr, *deltafromstartptr;
   
   int nodelen, memberlen, btimelen, etimelen, exectimelen,
   submitdelaylen, deltafromstartlen;
   
   if(strstr(line, "SEQNODE=") == NULL || strstr(line, "MEMBER=") == NULL ||
      strstr(line, "START=") == NULL || strstr(line, "END=") == NULL ||
      strstr(line, "EXECTIME=") == NULL || strstr(line, "SUBMITDELAY=") == NULL ||
      strstr(line, "DELTAFROMSTART=") == NULL) {
      return -1;
   }
   
   nodeptr=strstr(line, "SEQNODE=")+8;
   memberptr=strstr(line, "MEMBER=")+7;
   btimeptr=strstr(line, "START=")+6;
   etimeptr=strstr(line, "END=")+4;
   exectimeptr=strstr(line, "EXECTIME=")+9;
   submitdelayptr=strstr(line, "SUBMITDELAY=")+12;
   deltafromstartptr=strstr(line, "DELTAFROMSTART=")+15;
   
   deltafromstartlen=strlen(deltafromstartptr)-1;
   submitdelaylen=strlen(submitdelayptr)-deltafromstartlen-15-1-1;
   exectimelen=strlen(exectimeptr)-submitdelaylen-deltafromstartlen-15-12-1-1-1;
   etimelen=strlen(etimeptr)-exectimelen-submitdelaylen-deltafromstartlen-15-12-9-1-1-1-1;
   btimelen=strlen(btimeptr)-etimelen-exectimelen-submitdelaylen-deltafromstartlen-15-12-9-4-1-1-1-1-1;
   memberlen=strlen(memberptr)-btimelen-etimelen-exectimelen-submitdelaylen-deltafromstartlen-15-12-9-4-6-1-1-1-1-1-1;
   nodelen=strlen(nodeptr)-memberlen-btimelen-etimelen-exectimelen-submitdelaylen-deltafromstartlen-15-12-9-4-6-7-1-1-1-1-1-1-1;
   
   node=malloc(nodelen+1);
   member=malloc(memberlen+1);
   btime=malloc(btimelen+1);
   etime=malloc(etimelen+1);
   exectime=malloc(exectimelen+1);
   submitdelay=malloc(submitdelaylen+1);
   deltafromstart=malloc(deltafromstartlen+1);
   
   if(node == NULL || member == NULL || btime == NULL || etime == NULL ||
      exectime == NULL || submitdelay == NULL || deltafromstart == NULL) {
      return -1;
   }
   
   snprintf(node, nodelen+1, "%s", nodeptr);
   snprintf(member, memberlen+1, "%s", memberptr);
   snprintf(btime, btimelen+1, "%s", btimeptr);
   snprintf(etime, etimelen+1, "%s", etimeptr);
   snprintf(exectime, exectimelen+1, "%s", exectimeptr);
   snprintf(submitdelay, submitdelaylen+1, "%s", submitdelayptr);
   snprintf(deltafromstart, deltafromstartlen+1, "%s", deltafromstartptr);
   
   return addStatsNode(node, member, btime, etime, exectime, submitdelay, deltafromstart);
}

/*add stat node to linked list plus set root node if the list is empty*/
int addStatsNode(char *node, char *member, char *btime, char *etime, char *exectime, char *submitdelay, char *deltafromstart){
   StatsNode *stats_node_ptr;
   StatsNode *node_tmpptr;
   PastTimes *past_times_ptr = (PastTimes*)malloc(sizeof(PastTimes));
   PastTimes *time_tmpptr;
   
   if(past_times_ptr == NULL){
      return -1;
   }
   
   past_times_ptr->start = btime;
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
   
   SeqUtil_TRACE("logreader addStatsNode node:%s member:%s\n", node, member);
   
   return 0;
}

/*final step to the computation of averages; it calculates from the linked list and outputs results to avg file*/
int processStats(char *exp, char *datestamp){
   StatsNode *node_tmpptr;
   PastTimes *time_tmpptr;
   char *start=NULL, *end=NULL, *submitdelay=NULL, *exectime=NULL, *deltafromstart=NULL;
   int int_start[30], int_end[30], int_submitdelay[30], int_exectime[30], int_deltafromstart[30];
   char *avg_path=NULL, *buffer_1=NULL, *buffer_2=NULL;
   int avg_counter, iter_counter,i,truncate_amount=0;
   FILE *avg=NULL;
   
   if(read_type==3){
      buffer_1=sconcat(exp, "/stats/");
      buffer_2=sconcat(buffer_1, datestamp);
      avg_path=sconcat(buffer_2, "_avg");
   
      avg=fopen(avg_path, "w");
      if(avg == NULL) {
         return -1;
      }
   }
   
   if(rootStatsNode == NULL) {
      return 0;
   }
   node_tmpptr=rootStatsNode;
   
   while(node_tmpptr != NULL){
      time_tmpptr=node_tmpptr->times;
      iter_counter=0;
      for(i=0; i<30; ++i) {
        int_start[i]=0;
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
         int_start[iter_counter] = charToSeconds(time_tmpptr->start);
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
      start=secondsToChar(SeqUtil_basicTruncatedMean(&int_start, avg_counter, truncate_amount));
      end=secondsToChar(SeqUtil_basicTruncatedMean(&int_end, avg_counter, truncate_amount));
      submitdelay=secondsToChar(SeqUtil_basicTruncatedMean(&int_submitdelay, avg_counter,truncate_amount));
      exectime=secondsToChar(SeqUtil_basicTruncatedMean(&int_exectime, avg_counter,truncate_amount));
      deltafromstart=secondsToChar(SeqUtil_basicTruncatedMean(&int_deltafromstart, avg_counter,truncate_amount));
 
      if(read_type==3){
         fprintf(avg, "SEQNODE=%s:MEMBER=%s:START=%s:END=%s:EXECTIME=%s:SUBMITDELAY=%s:DELTAFROMSTART=%s\n",
             node_tmpptr->node, node_tmpptr->member, start, end, exectime, submitdelay, deltafromstart);
      }
      
      node_tmpptr=node_tmpptr->next;
   }
   
   if(avg != NULL){
      fclose(avg);
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

/*ATTENTION: pas utilisee, car produit une incertitude de stats_days secondes
 * Ecrite en avril 2015, a retirer eventuellement*/
/*returns average + toAdd/counter */
char *addToAverage(char *_toAdd, char *average, int counter){
   char buffer[9];
   char add_hour[3], add_minute[3], add_second[3];
   int add_total_seconds;
   char average_hour[3], average_minute[3], average_second[3];
   int average_total_seconds;
   int total_seconds, result_hour, result_minute, result_second;
   char *toAdd=_toAdd;
   
   if(strstr(_toAdd, ".") != NULL) {
      toAdd=strstr(_toAdd, ".")+1;
   }
   
   add_hour[0]=toAdd[0];
   add_hour[1]=toAdd[1];
   add_hour[2]='\0';
   add_minute[0]=toAdd[3];
   add_minute[1]=toAdd[4];
   add_minute[2]='\0';
   add_second[0]=toAdd[6];
   add_second[1]=toAdd[7];
   add_second[2]='\0';
   add_total_seconds=(atoi(add_second)+(atoi(add_minute)*60)+(atoi(add_hour)*60*60));
   
   average_hour[0]=average[0];
   average_hour[1]=average[1];
   average_hour[2]='\0';
   average_minute[0]=average[3];
   average_minute[1]=average[4];
   average_minute[2]='\0';
   average_second[0]=average[6];
   average_second[1]=average[7];
   average_second[2]='\0';
   average_total_seconds=(atoi(average_second)+(atoi(average_minute)*60)+(atoi(average_hour)*3600));
   
   total_seconds=(add_total_seconds/counter)+average_total_seconds;
   result_hour=(total_seconds/3600);
   result_minute=(total_seconds%3600)/60;
   result_second=(total_seconds%3600)%60;
   
   buffer[8]='\0';
   snprintf(buffer, 9, "%02i:%02i:%02i", result_hour, result_minute, result_second);
   
   return strdup(buffer);
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
void reset_node (char *node, char *ext) {
   struct _ListNodes      *ptr_Ltrotte;
   struct _ListListNodes  *ptr_LLtrotte;
   struct _ListNodes      *tmp_prev_list;
   
   for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next) {
      ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode;
      while (ptr_Ltrotte != NULL){
         tmp_prev_list=ptr_Ltrotte;
         ptr_Ltrotte = ptr_Ltrotte->next;
         
         if (strncmp(node, tmp_prev_list->PNode.TNode, strlen(node)) == 0 && strncmp(ext, tmp_prev_list->PNode.loop, strlen(ext)) == 0) {
            /*delete_node(tmp_prev_list, ptr_LLtrotte);*/
            tmp_prev_list->PNode.ignoreNode=1;
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
