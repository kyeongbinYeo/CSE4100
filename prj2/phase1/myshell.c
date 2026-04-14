#include "csapp.h"
#include "myshell.h"

/* ------------------------------------------------------------------ */
/*  Built-in command handlers                                           */
/* ------------------------------------------------------------------ */

static void builtin_exit(char **args)
{
    (void)args;
    exit(0);
}

static void builtin_cd(char **args)
{
    if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(args[1]) != 0)
        perror("cd");
}

/* Table of built-in commands and their handlers */
static builtin_entry_t builtin_table[] = {
    { "exit", builtin_exit },
    { "quit", builtin_exit },
    { "cd",   builtin_cd   },
    { NULL,   NULL         }
};

/* ------------------------------------------------------------------ */
/*  read_input                                                          */
/* ------------------------------------------------------------------ */

/*
 * Print the shell prompt and read one line from stdin.
 * Returns 1 on success, 0 on EOF.
 */
int read_input(char *buf)
{
    printf("%s", SHELL_PROMPT);
    fflush(stdout);

    if (fgets(buf, LINE_BUF_SIZE, stdin) == NULL)
        return 0;

    return 1;
}

/* ------------------------------------------------------------------ */
/*  parse_cmdline                                                       */
/* ------------------------------------------------------------------ */

/*
 * Tokenize input line with strtok into args array.
 * Returns 1 if last token is "&" (background), 0 otherwise.
 */
int parse_cmdline(char *line, char **args)
{
    int     count = 0;
    char   *token;

    token = strtok(line, " \t\n");
    while (token != NULL && count < ARG_MAX_COUNT - 1) {
        args[count++] = token;
        token = strtok(NULL, " \t\n");
    }
    args[count] = NULL;

    if (count == 0)
        return 0;

    /* Detect background operator */
    if (strcmp(args[count - 1], "&") == 0) {
        args[count - 1] = NULL;
        return 1;
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  run_builtin                                                         */
/* ------------------------------------------------------------------ */

/*
 * Search builtin_table for args[0].
 * If found, call its handler and return 1; otherwise return 0.
 */
int run_builtin(char **args)
{
    int i;

    if (args[0] == NULL)
        return 0;

    /* Ignore lone ampersand */
    if (strcmp(args[0], "&") == 0)
        return 1;

    for (i = 0; builtin_table[i].name != NULL; i++) {
        if (strcmp(args[0], builtin_table[i].name) == 0) {
            builtin_table[i].handler(args);
            return 1;
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/*  spawn_process                                                       */
/* ------------------------------------------------------------------ */

/*
 * Fork a child process and execvp the command.
 * Parent waits for foreground jobs; prints PID for background jobs.
 */
void spawn_process(char **args, int is_bg, char *line)
{
    pid_t   pid;
    int     status;

    pid = Fork();

    if (pid == 0) {
        /* Child process */
        if (execvp(args[0], args) < 0) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(1);
        }
    }

    /* Parent process */
    if (is_bg) {
        printf("[%d] %s", pid, line);
    } else {
        Waitpid(pid, &status, 0);
    }
}

/* ------------------------------------------------------------------ */
/*  execute_cmd                                                         */
/* ------------------------------------------------------------------ */

/*
 * Parse and dispatch one command line.
 * Built-ins run in the parent; external commands fork a child.
 */
void execute_cmd(char *line)
{
    char    buf[LINE_BUF_SIZE];
    char   *args[ARG_MAX_COUNT];
    int     is_bg;

    strncpy(buf, line, LINE_BUF_SIZE - 1);
    buf[LINE_BUF_SIZE - 1] = '\0';

    is_bg = parse_cmdline(buf, args);

    if (args[0] == NULL)
        return;

    if (run_builtin(args))
        return;

    spawn_process(args, is_bg, line);
}

/* ------------------------------------------------------------------ */
/*  main                                                                */
/* ------------------------------------------------------------------ */

/*
 * Shell entry point.
 * Infinite for(;;) loop: read → parse → execute.
 */
int main(void)
{
    char input[LINE_BUF_SIZE];

    for (;;) {
        if (!read_input(input)) {
            if (feof(stdin))
                exit(0);
            continue;
        }
        execute_cmd(input);
    }

    return 0;
}