#include "csapp.h"
#include "myshell.h"

/* ------------------------------------------------------------------ */
/*  Global variables                                                    */
/* ------------------------------------------------------------------ */

job_t           job_list[MAXJOBS];  // job list
volatile pid_t  fg_pid = 0;         // pid of current foreground process
sigset_t        mask, prev;         // global signal masks

/* ------------------------------------------------------------------ */
/*  Job list management                                                 */
/* ------------------------------------------------------------------ */

void init_jobs(void)
{
    int i;
    for (i = 0; i < MAXJOBS; i++) {
        job_list[i].pid   = 0;
        job_list[i].jid   = 0;
        job_list[i].state = UNDEF;
        job_list[i].cmdline[0] = '\0';
    }
}

int add_job(pid_t pid, int state, char *cmdline)
{
    int i, jid = 1;

    // find smallest available jid
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid >= jid)
            jid = job_list[i].jid + 1;

    for (i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == 0) {
            job_list[i].pid   = pid;
            job_list[i].jid   = jid;
            job_list[i].state = state;
            strncpy(job_list[i].cmdline, cmdline, MAXLINE - 1);
            return jid;
        }
    }
    fprintf(stderr, "job list is full\n");
    return -1;
}

void delete_job(pid_t pid)
{
    int i;
    for (i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == pid) {
            job_list[i].pid   = 0;
            job_list[i].jid   = 0;
            job_list[i].state = UNDEF;
            job_list[i].cmdline[0] = '\0';
            return;
        }
    }
}

job_t *find_job_by_pid(pid_t pid)
{
    int i;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].pid == pid)
            return &job_list[i];
    return NULL;
}

job_t *find_job_by_jid(int jid)
{
    int i;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid == jid)
            return &job_list[i];
    return NULL;
}

void list_jobs(void)
{
    int i;
    const char *state_str;

    for (i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == 0) continue;
        switch (job_list[i].state) {
            case RUNNING: state_str = "Running";     break;
            case STOPPED: state_str = "Stopped";     break;
            case DONE:    state_str = "Done";         break;
            default:      state_str = "Unknown";      break;
        }
        printf("[%d] %d %s %s", job_list[i].jid, job_list[i].pid,
               state_str, job_list[i].cmdline);
    }
}

/* ------------------------------------------------------------------ */
/*  Signal handlers                                                     */
/* ------------------------------------------------------------------ */

// SIGCHLD: child stopped or terminated
void sigchld_handler(int sig)
{
    pid_t pid;
    int   status;
    job_t *job;

    (void)sig;

    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        job = find_job_by_pid(pid);
        if (job == NULL) continue;

        if (WIFSTOPPED(status)) {
            // child stopped (Ctrl+Z)
            job->state = STOPPED;
            printf("\n[%d] %d Stopped %s", job->jid, job->pid, job->cmdline);
            fflush(stdout);
            if (job->pid == fg_pid)
                fg_pid = 0;  // release fg so sigsuspend loop exits
        } else {
            // child terminated
            if (pid == fg_pid) {
                fg_pid = 0;
            } else {
                // bg job terminated - reprint prompt
                printf("\n%s", PROMPT);
                fflush(stdout);
            }
            delete_job(pid);
        }
    }
}

// SIGINT: Ctrl+C - terminate foreground job
void sigint_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0) {
        Kill(-fg_pid, SIGINT);
        Sio_puts("\n");  // 개행 추가
    }
}


// SIGTSTP: Ctrl+Z - stop foreground job
void sigtstp_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0)
        Kill(-fg_pid, SIGTSTP);  // send to entire process group
}

/* ------------------------------------------------------------------ */
/*  Built-in command handlers                                           */
/* ------------------------------------------------------------------ */

static void handler_exit(char **argv)
{
    (void)argv;
    exit(0);  // just exit
}

