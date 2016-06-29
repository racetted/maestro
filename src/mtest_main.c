/* mtest_main.c - Used for experimentation and unit testing.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/tree.h>
#include <libxml/xpathInternals.h>
#include "ResourceVisitor.h"
#include "FlowVisitor.h"
#include "SeqUtil.h"
#include "nodeinfo.h"
#include "getopt.h"
#include "SeqNode.h"
#include "XmlUtils.h"

static char * testDir = NULL;
int MLLServerConnectionFid=0;

/********************************************************************************
 * MAESTRO TEST FILE
 *
 * This file is intended as a place to do unit testing and experimentation
 * during development and bug solving in maestro.
 *
 * This file assumes that the executable is being run from the maestro directory
 * so that paths can be relative to that directory:
 *
 * testDir is the location to look in for whaterver files are being used for
 * these tests.
 *
 * It is encouraged to put all files that need to be accessed in
 * that directory, so that, for example, if one must modify resourceVisitor
 * functions, they can checkout the mtest_main.c file from a previous commit and
 * run the tests periodically to make sure that all the functions still fulfill
 * their contract, and to catch runtime errors at the earliest possible moment.
 *
 * The base of the test file is the main(), the runTests() function and the
 * absolutePath() function, and the header() function.  The rest is the actual
 * tests, which should have a SETUP, and some TESTS where the result of the test
 * is verified, and raiseError() should be called if the result is different
 * from the expected result.
 *
 * Lower level functions should be tested first so that they may be known to
 * work when testing the higher level functions that use them.
 *
********************************************************************************/

/********************************************************************************
 * Creates an absolute path by appending the relative path to testDir, where
 * testDir = ${MAESTRO_REPO_LOCATION}/testDir/
 * This should be used for any paths so that the tests can be portable to
 * different users who keep their maestro stuff in different places.
********************************************************************************/
char * absolutePath(const char * relativePath)
{
   SeqUtil_TRACE(TL_FULL_TRACE, "absolutePath() begin\n");
   char * absPath = (char *) malloc( strlen(testDir) + 1 + strlen(relativePath) + 1);
   sprintf( absPath, "%s%c%s", testDir,'/', relativePath);
   SeqUtil_TRACE(TL_FULL_TRACE, "absolutePath() end, returning %s\n", absPath);
   return absPath;
}


void header(const char * test){
   SeqUtil_TRACE(TL_CRITICAL, "\n=================== UNIT TEST FOR %s ===================\n",test);
}

void printListWithLinefeed(LISTNODEPTR list_head, int trace_level){
   for_list(itr,list_head){
      SeqUtil_TRACE(trace_level, "%s\n", itr->data);
   }
}


/*
 * SOME NOTES ABOUT THIS TEST:
 * This test was made in another commit to develop an algorithm for getting the
 * list of all the nodes in an experiment.  I include it here as a means to test
 * the Flow_changeModule() function that uses a stack of xmlXPathContextPtr.
 *
 * This algorithm was going to be used for bug4689 but I ended up having to do
 * too many hackish things for my liking.  Therefore I'm going to wait until
 * Dominic gets back from vacation and discuss things like changing the desired
 * behavior so that it can be implemented with good design/coding practices.
 *
 * The root of the hackishness is switches, and the starting hack is to put
 * switch item names in square brackets.  As I worked on generic switches
 * (bug7312) I noticed that I hadn't thought about using loop_args.  So maybe
 * instead of weird hackish paths, we could use a (path,loop_args) pair as
 * entries in our 'node census'.
 */
void parseFlowTree_internal(FlowVisitorPtr fv, LISTNODEPTR * list_head,
                                    const char * basePath, int depth);

LISTNODEPTR parseFlowTree(const char * seq_exp_home)
{
   LISTNODEPTR list_head = NULL;
   FlowVisitorPtr fv = Flow_newVisitor(seq_exp_home);

   const char * basePath = (const char *) xmlGetProp(fv->context->node,
                                                      (const xmlChar *)"name");
   SeqListNode_pushFront(&list_head, basePath);

   parseFlowTree_internal(fv, &list_head,basePath, 0);

   free((char*)basePath);
   Flow_deleteVisitor(fv);
   return list_head;

}

void indent(int depth)
{
   int i;
   for(i = depth; i--;)
      fprintf(stderr, "    ");
}

