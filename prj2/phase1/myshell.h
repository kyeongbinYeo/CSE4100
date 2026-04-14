#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#define ARG_MAX_COUNT   128
#define LINE_BUF_SIZE   8192
#define SHELL_PROMPT    "CSE4100-SP-P2> "

/* Builtin command entry: name and handler function pointer */
typedef struct {
    const char *name;
    void (*handler)(char **args);
} builtin_entry_t;

/* Read a line from stdin into buf, print prompt first.
 * Returns 1 on success, 0 on EOF. */
int     read_input(char *buf);

/* Tokenize input line using strtok, fill args array.
 * Returns 1 if background job, 0 otherwise. */
int     parse_cmdline(char *line, char **args);

/* Look up and run a built-in command.
 * Returns 1 if matched, 0 otherwise. */
int     run_builtin(char **args);

/* Fork a child and execvp the external command. */
void    spawn_process(char **args, int is_bg, char *line);

/* Top-level handler: parse and dispatch a command line. */
void    execute_cmd(char *line);

#endif /* __MYSHELL_H__ */