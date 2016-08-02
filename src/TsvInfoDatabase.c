#include <stdio.h>
#include <stdlib.h>

#include "SeqNode.h"
#include "SeqUtil.h"
#include "SeqLoopsUtil.h"
#include "SeqNodeCensus.h"
/* #include "nodeinfo.h" */
#include "ResourceVisitor.h"

static const char *indent = "    ";
static const char *doubleIndent = "        ";
/********************************************************************************
 * Creates the batch resourse keylist:
 * Ex: "{resources {{CATCHUP 4} {CPU 2} {QUEUE null} {MPI 8} {MEMORY 500M} {WALLCLOCK 5}}}
 * The The braces are very important for the TCL Thread package to be able to
 * correctly parse the output.
 * NOTE: Notice the space after each "{key value} " pair except the last one.
 * This is part of the keyed list syntax and must be respected.
 * A space is possible after the last pair, but as a matter of consistency,
 * since TCL does funny stuff with extra spaces in certain contexts, they should
 * always be avoided.
********************************************************************************/
void resource_keylist(SeqNodeDataPtr ndp, FILE * fp, int human_readable)
{

   if( human_readable ){
      fprintf(fp, "    .resources\n");
      fprintf(fp, "%s.CATCHUP   = %d\n", doubleIndent, ndp->catchup);
      fprintf(fp, "%s.CPU       = %s\n", doubleIndent, ndp->cpu);
      fprintf(fp, "%s.QUEUE     = %s\n", doubleIndent, ndp->queue);
      fprintf(fp, "%s.MPI       = %d\n", doubleIndent, ndp->mpi);
      fprintf(fp, "%s.MEMORY    = %s\n", doubleIndent, ndp->memory);
      fprintf(fp, "%s.WALLCLOCK = %d\n", doubleIndent, ndp->wallclock);
   } else {
      /*
       * {resources {{CATCHUP 1} {CPU 8} ... {WALLCLOCK 5}}}
       */
      fprintf(fp,"{resources ");

      fprintf(fp, "{");
      fprintf(fp, "{CATCHUP %d} ",ndp->catchup);
      fprintf(fp, "{CPU %s} ",ndp->cpu);
      fprintf(fp, "{QUEUE %s} ",ndp->queue);
      fprintf(fp, "{MPI %d} ",ndp->mpi);
      fprintf(fp, "{MEMORY %s} ",ndp->memory);
      fprintf(fp, "{WALLCLOCK %d}",ndp->wallclock);
      fprintf(fp, "}");

      fprintf(fp, "}");
   }
}

/********************************************************************************
 * Creates the loop keylist. {loop {{START 1} {END 2} {STEP 3} {SET 4}}} The
 * bracing is very important.
 * NOTE: Notice the space after each "{key value} " pair except the last one.
 * This is part of the keyed list syntax and must be respected.
 * A space is possible after the last pair, but as a matter of consistency,
 * since TCL does funny stuff with extra spaces in certain contexts, they should
 * always be avoided.
********************************************************************************/
void loop_keylist(SeqNodeDataPtr ndp, FILE *fp, int human_readable)
{
   char *start = SeqLoops_getLoopAttribute( ndp->data, "START" ),
        *end = SeqLoops_getLoopAttribute( ndp->data, "END" ),
        *step = SeqLoops_getLoopAttribute( ndp->data, "STEP" ),
        *set = SeqLoops_getLoopAttribute( ndp->data, "SET" ),
        *expression = SeqLoops_getLoopAttribute( ndp->data, "EXPRESSION" );

   if( human_readable ){
      fprintf(fp, "%s.loop\n",indent);
      if( expression == NULL ){
         fprintf(fp, "%s.START = %s\n",doubleIndent, start);
         fprintf(fp, "%s.END   = %s\n",doubleIndent, end);
         fprintf(fp, "%s.STEP  = %s\n",doubleIndent, step);
         fprintf(fp, "%s.SET   = %s\n",doubleIndent, set);
      } else {
         fprintf(fp, "%s.EXPRESSION = %s\n",doubleIndent, expression);
      }
   } else {
      /*
       * {loop {{START 8} {END 18} {STEP 2} {SET 2}}}
       */

#define OPTION 4

      fprintf(fp, "{loop {");
#if OPTION == 1 || OPTION == 2
      if( expression == NULL ){
         fprintf(fp, "{START %s} ", start );
         fprintf(fp, "{END %s} ", end );
         fprintf(fp, "{STEP %s} ", step );
         fprintf(fp, "{SET %s}", set );
      } else {
         fprintf(fp, "{EXPRESSION %s}", expression );
      }
#elif OPTION == 3 || OPTION == 4
      fprintf(fp, "{START %s} ", start );
      fprintf(fp, "{END %s} ", end );
      fprintf(fp, "{STEP %s} ", step );
      fprintf(fp, "{SET %s} ", set ); /* Notice the space that is not there in options 1 and 2 */
      fprintf(fp, "{EXPRESSION %s}", expression );
#endif
      fprintf(fp, "}}");

   }

   free(start);
   free(end);
   free(step);
   free(set);
   free(expression);
}

