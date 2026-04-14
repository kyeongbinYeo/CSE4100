#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#include <sys/types.h>

#define MAXARGS     128
#define MAXLINE     8192
#define MAXCMDS     64       // max number of piped commands
#define MAXJOBS     16       // max number of jobs
#define PROMPT      "CSE4100-SP-P2> "

// job states
#define UNDEF   0
#define RUNNING 1
#define STOPPED 2
#define DONE    3

// builtin cmd table struct
typedef struct {
    const char *name;
    void (*handler)(char **argv);
} cmd_entry_t;

// job struct
typedef struct {
    pid_t   pid;              // process group id
    int     jid;              // job id
    int     state;            // RUNNING, STOPPED, DONE
    char    cmdline[MAXLINE]; // command line
} job_t;

// job list & fg pid
extern job_t job_list[MAXJOBS];
extern volatile pid_t fg_pid;

/* job list management */
void    init_jobs(void);
int     add_job(pid_t pid, int state, char *cmdline);
void    delete_job(pid_t pid);
job_t  *find_job_by_pid(pid_t pid);
job_t  *find_job_by_jid(int jid);
void    list_jobs(void);

/* signal handlers */
void    sigchld_handler(int sig);
void    sigint_handler(int sig);
void    sigtstp_handler(int sig);

/* shell functions */
int     read_input(char *cmdline);
void    eval(char *cmdline);
int     parseline(char *buf, char **argv);
int     builtin_command(char **argv);
int     split_pipes(char *buf, char *cmds[], int max);
void    run_pipeline(char *cmds[], int n_cmds, pid_t pgid);

#endif