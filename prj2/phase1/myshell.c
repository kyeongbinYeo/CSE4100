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

void eval(char *cmdline)
{
    char   *argv[MAXARGS];
    char    line_buf[MAXLINE];
    int     is_bg;
    pid_t   child_pid;
    int     exit_status;

    strcpy(line_buf, cmdline);
    is_bg = parseline(line_buf, argv);

    if (argv[0] == NULL)
        return;  // empty line

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
 * parseline - split input into tokens
 * bg job if last arg is &
 * 따옴표로 묶인 문자열도 처리
 */
int parseline(char *buf, char **argv)
{
    int     nargs = 0;
    int     is_bg;
    char   *ptr = buf;

    while (*ptr != '\0' && *ptr != '\n') {
        // 공백 스킵
        while (*ptr == ' ' || *ptr == '\t') ptr++;
        if (*ptr == '\0' || *ptr == '\n') break;

        if (*ptr == '"') {
            // 따옴표로 묶인 토큰: 따옴표 제거
            ptr++;
            argv[nargs++] = ptr;
            while (*ptr != '"' && *ptr != '\0' && *ptr != '\n') ptr++;
            if (*ptr == '"') *ptr++ = '\0';
        } else {
            // 일반 토큰
            argv[nargs++] = ptr;
            while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
                ptr++;
            if (*ptr) *ptr++ = '\0';
        }
    }
    argv[nargs] = NULL;

    if (nargs == 0)
        return 0;

    // 마지막 인자가 &인지 확인
    if ((is_bg = (!strcmp(argv[nargs - 1], "&"))) != 0)
        argv[--nargs] = NULL;

    return is_bg;
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