/********************************************************************************
 * NOT USED YED: THis is an example of a function that could be used for a
 * general purpose (not necessarily dependencies).  If we want to add another
 * key to the big keyed list, this can be used as a template.
 * NOTE: Notice the space after each "{key value} " pair except the last one.
 * This is part of the keyed list syntax and must be respected.
 * A space is possible after the last pair, but as a matter of consistency,
 * since TCL does funny stuff with extra spaces in certain contexts, they should
 * always be avoided.
********************************************************************************/
void dependency_keylist(SeqNodeDataPtr ndp, FILE *fp, int human_readable)
{
   if( human_readable ){
      fprintf(fp, "%s.depends\n",indent);
      fprintf(fp, "%s.dpename1 = %s\n", doubleIndent, "depvalue1");
      fprintf(fp, "%s.dpename2 = %s\n", doubleIndent, "depvalue2");
   } else {
      fprintf(fp, "{depends {");
      fprintf(fp, "{%s %s} ","depname1","depvalue1");
      fprintf(fp, "{%s %s}","depname1","depvalue1");
      fprintf(fp, "}}");
   }
}

void node_to_keylist(SeqNodeDataPtr ndp,FILE *fp, int human_readable)
{
   if( human_readable ){
      fprintf(fp, "%s\n", ndp->name);
      resource_keylist(ndp,fp, human_readable);

      /* Template for adding info to the human readable version of the TCL-ready
       * output:
       *
       * dependency_keylist(ndp,fp, human_readable);
       */

      if( ndp->type == Loop ){
         loop_keylist(ndp,fp,human_readable);
      }
   } else {
      fprintf(fp, "%s {", ndp->name);

      resource_keylist(ndp,fp,human_readable);

      /*
       * This is a tmplate for adding new keys to the master keyed list: a space
       * character, and a call to a function following the template of the other
       * functions. The space can't be part of the function because the the
       * first one would have a superfluous space: 
       * /module/node { resources {{CPU 1} ... }}
       * and the subfunctions don't know if they are called first or not.
       *
       * fprintf(fp, " ");
       * dependency_keylist(ndp, fp,human_readable);
       */

#if OPTION == 1 || OPTION == 4
      if( ndp->type == Loop ){
         fprintf(fp, " ");
         loop_keylist(ndp, fp, human_readable);
      }
#elif OPTION == 2 || OPTION == 3
      /* Always print, but put either {{START 1} {END 2} {STEP 2} {SET 1}} or
       * {{expression 1:2:3:4}}(option2)  or both (option3) */
      fprintf(fp, " ");
      loop_keylist(ndp, fp, human_readable);
#endif



      fprintf(fp,"}");

   }
}

enum {
   TSV = 0,
   HUMAN = 1
};
/********************************************************************************
 * Creates the tsv compatible and human readable files for storing info on nodes
 * of the experiment.
********************************************************************************/
int write_db_file(const char *seq_exp_home, FILE *tsv_output_fp, FILE *hr_output_fp)
{
   /*
    * Get list of nodes in experiment
    */
   PathArgNodePtr nodeList = getNodeList(seq_exp_home);
   PathArgNode_printList( nodeList , TL_FULL_TRACE );

   /*
    * Opitionnaly, one can reverse the list using SeqListNodeReverseList (this
    * is possible because I put the nextPtr as the first element of the struct).
    */
   /* SeqListNode_reverseList(&nodeList); */

   /*
    * For each node in the list, write an entry in the file
    */
   SeqNodeDataPtr ndp = NULL;
   for_pap_list(itr,nodeList){

      /* ndp = nodeinfo(itr->path, NI_RESOURCE_ONLY, NULL, seq_exp_home, NULL, NULL,itr->switch_args ); */
      ndp = SeqNode_createNode(itr->path);
      SeqNode_setSeqExpHome(ndp,seq_exp_home);
      ndp->type = itr->type;
      getNodeResources(ndp,seq_exp_home,itr->path);
      /*
       * Note that fprintf(NULL,...) is OK, but testing it here avoids testing
       * for every subsequent fprintf() that results from calling
       * node_to_keylist().
       */
      if( tsv_output_fp != NULL ){
         node_to_keylist(ndp, tsv_output_fp, TSV);
         /*
          * It seems to work even if I don't put the check (i.e. having a space
          * after the last element in the list), but TCL does do some funny
          * things sometimes with extra spaces, so as a matter of consistency,
          * let's avoid the extra space after the last element
          */
         if(itr->nextPtr != NULL){
            fprintf(tsv_output_fp, " ");
         }
      }

      if( hr_output_fp != NULL ){
         node_to_keylist(ndp, hr_output_fp, HUMAN);
         fprintf(hr_output_fp,"\n");
      }

      SeqNode_freeNode(ndp);
   }


out_free:
   PathArgNode_deleteList(&nodeList);
   return 0;
}