static void handler_cd(char **argv)
{
    // no argument given
    if (argv[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(argv[1]) != 0)
        perror("cd");
}

static void handler_jobs(char **argv)
{
    (void)argv;
    list_jobs();
}

static void handler_fg(char **argv)
{
    int    jid;
    job_t *job;
    pid_t  pid;

    if (argv[1] == NULL) {
        fprintf(stderr, "fg: missing job id\n");
        return;
    }

    jid = atoi(argv[1]);
    job = find_job_by_jid(jid);

    if (job == NULL) {
        fprintf(stderr, "fg: job %d not found\n", jid);
        return;
    }

    pid = job->pid;
    fg_pid = pid;
    job->state = RUNNING;
    Kill(-pid, SIGCONT);

    sigset_t empty;
    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, NULL);
    while (fg_pid == pid)
        sigsuspend(&empty);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

static void handler_bg(char **argv)
{
    int    jid;
    job_t *job;

    if (argv[1] == NULL) {
        fprintf(stderr, "bg: missing job id\n");
        return;
    }

    jid = atoi(argv[1]);
    job = find_job_by_jid(jid);

    if (job == NULL) {
        fprintf(stderr, "bg: job %d not found\n", jid);
        return;
    }

    job->state = RUNNING;
    Kill(-job->pid, SIGCONT);  // resume in background
    printf("[%d] %d %s", job->jid, job->pid, job->cmdline);
}

static void handler_kill(char **argv)
{
    int    jid;
    job_t *job;

    if (argv[1] == NULL) {
        fprintf(stderr, "kill: missing job id\n");
        return;
    }

    jid = atoi(argv[1]);
    job = find_job_by_jid(jid);

    if (job == NULL) {
        fprintf(stderr, "kill: job %d not found\n", jid);
        return;
    }

    Kill(-job->pid, SIGTERM);  // terminate process group
}

// table of supported builtin commands
static cmd_entry_t cmd_table[] = {
    { "exit", handler_exit },
    { "quit", handler_exit },
    { "cd",   handler_cd   },
    { "jobs", handler_jobs },
    { "fg",   handler_fg   },
    { "bg",   handler_bg   },
    { "kill", handler_kill },
    { NULL,   NULL         }
};

/* ------------------------------------------------------------------ */
/*  read_input                                                          */
/* ------------------------------------------------------------------ */

int read_input(char *cmdline)
{
    printf("%s", PROMPT);
    fflush(stdout);

    if (fgets(cmdline, MAXLINE, stdin) == NULL)
        return 0;

    return 1;
}

/* ------------------------------------------------------------------ */
/*  split_pipes                                                         */
/* ------------------------------------------------------------------ */

int split_pipes(char *buf, char *cmds[], int max)
{
    int   n = 0;
    char *token;

    token = strtok(buf, "|");
    while (token != NULL && n < max) {
        cmds[n++] = token;
        token = strtok(NULL, "|");
    }
    cmds[n] = NULL;

    return n;
}

/* ------------------------------------------------------------------ */
/*  parseline                                                           */
/* ------------------------------------------------------------------ */

int parseline(char *buf, char **argv)
{
    int     nargs = 0;
    int     is_bg;
    char   *ptr = buf;

    while (*ptr != '\0' && *ptr != '\n') {
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        if (*ptr == '\0' || *ptr == '\n') break;

        if (*ptr == '"') {
            // quoted token: strip the quotes
            ptr++;
            argv[nargs++] = ptr;
            while (*ptr != '"' && *ptr != '\0') ptr++;
            if (*ptr == '"') *ptr++ = '\0';
        } else {
            // normal token
            argv[nargs++] = ptr;
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
                ptr++;
            if (*ptr) *ptr++ = '\0';
        }
    }
    argv[nargs] = NULL;

    if (nargs == 0)
        return 0;

    if ((is_bg = (!strcmp(argv[nargs - 1], "&"))) != 0)
        argv[--nargs] = NULL;

    return is_bg;
}

/* ------------------------------------------------------------------ */
/*  builtin_command                                                     */
/* ------------------------------------------------------------------ */

int builtin_command(char **argv)
{
    int i;

    if (argv[0] == NULL)
        return 0;

    if (!strcmp(argv[0], "&"))
        return 1;

    for (i = 0; cmd_table[i].name != NULL; i++) {
        if (!strcmp(argv[0], cmd_table[i].name)) {
            cmd_table[i].handler(argv);
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  run_pipeline                                                        */
/* ------------------------------------------------------------------ */

void run_pipeline(char *cmds[], int n_cmds, pid_t pgid)
{
    char   *argv[MAXARGS];
    char    cmd_buf[MAXLINE];
    int     pipe_fd[2];
    pid_t   left_pid, right_pid;
    int     status;

    // base case: only one command
    if (n_cmds == 1) {
        strncpy(cmd_buf, cmds[0], MAXLINE - 1);
        cmd_buf[MAXLINE - 1] = '\0';
        parseline(cmd_buf, argv);

        if (argv[0] == NULL)
            exit(0);

        // set process group
        setpgid(0, pgid);

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(1);
        }
    }

    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(1);
    }

    // left child
    left_pid = Fork();
    if (left_pid == 0) {
        close(pipe_fd[0]);
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        strncpy(cmd_buf, cmds[0], MAXLINE - 1);
        cmd_buf[MAXLINE - 1] = '\0';
        parseline(cmd_buf, argv);

        if (argv[0] == NULL) exit(0);

        setpgid(0, pgid);

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(1);
        }
    }

    // right child
    right_pid = Fork();
    if (right_pid == 0) {
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);

        run_pipeline(cmds + 1, n_cmds - 1, pgid);
        exit(0);
    }

    close(pipe_fd[0]);
    close(pipe_fd[1]);
    Waitpid(left_pid, &status, 0);
    Waitpid(right_pid, &status, 0);
}

