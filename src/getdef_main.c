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
#include <unistd.h>
#include "SeqUtil.h"
#include "getopt.h"
static void printUsage()
{
   char * usage = "DESCRIPTION: getdef: Searches in _file_ for the value of _key_ where file contains\n\
    lines of the form \"key=value\".\n\
\n\
USAGE:\n\
\n\
    getdef [-v] [-e experiment_home] file key\n\
\n\
OPTIONS:\n\
\n\
    -e, --exp \n\
        Experiment path.  If it is not supplied, the environment variable \n\
        SEQ_EXP_HOME will be used.\n\
\n\
    -v, --verbose\n\
        Turn on full tracing\n\
\n\
    -h, --help\n\
        Show this help screen\n\
\n\
\n\ file can be the special shortcut keyword resources which will be replaced by ${SEQ_EXP_HOME}/resources/resources.def \n\
EXAMPLES:\n\
\n\
    getdef -e ${SEQ_EXP_HOME} resources some_variable\n\
    getdef resources some_other_variable   ($SEQ_EXP_HOME must be defined) \n\
    getdef some_file yet_another_variable \n"; 
   puts(usage);
}

int main ( int argc, char * argv[] )
{
   const char*  short_opts = "e:v";
   extern char* optarg;
   extern int   optind;
   struct       option long_opts[] =
   { /*  NAME        ,    has_arg       , flag  val(ID) */
      {"exp"    , required_argument,   0,     'e'},
      {"verbose"     , no_argument      ,   0,     'v'},
      {"help"        , no_argument      ,   0,     'h'},
      {NULL,0,0,0} /* End indicator */
   };
   int opt_index, c = 0;
   char *value,*deffile=NULL,*seq_exp_home=NULL;
   int file,key;

   if ( argc < 3 ) {
      printUsage();
      exit(1);
   }
   file=argc-2; key=argc-1;
   while ((c = getopt_long(argc, argv, short_opts, long_opts, &opt_index)) != -1) {
     switch(c) {
        case 'e':
           seq_exp_home = strdup( optarg );
           break;
        case 'v':
          SeqUtil_setTraceFlag( TRACE_LEVEL , TL_FULL_TRACE );
          SeqUtil_setTraceFlag( TF_TIMESTAMP , TF_ON );
          break;
        case 'h':
        case '?':
            printUsage(); 
            exit(1);
            break;
     }
   }

   if (strcmp(argv[file],"resources") == 0){
      if (seq_exp_home == NULL){
        if ((seq_exp_home=getenv("SEQ_EXP_HOME")) == NULL){
          raiseError("ERROR: Shortcut %s unavailable when SEQ_EXP_HOME is undefined\n",argv[file]);
        }
      }
     deffile = (char *) malloc(strlen(seq_exp_home)+strlen("/resources/resources.def")+2);
     sprintf(deffile,"%s/resources/resources.def",seq_exp_home);
   }else{
      if (seq_exp_home == NULL){
        if ((seq_exp_home=getenv("SEQ_EXP_HOME")) == NULL){
            seq_exp_home=getenv("HOME"); 
        }
      }
      deffile = (char *) malloc(strlen(argv[file])+1);
      strcpy(deffile,argv[file]);
   }
   
   if ( (value = SeqUtil_getdef( deffile, argv[key],seq_exp_home )) == NULL ){
     raiseError("ERROR: Unable to find key %s in %s\n", argv[key], argv[file]);}
   else{
     printf( "%s\n", value );
   }
   free(value);
   free(deffile);
   SeqUtil_unmapfiles();
   exit(0);
}