void parseFlowTree_internal(FlowVisitorPtr fv, LISTNODEPTR * list_head,
                                    const char * basePath, int depth)
{
   /*
    * Oh heavenly Father, I pray to thee to forgive me for this inelegant query
    * which amounts to saying "all children except SUBMITS".  There has to be a
    * way of saying something like child::!SUBMITS or something.
    */
   xmlXPathObjectPtr results = XmlUtils_getnodeset("(child::FAMILY|child::TASK\
                                                     |child::SWITCH|child::SWITCH_ITEM\
                                                     |child::MODULE|child::LOOP\
                                                     |child::NPASS_TASK|child::FOR_EACH)"
                                                      , fv->context);

   for_results( xmlNode, results ){
      const char * name = (const char *)xmlGetProp( xmlNode, (const xmlChar *)"name");
      xmlNodePtr node = xmlNode;

      if( strcmp(node->name, "TASK") == 0 || strcmp(node->name, "NPASS_TASK") == 0){
         char path[SEQ_MAXFIELD] = {0};
         sprintf( path, "%s/%s", basePath,name);
         SeqListNode_pushFront(list_head, path);
      } else if( strcmp(node->name, "MODULE") == 0){
         char path[SEQ_MAXFIELD] = {0};
         sprintf( path, "%s/%s", basePath,name);
         SeqListNode_pushFront(list_head, path);
         Flow_changeModule(fv, (const char *) name);
         parseFlowTree_internal(fv, list_head,path, depth+1);
         Flow_restoreContext(fv);
      } else if( strcmp(node->name, "LOOP") == 0
          || strcmp(node->name, "FAMILY") == 0
          || strcmp(node->name, "SWITCH") == 0){
         char path[SEQ_MAXFIELD] = {0};
         sprintf( path, "%s/%s", basePath,name);
         SeqListNode_pushFront(list_head, path);
         xmlNodePtr previousNode = fv->context->node;
         fv->context->node = node;
         parseFlowTree_internal(fv, list_head,path,depth+1);
         fv->context->node = previousNode;
      } else if( strcmp(node->name, "SWITCH_ITEM") == 0 ){
         char path[SEQ_MAXFIELD] = {0};
         const char * switch_item_name = xmlGetProp(xmlNode, (const xmlChar *)"name");
         sprintf( path, "%s[%s]", basePath,switch_item_name);
         xmlNodePtr previousNode = fv->context->node;
         fv->context->node = node;
         parseFlowTree_internal(fv, list_head,path, depth+1);
         fv->context->node = previousNode;
         free((char*)switch_item_name);
      }
      free((char *)name);
   }
   xmlXPathFreeObject(results);
}

int runTests(const char * seq_exp_home, const char * node, const char * datestamp)
{

   SeqUtil_setTraceFlag(TRACE_LEVEL, TL_CRITICAL);
   LISTNODEPTR list_head = parseFlowTree(seq_exp_home);

   SeqUtil_setTraceFlag(TRACE_LEVEL, TL_FULL_TRACE);
   SeqUtil_TRACE(TL_FULL_TRACE, "===============================================================\nNote that this test is dependent on an experiment that is not in the test directory.\n\n");
   SeqUtil_setTraceFlag(TRACE_LEVEL, TL_FULL_TRACE);
   SeqListNode_reverseList(&list_head);
   printListWithLinefeed(list_head,TL_FULL_TRACE);



   SeqListNode_deleteWholeList(&list_head);
   SeqUtil_TRACE(TL_CRITICAL, "============== ALL TESTS HAVE PASSED =====================\n");
   return 0;
}
int main ( int argc, char * argv[] )
{
   char * short_opts = "n:f:l:o:d:e:v";
   char *node = NULL, *seq_exp_home = NULL, *datestamp=NULL ;
   extern char *optarg;

   extern char *optarg;
   extern int   optind;
   struct       option long_opts[] =
   { /*  NAME        ,    has_arg       , flag  val(ID) */

      {"exp"         , required_argument,   0,     'e'},
      {"node"        , required_argument,   0,     'n'},
      {"loop-args"   , required_argument,   0,     'l'},
      {"datestamp"   , required_argument,   0,     'd'},
      {"outputfile"  , required_argument,   0,     'o'},
      {"filters"     , required_argument,   0,     'f'},
      {"verbose"     , no_argument      ,   0,     'v'},
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;

   while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_index )) != -1) {
      switch(c) {
         case 'n':
            node = strdup(optarg);
            break;
         case 'e':
            seq_exp_home = strdup(optarg);
            break;
         case 'd':
            datestamp = strdup(optarg);
            break;
         case '?':
            exit(1);
      }
   }

   SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );

   const char * PWD = getenv("PWD");
   /* Check that the path PWD ends with maestro.  It's the best we can do to
    * make sure that mtest is being run from the right place. */
   const char * p = PWD;
   while(*p++ != 0 );
   while(*(p-1) != '/') --p;
   if( strcmp(p,"maestro") != 0 ){
      SeqUtil_TRACE(TL_FULL_TRACE, "\
Main function for doing tests, please run this from the maestro directory so\n\
that the location of the test files may be known.  Eg by doing \n\
   'make install; mtest'\n\
or\n\
   'make; ./src/mtest\n\
from the maestro directory.\n");
      exit(1);
   }

   char * suffix = "/src/testDir";
   testDir = (char *) malloc( sizeof(char) * (strlen(PWD) + strlen(suffix) + 1));
   sprintf( testDir, "%s%s" , PWD, suffix);

   puts ( testDir );
   seq_exp_home = strdup("/home/ops/afsi/phc/Documents/Experiences/sample/");

   runTests(seq_exp_home,node,datestamp);

   free(node);
   free(seq_exp_home);
   free(datestamp);
   return 0;
}








