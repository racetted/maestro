void test_genSwitch()
{
   const char * test_flow = absolutePath("gen_switch/gen_switch_flow.xml");
   /* SeqNodeDataPtr ndp = SeqNode_createNode("PHIL"); */

   /* Create a FlowVisitor by specifying an xmlFile instead of the usual way of
    * specifying the expHome since there is no home */
   FlowVisitorPtr fv = createTestFlowVisitor(test_flow,NULL);

   /* Get the flow visitor to the switch */
   int isFirst = 1;
   Flow_doNodeQuery(fv,"my_module", isFirst);
   isFirst = 0;
   Flow_doNodeQuery(fv,"my_gen_switch", isFirst);

   /* Flow_parseSwitchAttributes() */
   {
      const char * switchType = xmlGetProp(fv->context->node, "type");
      SeqUtil_TRACE(TL_FULL_TRACE, "swithch type = %s\n",switchType);

      /* Get the path of the swithc */
      /* containerTaskPath( const char * expHome, const char * nodePath) */
      {
         /* From the path fv->currentFlowNode and the experiment home, construct
          * the path to the container.tsk of the switch */
      } const char * containerTaskPath = absolutePath("gen_switch/container.tsk");

      SeqUtil_TRACE(TL_FULL_TRACE, "%s\n", containerTaskPath);
      /* Take script path, return a string containing output */
      const char * output_of_script = NULL;
      output_of_script = SeqUtil_getScriptOutput(containerTaskPath, SEQ_MAXFIELD);

      SeqUtil_TRACE(TL_CRITICAL,"Output received by function:%s\n", output_of_script);

      output_of_script = SeqUtil_popenGetScriptOutput(containerTaskPath, SEQ_MAXFIELD);
      SeqUtil_TRACE(TL_CRITICAL,"Output received by popen_function:%s\n", output_of_script);

      /*
       * That takes care of the reading of the script part.  Now back to
       * implementing the desired behavior.  We have to look at the original
       * function.  This function needs to be divided so that we have one part
       * that doesn't depend on the switch value, and one part that does.
       *
       * Lets inspect the actual version of Flow_parseSwitchAttributes().  Then
       * we can break it up, into two functions: one that takes the info from
       * the SWITCH node which will be called for both types of switches, then
       * one set of two functions: one that will be called for generic switches,
       * and the other that will be called for generic switches
       *
       * This is done in a flow-chart.
       *
       */
      if( strcmp(output_of_script, "Hillary") != 0
            && strcmp(output_of_script,"Trump") != 0)
         raiseError("TEST_FAILED: script capturing functions\n");
   }
}


/* test_genSwitch_nodeinfo(); */


   SeqUtil_setTraceFlag(TRACE_LEVEL, TL_CRITICAL);
   int i;
   for(i = 0; i < 20; i++)
      test_genSwitch();
