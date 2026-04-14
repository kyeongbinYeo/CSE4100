#include "csapp.h"
#include "myshell.h"

/*
 * 전역 변수들
 */
job_t           job_list[MAXJOBS];  // job 리스트 배열
volatile pid_t  fg_pid = 0;         // 현재 foreground 프로세스 pid
sigset_t        mask, prev;         // 시그널 마스크

/*
 * init_jobs - job 리스트 초기화
 */
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

/*
 * add_job - 새로운 job을 리스트에 추가
 * 가장 작은 사용 가능한 jid를 할당함
 */
int add_job(pid_t pid, int state, char *cmdline)
{
    int i, jid = 1;

    // 사용 가능한 가장 작은 jid 찾기
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

/*
 * delete_job - pid에 해당하는 job 삭제
 */
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

/*
 * find_job_by_pid - pid로 job 찾기
 */
job_t *find_job_by_pid(pid_t pid)
{
    int i;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].pid == pid)
            return &job_list[i];
    return NULL;
}

/*
 * find_job_by_jid - jid로 job 찾기
 */
job_t *find_job_by_jid(int jid)
{
    int i;
    for (i = 0; i < MAXJOBS; i++)
        if (job_list[i].jid == jid)
            return &job_list[i];
    return NULL;
}

/*
 * list_jobs - 현재 job 목록 출력
 */
void list_jobs(void)
{
    int i;
    const char *state_str;

    for (i = 0; i < MAXJOBS; i++) {
        if (job_list[i].pid == 0) continue;
        switch (job_list[i].state) {
            case RUNNING: state_str = "Running";     break;
            case STOPPED: state_str = "Stopped";     break;
            case DONE:    state_str = "Done";        break;
            default:      state_str = "Unknown";     break;
        }
        printf("[%d] %d %s %s", job_list[i].jid, job_list[i].pid,
               state_str, job_list[i].cmdline);
    }
}

/*
 * sigchld_handler - SIGCHLD 시그널 핸들러
 * 자식 프로세스가 종료되거나 정지되면 호출됨
 */
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
            // 자식이 정지됨 (Ctrl+Z)
            job->state = STOPPED;
            printf("\n[%d] %d Stopped %s", job->jid, job->pid, job->cmdline);
            fflush(stdout);
            if (job->pid == fg_pid)
                fg_pid = 0;
        } else {
            // 자식이 종료됨
            if (pid == fg_pid) {
                fg_pid = 0;
            } else {
                // 백그라운드 job 종료시 프롬프트 다시 출력
                printf("\n%s", PROMPT);
                fflush(stdout);
            }
            delete_job(pid);
        }
    }
}

/*
 * sigint_handler - SIGINT 시그널 핸들러 (Ctrl+C)
 * foreground job 종료
 */
void sigint_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0) {
        Kill(-fg_pid, SIGINT);
        Sio_puts("\n");
    }
}

/*
 * sigtstp_handler - SIGTSTP 시그널 핸들러 (Ctrl+Z)
 * foreground job 정지
 */
void sigtstp_handler(int sig)
{
    (void)sig;
    if (fg_pid > 0)
        Kill(-fg_pid, SIGTSTP);
}

/*
 * handler_exit - exit/quit 명령어 처리
 */
static void handler_exit(char **argv)
{
    (void)argv;
    exit(0);
}

/*
 * handler_cd - cd 명령어 처리
 */
static void handler_cd(char **argv)
{
    if (argv[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return;
    }
    if (chdir(argv[1]) != 0)
        perror("cd");
}

/*
 * handler_jobs - jobs 명령어 처리
 */
static void handler_jobs(char **argv)
{
    (void)argv;
    list_jobs();
}

/*
 * handler_fg - fg 명령어 처리
 * 백그라운드/정지된 job을 foreground로 가져옴
 */
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

    // job이 삭제되기 전에 pid 저장
    pid = job->pid;
    fg_pid = pid;
    job->state = RUNNING;
    Kill(-pid, SIGCONT);

    // foreground에서 대기
    sigset_t empty;
    sigemptyset(&empty);
    sigprocmask(SIG_SETMASK, &empty, NULL);
    while (fg_pid == pid)
        sigsuspend(&empty);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

/*
 * handler_bg - bg 명령어 처리
 * 정지된 job을 백그라운드에서 재개
 */
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
    Kill(-job->pid, SIGCONT);
    printf("[%d] %d %s", job->jid, job->pid, job->cmdline);
}

