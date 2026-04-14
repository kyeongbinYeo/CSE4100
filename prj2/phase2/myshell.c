#include "csapp.h"
#include "myshell.h"

/* handlers for builtin cmds */
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
        perror("cd");  // chdir failed
}

// table of supported builtin commands
static cmd_entry_t cmd_table[] = {
    { "exit", handler_exit },
    { "quit", handler_exit },  // quit does same as exit
    { "cd",   handler_cd   },
    { NULL,   NULL         }   // end of table
};

/*
 * show prompt and read from stdin
 * returns 0 if EOF
 */
int read_input(char *cmdline)
{
    printf("%s", PROMPT);
    fflush(stdout);

    if (fgets(cmdline, MAXLINE, stdin) == NULL)
        return 0;

    return 1;
}

/*
 * split_pipes - split cmdline by '|' into separate command strings
 * returns number of commands found
 */
int split_pipes(char *buf, char *cmds[], int max)
{
    int n = 0;
    char *token;

    token = strtok(buf, "|");
    while (token != NULL && n < max) {
        cmds[n++] = token;
        token = strtok(NULL, "|");
    }
    cmds[n] = NULL;

    return n;
}

/*
 * parseline - split input into tokens
 * handles quoted strings (e.g. "abc") by stripping quotes
 * bg job if last arg is &
 */
int parseline(char *buf, char **argv)
{
    int     nargs = 0;
    int     is_bg;
    char   *ptr = buf;

    while (*ptr != '\0' && *ptr != '\n') {
        // skip whitespace
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

    // check for background operator
    if ((is_bg = (!strcmp(argv[nargs - 1], "&"))) != 0)
        argv[--nargs] = NULL;

    return is_bg;
}

int builtin_command(char **argv)
{
    int i;

    if (argv[0] == NULL)
        return 0;

    if (!strcmp(argv[0], "&"))  // ignore &
        return 1;

    /* search through cmd_table */
    for (i = 0; cmd_table[i].name != NULL; i++) {
        if (!strcmp(argv[0], cmd_table[i].name)) {
            cmd_table[i].handler(argv);
            return 1;
        }
    }

    return 0;  // not a builtin
}

/*
 * run_pipeline - execute a pipeline of n_cmds commands recursively
 *
 * base case : only one cmd left → just execvp
 * recursive : pipe() + fork two children
 *     left  → runs cmds[0], stdout → pipe write end
 *     right → runs rest of pipeline recursively, stdin ← pipe read end
 */
void run_pipeline(char *cmds[], int n_cmds)
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

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(1);
        }
    }

    // recursive case: create pipe and fork
    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(1);
    }

    // left child: runs cmds[0], stdout → pipe
    left_pid = Fork();
    if (left_pid == 0) {
        close(pipe_fd[0]);                   // dont need read end
        dup2(pipe_fd[1], STDOUT_FILENO);     // stdout → pipe
        close(pipe_fd[1]);

        strncpy(cmd_buf, cmds[0], MAXLINE - 1);
        cmd_buf[MAXLINE - 1] = '\0';
        parseline(cmd_buf, argv);

        if (argv[0] == NULL)
            exit(0);

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(1);
        }
    }

    // right child: runs rest of pipeline, stdin ← pipe
    right_pid = Fork();
    if (right_pid == 0) {
        close(pipe_fd[1]);                   // dont need write end
        dup2(pipe_fd[0], STDIN_FILENO);      // stdin ← pipe
        close(pipe_fd[0]);

        run_pipeline(cmds + 1, n_cmds - 1); // recurse
        exit(0);
    }

    // parent: close both ends and wait
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    Waitpid(left_pid, &status, 0);
    Waitpid(right_pid, &status, 0);
}

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
        return;  // empty line

    // check if there's a pipe
    n_cmds = split_pipes(pipe_buf, cmds, MAXCMDS);

    if (n_cmds > 1) {
        // pipeline: fork a child to handle the whole pipeline
        child_pid = Fork();
        if (child_pid == 0) {
            run_pipeline(cmds, n_cmds);
            exit(0);
        }
        if (!is_bg) {
            Waitpid(child_pid, &exit_status, 0);
        } else {
            printf("[%d] %s", child_pid, cmdline);
        }
        return;
    }

    // no pipe: handle builtins or single command
    if (!builtin_command(argv)) {
        child_pid = Fork();

        if (child_pid == 0) {
            // child process - run the command
            if (execvp(argv[0], argv) < 0) {
                fprintf(stderr, "%s: command not found\n", argv[0]);
                exit(1);
            }
        }

        if (!is_bg) {
            Waitpid(child_pid, &exit_status, 0);  // wait for fg
        } else {
            printf("[%d] %s", child_pid, cmdline);  // bg job
        }
    }
}

int main(void)
{
    char cmdline[MAXLINE];

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