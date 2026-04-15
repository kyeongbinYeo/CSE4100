# Phase 3: 백그라운드 실행 및 Job Control 구현

## 프로젝트 소개

Phase 3에서는 Phase 2까지 만든 쉘에 백그라운드 실행과 Job Control 기능을 추가했다. 이제 명령어 끝에 `&`를 붙이면 백그라운드에서 실행되고, `jobs`, `fg`, `bg`, `kill` 명령어로 실행 중인 프로세스들을 관리할 수 있다.

또한 Ctrl+C와 Ctrl+Z 시그널을 처리해서 foreground 프로세스를 종료하거나 정지시킬 수 있다.

## 구현한 기능

### Phase 1, 2 기능 (유지)
- `cd`, `exit`, `quit`: 내장 명령어
- `ls`, `mkdir`, `cat`, `echo` 등: 외부 명령어
- 파이프(|): 명령어 간 연결

### Phase 3 추가 기능
- 백그라운드 실행(`&`): 명령어를 백그라운드에서 실행
- `jobs`: 현재 실행 중인 job 목록 출력
- `fg <job_id>`: 백그라운드/정지된 job을 foreground로 가져옴
- `bg <job_id>`: 정지된 job을 백그라운드에서 재개
- `kill <job_id>`: job 종료
- Ctrl+C: foreground 프로세스 종료 (SIGINT)
- Ctrl+Z: foreground 프로세스 정지 (SIGTSTP)

## 주요 함수 설명

### Job 관리 함수들
- `init_jobs()`: job 리스트 초기화
- `add_job()`: 새 job 추가, 가장 작은 jid 할당
- `delete_job()`: job 삭제
- `find_job_by_jid()`: jid로 job 찾기
- `list_jobs()`: 현재 job 목록 출력

### 시그널 핸들러
- `sigchld_handler()`: 자식 프로세스가 종료되거나 정지되면 호출됨. WIFSTOPPED로 정지인지 종료인지 구분해서 처리한다.
- `sigint_handler()`: Ctrl+C 누르면 foreground 프로세스 그룹에 SIGINT 전달
- `sigtstp_handler()`: Ctrl+Z 누르면 foreground 프로세스 그룹에 SIGTSTP 전달

### 빌트인 명령어 핸들러
- `handler_fg()`: 백그라운드/정지된 job을 foreground로 가져와서 SIGCONT로 재개
- `handler_bg()`: 정지된 job에 SIGCONT 보내서 백그라운드에서 재개
- `handler_kill()`: job에 SIGTERM 보내서 종료

### check_done_jobs()
백그라운드에서 종료된 job들을 확인하고 "Terminated" 메시지를 출력한다. 실제 Linux bash처럼 다음 명령어 입력 전에 종료된 job을 알려준다.

## Job 상태 관리

job은 다음 4가지 상태를 가진다:
- `UNDEF`: 미정의 (빈 슬롯)
- `RUNNING`: 실행 중
- `STOPPED`: 정지됨 (Ctrl+Z)
- `DONE`: 종료됨 (백그라운드 job이 끝났을 때)

상태 전이:
```
RUNNING(fg) --Ctrl+Z--> STOPPED --bg--> RUNNING(bg)
     |                     |
     |                     +--fg--> RUNNING(fg)
     |
     +--Ctrl+C--> 종료 (delete)
```

## 시그널 처리 원리

fork() 전에 시그널을 블록해서 race condition을 방지했다. 자식 프로세스는 새로운 프로세스 그룹을 만들어서 쉘과 분리했다.

```
쉘(부모)                     자식 프로세스
   |                              |
sigprocmask(BLOCK)               |
   |                              |
fork() -------------------------→ setpgid(0,0) // 새 프로세스 그룹
   |                              |
add_job()                        execvp()
   |                              |
sigprocmask(UNBLOCK)             |
   |                              |
sigsuspend() <---- SIGCHLD ------+
```

## 컴파일 및 실행

```bash
make        # 컴파일
./myshell   # 실행
```

## 사용 방법

### 백그라운드 실행 (&)

```bash
# 명령어 끝에 & 붙이면 백그라운드 실행
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &

# 프롬프트가 바로 나와서 다른 작업 가능
CSE4100-SP-P2> ls
Makefile  myshell  myshell.c  myshell.h

# 파이프라인도 백그라운드 실행 가능
CSE4100-SP-P2> sleep 50 | sleep 50 &
[2] 12350 sleep 50 | sleep 50 &
```