/*
 * handler_kill - kill 명령어 처리
 * job 종료
 */
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

    Kill(-job->pid, SIGTERM);
}

// 빌트인 명령어 테이블
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

/*
 * read_input - 사용자 입력 읽기
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
 * split_pipes - 파이프(|)로 명령어 분리
 */
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

/*
 * parseline - 명령어 라인 파싱
 * 공백으로 토큰 분리, 따옴표 처리, 백그라운드(&) 확인
 * 반환값: 백그라운드 실행이면 1, 아니면 0
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
            // 따옴표로 묶인 토큰 처리
            ptr++;
            argv[nargs++] = ptr;
            while (*ptr != '"' && *ptr != '\0') ptr++;
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

/*
 * builtin_command - 빌트인 명령어인지 확인하고 실행
 * 반환값: 빌트인이면 1, 아니면 0
 */
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

/*
 * run_pipeline - 파이프라인 실행
 * 재귀적으로 파이프 연결하여 명령어들 실행
 */
void run_pipeline(char *cmds[], int n_cmds, pid_t pgid)
{
    char   *argv[MAXARGS];
    char    cmd_buf[MAXLINE];
    int     pipe_fd[2];
    pid_t   left_pid, right_pid;
    int     status;

    // 기본 케이스: 명령어가 하나만 남음
    if (n_cmds == 1) {
        strncpy(cmd_buf, cmds[0], MAXLINE - 1);
        cmd_buf[MAXLINE - 1] = '\0';
        parseline(cmd_buf, argv);

        if (argv[0] == NULL)
            exit(0);

        setpgid(0, pgid);

        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%s: command not found\n", argv[0]);
            exit(1);
        }
    }

    // 파이프 생성
    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(1);
    }

    // 왼쪽 자식: 첫 번째 명령어 실행
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

    // 오른쪽 자식: 나머지 파이프라인 실행
    right_pid = Fork();
    if (right_pid == 0) {
        close(pipe_fd[1]);
        dup2(pipe_fd[0], STDIN_FILENO);
        close(pipe_fd[0]);

        run_pipeline(cmds + 1, n_cmds - 1, pgid);
        exit(0);
    }

    // 부모: 파이프 닫고 자식들 대기
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    waitpid(left_pid, &status, 0);
    waitpid(right_pid, &status, 0);
}

/*
 * eval - 명령어 평가 및 실행
 * 파이프라인, 백그라운드, 빌트인 명령어 처리
 */
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

    // fork 전에 시그널 블록 (race condition 방지)
    sigprocmask(SIG_BLOCK, &mask, &prev);

    n_cmds = split_pipes(pipe_buf, cmds, MAXCMDS);

    // 파이프라인 처리
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
            // foreground 실행
            add_job(child_pid, RUNNING, cmdline);
            fg_pid = child_pid;
            sigprocmask(SIG_SETMASK, &prev, NULL);
            sigset_t empty;
            sigemptyset(&empty);
            while (fg_pid == child_pid)
                sigsuspend(&empty);
        } else {
            // 백그라운드 실행
            int jid = add_job(child_pid, RUNNING, cmdline);
            printf("[%d] %d %s", jid, child_pid, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
        return;
    }

    // 단일 명령어 처리
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
            // foreground 실행
            add_job(child_pid, RUNNING, cmdline);
            fg_pid = child_pid;
            sigprocmask(SIG_SETMASK, &prev, NULL);
            sigset_t empty;
            sigemptyset(&empty);
            while (fg_pid == child_pid)
                sigsuspend(&empty);
        } else {
            // 백그라운드 실행
            int jid = add_job(child_pid, RUNNING, cmdline);
            printf("[%d] %d %s", jid, child_pid, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
    } else {
        sigprocmask(SIG_SETMASK, &prev, NULL);
    }

    (void)exit_status;
}

/*
 * main - 쉘 메인 함수
 */
int main(void)
{
    char cmdline[MAXLINE];

    init_jobs();

    // 시그널 마스크 설정
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTSTP);
    sigprocmask(SIG_BLOCK, &mask, &prev);

    // 시그널 핸들러 등록
    Signal(SIGCHLD, sigchld_handler);
    Signal(SIGINT,  sigint_handler);
    Signal(SIGTSTP, sigtstp_handler);

    sigprocmask(SIG_SETMASK, &prev, NULL);

    // 메인 루프
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