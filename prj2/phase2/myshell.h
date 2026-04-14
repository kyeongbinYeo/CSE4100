#ifndef __MYSHELL_H__
#define __MYSHELL_H__

#define MAXARGS     128
#define MAXLINE     8192
#define PROMPT      "CSE4100-SP-P2> "

// builtin cmd table struct
typedef struct {
    const char *name;
    void (*handler)(char **argv);
} cmd_entry_t;

int     read_input(char *cmdline);  // print prompt & get input
void    eval(char *cmdline);
int     parseline(char *buf, char **argv);
int     builtin_command(char **argv);  // returns 1 if builtin

#endif