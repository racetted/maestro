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
	   //fprintf(stdout,"First Time\n");
           MyListListNodes.Nodelength=len;
	   if ( (ptr_lhead=(struct _ListNodes *) malloc(sizeof(struct _ListNodes))) != NULL ) {
	              strcpy(ptr_lhead->PNode.Node,ComposedNode);
	              strcpy(ptr_lhead->PNode.TNode,node);
		      strcpy(ptr_lhead->PNode.loop,loop);
		      switch ( S ) 
		      {
		        case 'a':
			         //fprintf(stdout,"atime=%s\n",atime);
	                         strcpy(ptr_lhead->PNode.atime,atime);
				 break;
		        case 'b':
			         //fprintf(stdout,"btime=%s\n",btime);
	                         strcpy(ptr_lhead->PNode.btime,btime);
				 break;
		        case 'e':
			         //fprintf(stdout,"etime=%s\n",etime);
	                         strcpy(ptr_lhead->PNode.etime,etime);
				 break;
		        case 'i':
			         //fprintf(stdout,"itime=%s\n",itime);
	                         strcpy(ptr_lhead->PNode.itime,itime);
				 break;
		        case 's':
			         //fprintf(stdout,"atime=%s\n",stime);
	                         strcpy(ptr_lhead->PNode.stime,stime);
				 break;
			case 'w':
			         //fprintf(stdout,"atime=%s\n",stime);
			         strcpy(ptr_lhead->PNode.wtime,wtime);
				 break;
			case 'd':
			         //fprintf(stdout,"atime=%s\n",stime);
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
       //fprintf(stdout,"Not First Time\n");
       for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next)
       { 
           if ( ptr_LLtrotte->Nodelength == len ) {
	        found_len=1;
                /* found same length, find  exact node if any  */
                for ( ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode; ptr_Ltrotte != NULL ; ptr_Ltrotte = ptr_Ltrotte->next) {
		               //fprintf(stdout,"Compring Pnode=%s with node=%s\n",ptr_Ltrotte->PNode.Node,node);
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

       //fprintf(stdout,"did not find  nodes with same length\n");

       if ( found_len == 0 && (ptr_LLpreced->next = (struct _ListListNodes *) malloc(sizeof(struct _ListListNodes ))) != NULL ) {
	       ptr_LLpreced->next->Nodelength=len;
	       ptr_LLpreced->next->next= NULL;
               //fprintf(stdout,"Inserting other node with diff length\n");
	       
	       if ( (ptr_Ltrotte = (struct _ListNodes *) malloc(sizeof(struct _ListNodes ))) != NULL ) {
                      //fprintf(stdout,"Inserting other node with new length\n");
	               strcpy(ptr_Ltrotte->PNode.Node,ComposedNode);
	               strcpy(ptr_Ltrotte->PNode.TNode,node);
	               strcpy(ptr_Ltrotte->PNode.loop,loop);
		      
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

void read_file (char *base , FILE *log)
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
  
           /* (char S, char *node, char *loop, char *stime, char *btime, char *etime , char *atime , char *itime, char * exectime, char * submitdelay, int len )  */
          //fprintf(log,"dstamp=%s node=%s signal=%s loop=%s\n",dstamp,&node[9],signal,&loop[8]);  
	  
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


void print_LListe ( struct _ListListNodes MyListListNodes , FILE *log) 
{
      struct _ListNodes      *ptr_Ltrotte;
      struct _ListListNodes  *ptr_LLtrotte;
      struct _NodeLoopList  *ptr_NLoopHead, *ptr_NLHtrotte, *ptr_NLHpreced;
      struct _LoopExt       *ptr_LEXHead,   *ptr_LXHtrotte, *ptr_LXHpreced ;
      struct tm ti;
      time_t Sepoch,Bepoch,Eepoch;
      time_t ExeTime, SubDelay;
      int hre,min,sec,n, found_loopNode, i, j;
      static char sbuffer[10],ebuffer[10];
      char *last_ext, *tmp_last_ext, *tmp_Lext, *stats_output = NULL, *tmp_output;
      char output_buffer[164];

      for ( ptr_LLtrotte = &MyListListNodes; ptr_LLtrotte != NULL ; ptr_LLtrotte = ptr_LLtrotte->next)
      {
	   /* printf all node having this length */
           for ( ptr_Ltrotte = ptr_LLtrotte->Ptr_LNode ; ptr_Ltrotte != NULL ; ptr_Ltrotte = ptr_Ltrotte->next)
	   {
	      if(ptr_Ltrotte->PNode.ignoreNode == 0){
		  found_loopNode = 0;

	          /* timing */
                  n=sscanf(ptr_Ltrotte->PNode.stime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
		  Sepoch=mktime(&ti);
                  
		  n=sscanf(ptr_Ltrotte->PNode.btime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
		  Bepoch=mktime(&ti);
                  
		  n=sscanf(ptr_Ltrotte->PNode.etime,"%4d%2d%2d.%2d:%2d:%2d", &ti.tm_year, &ti.tm_mon, &ti.tm_mday, &ti.tm_hour, &ti.tm_min, &ti.tm_sec);
		  Eepoch=mktime(&ti);


                  ExeTime = Eepoch - Bepoch;
		  SubDelay = Bepoch - Sepoch; /* chek sign */

                  memset(ebuffer,'\0', sizeof(ebuffer));
                  memset(sbuffer,'\0', sizeof(sbuffer));

                  strftime (ebuffer,10,"%H:%M:%S",localtime(&ExeTime));
                  strftime (sbuffer,10,"%H:%M:%S",localtime(&SubDelay));
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
					    strcpy(ptr_LEXHead->ldtime,ptr_Ltrotte->PNode.dtime);
					    strcpy(ptr_LEXHead->exectime, ebuffer);
					    strcpy(ptr_LEXHead->submitdelay, sbuffer);
					    ptr_LEXHead->LastAction=ptr_Ltrotte->PNode.LastAction;
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
					  ptr_LXHpreced->next->LastAction=ptr_Ltrotte->PNode.LastAction;
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
					      ptr_LEXHead->LastAction=ptr_Ltrotte->PNode.LastAction;
					      ptr_LEXHead->next = NULL;
				       }
			      }
	                 }
	      }
	   }
      }

      /* print loop task */
      for ( ptr_NLHtrotte = &MyNodeLoopList; ptr_NLHtrotte != NULL ; ptr_NLHtrotte = ptr_NLHtrotte->next) {
	       stats_output = "stats {";
	       last_ext="\"\"";
               /*fprintf(stdout,"node=%s Loops=",ptr_NLHtrotte->Node);*/
	       fprintf(stdout, "/%s\\", ptr_NLHtrotte->Node);
	       if(read_type == 0 || read_type == 1) {
	          fprintf(stdout, "display_infos {} statuses {");
	       }
               for ( ptr_LXHtrotte = ptr_NLHtrotte->ptr_LoopExt; ptr_LXHtrotte != NULL ; ptr_LXHtrotte = ptr_LXHtrotte->next) {
		  sprintf(output_buffer, "%s {exectime %s submitdelay %s begintime %s endtime %s} ", 
			  ptr_LXHtrotte->Lext, ptr_LXHtrotte->exectime, ptr_LXHtrotte->submitdelay,
			  ptr_LXHtrotte->lbtime,  ptr_LXHtrotte->letime);
		  tmp_output=strdup(stats_output);
		  stats_output=sconcat(tmp_output, output_buffer);
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
		  /*if(strchr(ptr_LXHtrotte->Lext,'+') != NULL) {
		     tmp_Lext = strdup(ptr_LXHtrotte->Lext);
		     tmp_last_ext = strdup(last_ext);
		     for (i=0; tmp_Lext[i]; tmp_Lext[i]=='+' ? i++ : *tmp_Lext++);
		     for (j=0; tmp_last_ext[j]; tmp_last_ext[j]=='+' ? j++ : *tmp_last_ext++);
		     if(i >= j && strcmp(ptr_LXHtrotte->Lext,"all") != 0 && strcmp(ptr_LXHtrotte->Lext,"null") != 0) {
			last_ext=strdup(ptr_LXHtrotte->Lext);
		     }
		  } else if (strcmp(ptr_LXHtrotte->Lext,"all") != 0 && strcmp(ptr_LXHtrotte->Lext,"null") != 0){
		  if (strcmp(ptr_LXHtrotte->Lext,"all") != 0 && strcmp(ptr_LXHtrotte->Lext,"null") != 0 && strcmp(ptr_LXHtrotte->Lext,"") != 0){
		     last_ext=strdup(ptr_LXHtrotte->Lext);
		  }*/
	       }
	       sprintf(output_buffer, "}");
	       tmp_output=strdup(stats_output);
	       stats_output=sconcat(tmp_output, output_buffer);
	       if (read_type == 0 || read_type == 1) {
	          fprintf(stdout, "} current latest\\");
	       }
	       if (read_type == 0 || read_type == 2) {
		  fprintf(stdout, "%s stats_info {}\\", stats_output);  
	       }
      }
      
}

/*edited from http://www.c4learn.com/c-programming/c-concating-strings-dynamic-allocation/*/
char * sconcat(char *ptr1,char *ptr2){
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

/*in case of init*/
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

/*not used for now (creates memfault btw), ignoreNode attribute instead*/
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