/* ------------------------------------------------------------------ */
/*  eval                                                                */
/* ------------------------------------------------------------------ */

void eval(char *cmdline)
{
    char   *argv[MAXARGS];
    char    line_buf[MAXLINE];
    char    pipe_buf[MAXLINE];
    char   *cmds[MAXCMDS];
    int     is_bg;
    int     n_cmds;
    pid_t   child_pid;
    int     exit_status;

    strcpy(line_buf, cmdline);
    strcpy(pipe_buf, cmdline);

    is_bg = parseline(line_buf, argv);

    if (argv[0] == NULL)
        return;

    sigprocmask(SIG_BLOCK, &mask, &prev);

    n_cmds = split_pipes(pipe_buf, cmds, MAXCMDS);

    if (n_cmds > 1) {
        child_pid = Fork();
        if (child_pid == 0) {
            sigprocmask(SIG_SETMASK, &prev, NULL);
            setpgid(0, 0);
            run_pipeline(cmds, n_cmds, getpid());
            exit(0);
        }

        setpgid(child_pid, child_pid);

        if (!is_bg) {
            add_job(child_pid, RUNNING, cmdline);
            fg_pid = child_pid;
            sigprocmask(SIG_SETMASK, &prev, NULL);
            sigset_t empty;
            sigemptyset(&empty);
            while (fg_pid == child_pid)
                sigsuspend(&empty);
        } else {
            int jid = add_job(child_pid, RUNNING, cmdline);
            printf("[%d] %d %s", jid, child_pid, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
        return;
    }

    if (!builtin_command(argv)) {
        child_pid = Fork();

        if (child_pid == 0) {
            sigprocmask(SIG_SETMASK, &prev, NULL);
            setpgid(0, 0);

            if (execvp(argv[0], argv) < 0) {
                fprintf(stderr, "%s: command not found\n", argv[0]);
                exit(1);
            }
        }

        setpgid(child_pid, child_pid);

        if (!is_bg) {
            add_job(child_pid, RUNNING, cmdline);
            fg_pid = child_pid;
            sigprocmask(SIG_SETMASK, &prev, NULL);
            sigset_t empty;
            sigemptyset(&empty);
            while (fg_pid == child_pid)
                sigsuspend(&empty);
        } else {
            int jid = add_job(child_pid, RUNNING, cmdline);
            printf("[%d] %d %s", jid, child_pid, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
    } else {
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }

    (void)exit_status;
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

int main(void)
{
    char cmdline[MAXLINE];

    // initialize job list
    init_jobs();

    // initialize global signal mask
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, &prev);  // block and save prev

    // register signal handlers
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT,  sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);

    // unblock after handlers registered
    sigprocmask(SIG_SETMASK, &prev, NULL);

    while (1) {
        if (!read_input(cmdline)) {
            if (feof(stdin))
                exit(0);
            continue;
        }
        eval(cmdline);
    }

    return 0;
}