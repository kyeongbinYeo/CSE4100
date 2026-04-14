#include "csapp.h"
#include "myshell.h"

#define PROMPT "CSE4100-SP-P2> "

char *history[HISTORY_LIMIT];
int history_count = 0;

/* Phase 1에서는 히스토리 재실행 기능은 없으므로 더미로 */
int handle_history_reexec(char *cmd) {
    return 0;
}

/* 명령어를 히스토리에 저장 (Phase 2 확장 대비) */
void add_history(const char *cmd) {
    if (history_count < HISTORY_LIMIT) {
        history[history_count++] = strdup(cmd);
    } else {
        free(history[0]);
        for (int i = 1; i < HISTORY_LIMIT; ++i)
            history[i - 1] = history[i];
        history[HISTORY_LIMIT - 1] = strdup(cmd);
    }
}

/* 명령어 실행 함수 */
void eval(char *cmdline) {
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL) return;

    if (handle_history_reexec(argv[0])) return;

    add_history(buf);

    if (!builtin_command(argv)) {
        if ((pid = Fork()) == 0) {
            // 절대/상대 경로 직접 실행
            if (argv[0][0] == '/' || argv[0][0] == '.') {
                execve(argv[0], argv, environ);
            } else {
                // PATH 탐색
                char *path_env = getenv("PATH");
                if (path_env != NULL) {
                    char *path_copy = strdup(path_env);
                    char *dir = strtok(path_copy, ":");
                    char fullpath[MAXLINE];
                    while (dir != NULL) {
                        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, argv[0]);
                        execve(fullpath, argv, environ);
                        dir = strtok(NULL, ":");
                    }
                    free(path_copy);
                }
            }
            fprintf(stderr, "%s: 명령어를 찾을 수 없습니다.\n", argv[0]);
            exit(1);
        }

        if (!bg) {
            int status;
            Waitpid(pid, &status, 0);
        } else {
            printf("[%d] %s", pid, cmdline);
        }
    }
}

/* 내장 명령어 처리 */
int builtin_command(char **argv) {
    if (!strcmp(argv[0], "exit") || !strcmp(argv[0], "quit"))
        exit(0);

    if (!strcmp(argv[0], "&"))
        return 1;

    if (!strcmp(argv[0], "cd")) {
        if (argv[1] == NULL)
            fprintf(stderr, "cd: 디렉토리를 지정해주세요.\n");
        else if (chdir(argv[1]) < 0)
            perror("cd 오류");
        return 1;
    }

    if (!strcmp(argv[0], "history")) {
        for (int i = 0; i < history_count; ++i)
            printf("%d\t%s\n", i + 1, history[i]);  // 개행 포함
        return 1;
    }

    return 0;
}

/* 명령어 파싱 */
int parseline(char *buf, char **argv) {
    int argc = 0;
    int bg;

    // 공백 제거
    while (*buf == ' ')
        buf++;

    // 인자 분리
    while (*buf != '\0') {
        while (*buf == ' ') buf++;
        if (*buf == '\0') break;

        argv[argc++] = buf;

        while (*buf && *buf != ' ' && *buf != '\n')
            buf++;

        if (*buf == '\0') break;

        *buf = '\0';
        buf++;
    }

    argv[argc] = NULL;

    if (argc == 0) return 1;

    bg = (argc > 0 && !strcmp(argv[argc - 1], "&"));
    if (bg)
        argv[--argc] = NULL;

    return bg;
}

/* 메인 루프 */
int main() {
    char cmdline[MAXLINE];

    while (1) {
        printf("%s", PROMPT);
        if (fgets(cmdline, MAXLINE, stdin) == NULL) {
            if (feof(stdin))
                exit(0);
        }
        eval(cmdline);
    }
}
