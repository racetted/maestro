/* getdef_main.c - Command-line API to get definitions from files used by the Maestro sequencer software package.
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
#include "SeqUtil.h"
static void printUsage()
{
   printf("Usage:\n");
   printf("      getdef [-d] file key\n");
   printf("         where:\n");
   printf("         file     is full path of of the definition file (or shortcut: 'resources')\n");
   printf("         key      is name of the key to search for\n");
   printf("Example: getdef ${SEQ_EXP_HOME}/resources/resources.def loop_max\n");
   exit(1);
}

#ifdef Mop_linux
main_getdef ( int argc, char * argv[] )
#else
main ( int argc, char * argv[] )
#endif
{
   char *value,*deffile=NULL,*seq_exp_home=NULL;
   int file,key,c;

   if ( argc < 3 ) {
      printUsage();
   }
   file=argc-2; key=argc-1;
   while ((c = getopt(argc, (char* const*) argv, "d")) != -1) {
     switch(c) {
     case 'd':
       SeqUtil_setTraceLevel(1);
       break;
     case '?':
         printUsage(); 
         break;
     }
   }

   if (strcmp(argv[file],"resources") == 0){
     if ((seq_exp_home=getenv("SEQ_EXP_HOME")) == NULL){
       raiseError("ERROR: Shortcut %s unavailable when SEQ_EXP_HOME is undefined\n",argv[file]);
     }
     deffile = (char *) malloc(strlen(seq_exp_home)+strlen("/resources/resources.def")+2);
     sprintf(deffile,"%s/resources/resources.def",seq_exp_home);}
   else{
     deffile = (char *) malloc(strlen(argv[file])+1);
     strcpy(deffile,argv[file]);
   }
   if ( (value = SeqUtil_getdef( deffile, argv[key] )) == NULL ){
     raiseError("ERROR: Unable to find key %s in %s\n", argv[key], argv[file]);}
   else{
     printf( "%s\n", value );
   }
   free(value);
   free(deffile);
   free(0);
}
