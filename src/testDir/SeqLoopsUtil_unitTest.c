int countList(LISTNODEPTR list_head)
{
   int count = 0;
   LISTNODEPTR itr = NULL;
   if( list_head == NULL )
      return -1;
   for(itr = list_head, count = 0; itr != NULL; itr = itr->nextPtr)
      count++;
   return count;
}

int test_SeqLoops_getNodeLoopContainersExtensionsInReverse()
{
   const char * exp = "/home/ops/afsi/phc/Documents/Experiences/sample-loops_bug";
   const char * node = "/sample_1.4.3/BadNames/outer_loop/inner_loop/END/SET";
   SeqNodeDataPtr ndp = nodeinfo (node, NI_SHOW_ALL ,NULL ,exp,
                                 NULL, "20160102030000",NULL );
   SeqNode_printNode(ndp, NI_SHOW_ALL,NULL);


   LISTNODEPTR extensions = SeqLoops_getLoopContainerExtensionsInReverse(ndp,"outer_loop=*,inner_loop=*,END=*");
   if( countList(extensions) != 30)
      TEST_FAILED;
   SeqListNode_deleteWholeList(extensions);


   extensions = SeqLoops_getLoopContainerExtensionsInReverse(ndp,"outer_loop=1,inner_loop=2,END=3");
   if( countList(extensions) != 1)
      TEST_FAILED;
   SeqListNode_deleteWholeList(extensions);


   extensions = SeqLoops_getLoopContainerExtensionsInReverse(ndp,"");
   if( countList(extensions) != 1 || strlen(extensions->data) != 0)
      TEST_FAILED;
   SeqListNode_deleteWholeList(extensions);


   return 0;
}
   test_SeqLoops_getNodeLoopContainersExtensionsInReverse();
