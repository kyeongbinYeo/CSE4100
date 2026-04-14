#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#define MAXARGS 128
#define MAXLINE 8192
#define HISTORY_LIMIT 1000
#define PROMPT "CSE4100-SP-P2> "

void eval(char *cmdline);
int builtin_command(char **argv);
int parseline(char *buf, char **argv);
void add_history(const char *cmd);
int handle_history_reexec(char *cmd);

#endif /* __MYSHELL_H__ */