#ifndef L2D2SERVER_H
#define L2D2SERVER_H


/* structure that holds l2d2server 'global' data */
typedef struct {
   unsigned int pid;
   unsigned int depProcPid;
   int      sock;     
   int      port;
   int      pollfreq;
   int      dependencyTimeOut;
   int      dzone;
   int      maxNumOfProcess;
   int      maxClientPerProcess;
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
   char     lock_server_pid_file[250];
   char     dependencyPollDir[250];
   char     home[256];
   char     *user;
   char     *tmpdir;
   char     *m5sum;
   char     *mversion;
   char     *mshortcut;
} _l2d2server;

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

typedef struct {
      char host[64];
      char xp[256];
      char node[512];
      char signal[16];
      char Open_str[1024];
      char Close_str[1024];
      unsigned int trans;
} _l2d2client;

typedef enum _TypeOfWorker {
      ETERNAL,
      TRANSIENT
} TypeOfWorker;

union semun {
    int val;
    struct semid_ds *buf;
    ushort *array;
};

static void maestro_l2d2_main_process_server (int fserver);
static void l2d2server_shutdown (FILE *fp);
static void l2d2server_remove (FILE *fp);
static void l2d2SelectServlet( int sock , TypeOfWorker twrk );

extern void logZone(int , int , FILE *fp , char * , ...);
extern int initsem(key_t key, int nsems);
extern char *page_start_dep , *page_end_dep;
extern char *page_start_blocked , *page_end_blocked;

#endif
