#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include "logreader.h"
#include "SeqUtil.h"

/*log_node represents a line in a log file*/
typedef struct log_node {
  char *timestamp;
  char *seqnode;
  char *msgtype;
  char *seqmsg;
  char *seqloop;
  
  struct log_node *next;
} lognode;

/*keep these to retrieve the list*/
lognode *root_node = NULL;
lognode *current_node = NULL;

/*add node to linked list plus set root node if the list is empty*/
int addNode ( char *timestamp, char *seqnode, char *msgtype, char *seqloop, char *seqmsg ) {
   
  lognode *tmpptr = (lognode*)malloc(sizeof(lognode));
  if (tmpptr == NULL || timestamp == NULL) {
    return -1;
  }
  
  tmpptr->timestamp = timestamp;
  tmpptr->seqnode = seqnode;
  tmpptr->msgtype = msgtype;
  tmpptr->seqloop = seqloop;
  tmpptr->seqmsg = seqmsg;
  
  tmpptr->next=NULL;
  if (current_node != NULL) {
    current_node->next=tmpptr;
  }
  current_node=tmpptr;
  if (root_node == NULL) {
    root_node = tmpptr;
  }
  
  return 0;
}

/*remove old information of exp nodes from linked list,*/
/*not used on root exp nodes*/
int removeOldNodes (lognode *lastsubmit) {
  
  lognode *remove_iterator;
  lognode *last_known;
  
  if ((root_node == NULL) || (lastsubmit == NULL)) {
    return -1;
  }
  
  remove_iterator=root_node;
  
  while ((remove_iterator != NULL) && (remove_iterator != lastsubmit)) {
    last_known=remove_iterator;
    remove_iterator=remove_iterator->next;
    
    if ((strcmp(last_known->seqnode, lastsubmit->seqnode) == 0) && (strcmp(last_known->seqloop, lastsubmit->seqloop) == 0) && (strcmp(last_known->msgtype, "abort") != 0) && 
       (strcmp(last_known->msgtype, "info") != 0) && (strcmp(last_known->msgtype, "event") != 0) && (strcmp(last_known->msgtype, "init") != 0)) {
       SeqUtil_TRACE("removeOldNodes: removing %s %s\n", last_known->seqnode, last_known->seqloop);
       deleteNode(last_known);
    }
  }  
  return 0;
}

/*delete a node from the list*/
int deleteNode (lognode *node) {
  
  lognode *todelete=NULL;
  lognode *prev=NULL;
  
  if ((root_node == NULL) || (node == NULL)) {
    return -1;
  }
  
  todelete=root_node;
  while((todelete != node) && (todelete->next != NULL)) {
    prev=todelete;
    todelete = todelete->next;
  }
  if(prev != NULL) {
    prev->next=todelete->next;
  }
  if (todelete == current_node) {
    prev->next=NULL;
    current_node=prev;
  } else if (todelete == root_node) {
    root_node = todelete->next;
  }
  
  free(todelete);
  todelete=NULL;
  
  return 0;
}

/*when the list is done, create the final output file*/
/*note that the linked list is deleted*/
int processList(const char* outputfile) {
  
  FILE *outFile;
  lognode *slowiterator=root_node, *fastiterator=root_node, *lastiterator; 
  char* tmpoutput=NULL;
  /*char tmpoutputline[1024];*/
  
  if(root_node == NULL) {
    return -1;
  }
  
  /*determine where to send the output*/
  if ( outputfile == NULL ) {
    tmpoutput="stdout";
  } else {
    outFile = fopen (outputfile,"w");
    if (outFile != NULL) {
      tmpoutput="outFile";
    }
    else {
      SeqUtil_TRACE("logreader: cannot open output file %s, sending output to stdout\n", outputfile);
      tmpoutput="stdout";
    }
  }
  
  SeqUtil_TRACE("logreader: processing log list, first node: %s\n", root_node->seqnode);
  
  /*write formatted log linked list to output stream*/
  while (slowiterator != NULL) {
    while (fastiterator != NULL) {
      if (strcmp(slowiterator->seqnode, fastiterator->seqnode) == 0) {
        if (strcmp(tmpoutput, "stdout") == 0) {
            fprintf(stdout, "%s!~!%s!~!%s!~!%s!~!%s\n", fastiterator->timestamp, fastiterator->seqnode, fastiterator->msgtype, fastiterator->seqloop, fastiterator->seqmsg);
        } else { 
            fprintf(outFile, "%s!~!%s!~!%s!~!%s!~!%s\n", fastiterator->timestamp, fastiterator->seqnode, fastiterator->msgtype, fastiterator->seqloop, fastiterator->seqmsg);
        }
      }
      lastiterator=fastiterator;
      fastiterator=fastiterator->next;
      if ((lastiterator != slowiterator) && (strcmp(slowiterator->seqnode, lastiterator->seqnode) == 0)) {
        deleteNode(lastiterator);
      }
    }
  
    slowiterator=slowiterator->next;
    fastiterator=slowiterator;
      
  }
  
  if (tmpoutput=="outFile")
    fclose(outFile);
  
  return 0;
}

