#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#define MAX_ARGS    128
#define MAX_LINE    8192
#define SHELL_PROMPT "CSE4100-SP-P2> "

/* Parse command line into argument array, return 1 if background job */
int     parse_cmdline(char *line, char **args);

/* Check and execute built-in commands, return 1 if built-in */
int     run_builtin(char **args);

/* Main command evaluation function */
void    execute_cmd(char *line);

#endif /* __MYSHELL_H__ */