### jobs 명령어

```bash
# 현재 실행 중인 job 목록 확인
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> sleep 200 &
[2] 12346 sleep 200 &
CSE4100-SP-P2> jobs
[1] 12345 Running sleep 100 &
[2] 12346 Running sleep 200 &
```

### fg 명령어

```bash
# 백그라운드 job을 foreground로 가져오기
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> fg 1
(sleep이 foreground에서 실행됨, 끝날 때까지 대기)

# 정지된 job을 foreground로 가져오기
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> fg 1
^Z
[1] 12345 Stopped sleep 100 &
CSE4100-SP-P2> fg 1
(다시 foreground에서 실행됨)
```

### bg 명령어

```bash
# 정지된 job을 백그라운드에서 재개
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> fg 1
^Z
[1] 12345 Stopped sleep 100 &
CSE4100-SP-P2> jobs
[1] 12345 Stopped sleep 100 &
CSE4100-SP-P2> bg 1
[1] 12345 sleep 100 &
CSE4100-SP-P2> jobs
[1] 12345 Running sleep 100 &
```

### kill 명령어

```bash
# job 종료
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> sleep 200 &
[2] 12346 sleep 200 &
CSE4100-SP-P2> jobs
[1] 12345 Running sleep 100 &
[2] 12346 Running sleep 200 &
CSE4100-SP-P2> kill 1
CSE4100-SP-P2> (Enter)
[1] 12345 Terminated sleep 100 &
CSE4100-SP-P2> jobs
[2] 12346 Running sleep 200 &
```

### Ctrl+C (SIGINT) - 프로세스 종료

```bash
# foreground 프로세스 강제 종료
CSE4100-SP-P2> sleep 100
^C
CSE4100-SP-P2>

# 백그라운드에서 가져온 후 종료
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> fg 1
^C
CSE4100-SP-P2>
```

### Ctrl+Z (SIGTSTP) - 프로세스 정지

```bash
# foreground 프로세스 정지
CSE4100-SP-P2> sleep 100
^Z
[1] 12345 Stopped sleep 100
CSE4100-SP-P2> jobs
[1] 12345 Stopped sleep 100

# fg나 bg로 다시 실행 가능
CSE4100-SP-P2> bg 1
[1] 12345 sleep 100
CSE4100-SP-P2> jobs
[1] 12345 Running sleep 100
```

### 존재하지 않는 job 처리

```bash
CSE4100-SP-P2> fg 99
fg: job 99 not found
CSE4100-SP-P2> bg 99
bg: job 99 not found
CSE4100-SP-P2> kill 99
kill: job 99 not found
```

### 여러 job 동시 관리

```bash
CSE4100-SP-P2> sleep 100 &
[1] 12345 sleep 100 &
CSE4100-SP-P2> sleep 200 &
[2] 12346 sleep 200 &
CSE4100-SP-P2> sleep 300 &
[3] 12347 sleep 300 &
CSE4100-SP-P2> jobs
[1] 12345 Running sleep 100 &
[2] 12346 Running sleep 200 &
[3] 12347 Running sleep 300 &
CSE4100-SP-P2> kill 2
CSE4100-SP-P2> (Enter)
[2] 12346 Terminated sleep 200 &
CSE4100-SP-P2> jobs
[1] 12345 Running sleep 100 &
[3] 12347 Running sleep 300 &
```

### Phase 1, 2 명령어도 그대로 동작

```bash
CSE4100-SP-P2> ls -al | grep myshell
-rwxr-xr-x 1 user user 71128 Apr 15 10:00 myshell
-rw-r--r-- 1 user user 14353 Apr 15 10:00 myshell.c

CSE4100-SP-P2> echo "hello world"
hello world

CSE4100-SP-P2> cd ..
CSE4100-SP-P2> pwd
/home/user
```

### 쉘 종료

```bash
CSE4100-SP-P2> exit
```

## 배운 점

- 시그널 핸들러 작성 시 async-signal-safe 함수만 사용해야 한다는 걸 알게 됐다
- fork() 전에 시그널을 블록해서 race condition을 방지하는 방법을 배웠다
- 프로세스 그룹과 setpgid()의 중요성을 이해했다 (Ctrl+C가 자식에게만 가도록)
- sigsuspend()로 시그널을 기다리는 방법을 배웠다
- 실제 bash가 job을 어떻게 관리하는지 이해하게 됐다