int logreader ( const char* inputfile, const char* outputfile, char* node, char* loops, char* signal, int start_offset, int end_offset) {

  FILE *inFile;
  char *whichnode=NULL;
  char tmpline[1024];
  char *tmp_seqnode=NULL, *tmp_seqmsg=NULL, *tmp_msgtype=NULL, *tmp_seqloop=NULL, *tmp_timestamp=NULL;
  char *seqnodeptr=NULL, *seqmsgptr=NULL, *msgtypeptr=NULL, *seqloopptr=NULL, *timestampptr=NULL;
  int seqnodelen, seqmsglen, msgtypelen, seqlooplen, timestamplen;
  int currentline;
  char *tampon;
  
  SeqUtil_TRACE("logreader: inputfile=%s, outputfile=%s, node=%s, loop extension=%s, signal=%s, start offset=%d, end offset = %d\n",
   inputfile, outputfile, node, loops, signal, start_offset, end_offset);
  
  /*open inputfile in binary mode for easy tcl to C seek conversion*/
  inFile = fopen (inputfile,"rb");
  if (inFile == NULL) {
    raiseError("logreader: ERROR cannot open input file %s, exiting\n", inputfile);
  }
  
  if (node != NULL) {
    whichnode=node;
  } else {
    whichnode="all";
  }
  
  currentline = start_offset;
  
  /*position the stream to the start_offset*/
  if ((start_offset != 0) && (fseek( inFile , start_offset , SEEK_SET ) != 0)) {
    SeqUtil_TRACE("logreader: ERROR while setting the position indicator in input log file %s, start offset ignored\n", inputfile);
  }
  
  /*process lines one by one*/
  while ((fgets(tmpline, sizeof(tmpline), inFile) != NULL) && ((end_offset == 0) || 
     (currentline <= end_offset)) && (strstr(tmpline, "TIMESTAMP") != NULL)) {
    
    tmp_seqmsg=NULL;
    tmp_seqloop=NULL;
    tmp_msgtype=NULL;
    tmp_seqnode=NULL;
    tmp_timestamp=NULL;
    
    seqmsgptr=strstr(tmpline, "SEQMSG=")+7;
    seqloopptr=strstr(tmpline, "SEQLOOP=")+8;
    msgtypeptr=strstr(tmpline, "MSGTYPE=")+8;
    seqnodeptr=strstr(tmpline, "SEQNODE=")+8;
    timestampptr=strstr(tmpline, "TIMESTAMP=")+10;
    
    seqmsglen=strlen(seqmsgptr)-1;
    seqlooplen=strlen(seqloopptr)-seqmsglen-9;
    msgtypelen=strlen(msgtypeptr)-seqmsglen-seqlooplen-18;
    seqnodelen=strlen(seqnodeptr)-msgtypelen-seqmsglen-seqlooplen-27;
    timestamplen=strlen(timestampptr)-seqnodelen-msgtypelen-seqmsglen-seqlooplen-36;
    
    tmp_seqmsg=malloc(seqmsglen+1);
    tmp_seqloop=malloc(seqlooplen+1);
    tmp_msgtype=malloc(msgtypelen+1);
    tmp_seqnode=malloc(seqnodelen+1);
    tmp_timestamp=malloc(timestamplen+1);
    
    snprintf(tmp_seqmsg, seqmsglen+1, "%s", seqmsgptr);
    snprintf(tmp_seqloop, seqlooplen+1, "%s", seqloopptr);
    snprintf(tmp_msgtype, msgtypelen+1, "%s", msgtypeptr);
    snprintf(tmp_seqnode, seqnodelen+1, "%s", seqnodeptr);
    snprintf(tmp_timestamp, timestamplen+1, "%s", timestampptr);
    
    if (((whichnode == "all") || (strcmp(tmp_seqnode, whichnode) == 0) || ((strchr(seqnodeptr+1, '/') == NULL) && (whichnode == "/"))) &&
       ((signal == NULL) || (strcmp(tmp_msgtype, signal) == 0)) &&
       ((loops == NULL) || (strcmp(tmp_seqloop, loops) == 0))) {
      addNode(tmp_timestamp, tmp_seqnode, tmp_msgtype, tmp_seqloop, tmp_seqmsg);
      /*SeqUtil_TRACE("timestamp=%s, node=%s, msgtype=%s, seqloop=%s, seqmsg=%s", current_node->timestamp, current_node->seqnode, current_node->msgtype, current_node->seqloop, current_node->seqmsg);*/
      /* if ((strncmp(current_node->msgtype, "submit", 6) == 0) && (strchr((current_node->seqnode)+1, '/') != NULL)) { */
      if (strchr((current_node->seqnode)+1, '/') != NULL) {
         removeOldNodes(current_node);
      }
    }
    
    currentline = currentline + 1;
  }
  
  SeqUtil_TRACE("logreader: creating output from log file %s\n", inputfile);
  if (root_node != NULL) {
    if (processList(outputfile) == -1) {
      raiseError("logreader: ERROR while processing the log file %s, exiting\n", inputfile);
    }
  }
   
  if (inFile != NULL)
    fclose(inFile);
  
  return 0;
}
