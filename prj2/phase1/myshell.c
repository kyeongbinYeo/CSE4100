#include "csapp.h"
#include "myshell.h"

/*
 * parse_cmdline - Tokenize input line into argument array.
 *                 Returns 1 if background job requested, 0 otherwise.
 */
int parse_cmdline(char *line, char **args)
{
    int count = 0;
    char *ptr = line;

    /* Skip leading whitespace */
    while (*ptr == ' ' || *ptr == '\t')
        ptr++;

    while (*ptr != '\0') {
        /* Skip whitespace between tokens */
        while (*ptr == ' ' || *ptr == '\t')
            ptr++;

        if (*ptr == '\0' || *ptr == '\n')
            break;

        args[count++] = ptr;

        /* Advance to end of token */
        while (*ptr != '\0' && *ptr != ' ' &&
               *ptr != '\t' && *ptr != '\n')
            ptr++;

        if (*ptr == '\0')
            break;

        *ptr = '\0';
        ptr++;
    }

    args[count] = NULL;

    if (count == 0)
        return 0;

    /* Check for background operator */
    if (!strcmp(args[count - 1], "&")) {
        args[count - 1] = NULL;
        return 1;
    }

    return 0;
}

/*
 * run_builtin - Handle shell built-in commands.
 *               Returns 1 if command was a built-in, 0 otherwise.
 */
int run_builtin(char **args)
{
    /* Ignore lone ampersand */
    if (!strcmp(args[0], "&"))
        return 1;

    /* exit / quit: terminate the shell */
    if (!strcmp(args[0], "exit") || !strcmp(args[0], "quit"))
        exit(0);

    /* cd: change working directory */
    if (!strcmp(args[0], "cd")) {
        if (args[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("cd");
        }
        return 1;
    }

    return 0;
}

/*
 * execute_cmd - Evaluate and execute a command line.
 *               Uses execvp for automatic PATH resolution.
 */
void execute_cmd(char *line)
{
    char    buf[MAX_LINE];
    char   *args[MAX_ARGS];
    int     is_bg;
    pid_t   child_pid;
    int     status;

    strncpy(buf, line, MAX_LINE - 1);
    buf[MAX_LINE - 1] = '\0';

    is_bg = parse_cmdline(buf, args);

    /* Empty input */
    if (args[0] == NULL)
        return;

    /* Handle built-in commands in parent process */
    if (run_builtin(args))
        return;

    /* Fork child for external command */
    child_pid = Fork();

    if (child_pid == 0) {
        /* Child: execvp automatically searches PATH */
        if (execvp(args[0], args) < 0) {
            fprintf(stderr, "%s: command not found\n", args[0]);
            exit(1);
        }
    }

    /* Parent: wait for foreground job or print bg job info */
    if (!is_bg) {
        Waitpid(child_pid, &status, 0);
    } else {
        printf("[%d] %s", child_pid, line);
    }
}

/*
 * read_input - Print prompt and read a line from stdin.
 *              Returns 0 on EOF, 1 otherwise.
 */
int read_input(char *buf)
{
    printf("%s", SHELL_PROMPT);
    fflush(stdout);

    if (fgets(buf, MAX_LINE, stdin) == NULL)
        return 0;

    return 1;
}

/*
 * main - Shell entry point.
 *        Uses do-while loop: prompt → read → parse → execute.
 */
int main(void)
{
    char input[MAX_LINE];

    do {
        if (!read_input(input)) {
            if (feof(stdin))
                exit(0);
            continue;
        }
        execute_cmd(input);
    } while (1);

    return 0;
}