/******************************************************************************
*FILE: nodelogger.h
*
*AUTHOR: Ping An Tan
******************************************************************************/

/*****************************************************************************
* nodelogger: write a formatted message to the oprun log
*        job: the name of the job
*        signl: 
*           C - 3bells "Job Has Bombed...Run Continues"
*           S - 3bells "Job Has Bombed...Run Stopped"
*           N - 3bells "Job Was Not Submitted"
*           B - 3bells "Job Has Bombed...Branch Stopped"
*           E - 3bells "Informative....Stem Ended"
*           I - 1bell  "*** MESSAGE ***"
*           A - 3bells "Job Has Bombed...Branch Continues"
*           R - 5bells "*** MESSAGE ***"
*           X - 0bells no additional message
*        message: the message we wanted to log.
*
* example: nodelogger("r1start_12",'x',"log test 1-2-3")           
* NOTE: call putenv("CMCNODELOG=on"); before using the nodelogger procedure to log all
*       messages, otherwise the  message will be written only to stdout.
* 
******************************************************************************/
extern void nodelogger(const char *job, const char *type, const char* loop_ext, const char *message, const char *datestamp);
extern void get_time (char * , int );

