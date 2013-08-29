#ifndef L2D2SERVER_H
#define L2D2SERVER_H

#define MAX_RETRIES 10

/* structure that holds l2d2server 'global' data */
typedef struct {
   unsigned int pid;
   unsigned int depProcPid;
   int      sock;     
   int      port;
   int      pollfreq;
   int      dependencyTimeOut;
   int      dzone;
   int      numProcessAtstart;
   char     ip[20];
   char     host[128];
   char     logdir[250];
   char     mlog[250];
   char     mlogerr[250];
   char     dmlog[250];
   char     dmlogerr[250];
   char     web[250];
   char     auth[250];
   char     web_dep[250];
   char     web_blk[250];
   char     lock_server_pid_file[250];
   char     dependencyPollDir[250];
   char     home[256];
   char     *user;
   char     *tmpdir;
   char     *m5sum;
   char     *mversion;
   char     *mshortcut;
} _l2d2server;

struct _depList {
   char xpd_name[250];
   char xpd_sname[250];
   char xpd_node[250];
   char xpd_status[15];
   char xpd_lock[1024];
   char xpd_sub[1024];
   char xpd_inode[20];
   struct _depList *next;
} depList;

struct _depParameters {
   char xpd_name[250];
   char xpd_node[250];
   char xpd_indx[15];
   char xpd_xpdate[20];
   char xpd_status[20];
   char xpd_largs[250];
   char xpd_susr[50];
   char xpd_sname[250];
   char xpd_snode[250];
   char xpd_sxpdate[20];
   char xpd_slargs[250];
   char xpd_sub[1024];
   char xpd_lock[1024];
   char xpd_dfilename[1024];
   char xpd_container[1024];
   char xpd_mversion[20];
   char xpd_mdomain[250];
   char xpd_regtimedate[20];
   char xpd_regtimepoch[20];
   char xpd_key[33];
} depParameters;

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

struct _depList depList;

static void maestro_l2d2_main_process_server (int fserver);
static void l2d2server_shutdown (void);
static void l2d2server_remove (void);
static void l2d2server_atexit (void);
static void l2d2Servlet( int sock );
static void l2d2SelectServlet( int sock );

#endif
