# Phase 2: 파이프라인 구현

## 프로젝트 소개

Phase 2에서는 Phase 1에서 만든 쉘에 파이프(|) 기능을 추가했다. 파이프란 왼쪽 명령어의 출력을 오른쪽 명령어의 입력으로 연결해주는 기능이다.

예를 들어 `ls | grep myshell`을 실행하면 ls의 출력 결과에서 "myshell"이 포함된 줄만 필터링해서 보여준다.

## 구현한 기능

### Phase 1 기능 (유지)
- `cd`: 디렉토리 이동
- `exit`, `quit`: 쉘 종료
- `ls`, `mkdir`, `rmdir`, `touch`, `cat`, `echo` 등 외부 명령어

### Phase 2 추가 기능
- 파이프(|): 명령어 간 출력/입력 연결
- 다중 파이프: `cmd1 | cmd2 | cmd3` 형태로 여러 개 연결 가능

## 주요 함수 설명

### split_pipes()
입력된 명령어를 `|` 기준으로 분리한다. 예를 들어 `ls | grep test`를 입력하면 `cmds[0] = "ls"`, `cmds[1] = "grep test"`로 나눠진다.

### run_pipeline()
파이프라인을 재귀적으로 실행하는 함수다. 핵심 로직은 다음과 같다:
- 명령어가 1개면 그냥 execvp()로 실행
- 명령어가 여러 개면 pipe()로 파이프 생성 후 fork()로 자식 2개 생성
- 왼쪽 자식: 첫 번째 명령어 실행, stdout을 파이프에 연결
- 오른쪽 자식: 나머지 파이프라인을 재귀 호출, stdin을 파이프에 연결

### eval()
Phase 1과 동일하지만, 파이프가 있는지 먼저 확인한다. 파이프가 있으면 run_pipeline()을 호출하고, 없으면 기존처럼 단일 명령어를 실행한다.

## 파이프 실행 원리

`ls | grep myshell` 실행 시:

```
        pipe()
          |
    +-----+-----+
    |           |
    v           v
[자식1]       [자식2]
ls 실행      grep 실행
stdout→파이프  파이프→stdin
    |           ^
    +-----------+
      데이터 흐름
```

1. pipe()로 파이프 생성 (읽기용, 쓰기용 fd 2개)
2. fork()로 자식1 생성 → ls 실행, stdout을 파이프 쓰기 끝에 연결
3. fork()로 자식2 생성 → grep 실행, stdin을 파이프 읽기 끝에 연결
4. 부모는 두 자식이 끝날 때까지 waitpid()로 대기

## 컴파일 및 실행

```bash
make        # 컴파일
./myshell   # 실행
```

## 사용 방법

### 기본 파이프

```bash
# ls 결과에서 myshell 포함된 줄만 출력
CSE4100-SP-P2> ls | grep myshell
myshell
myshell.c
myshell.h
myshell.o

# ls -al 결과에서 필터링
CSE4100-SP-P2> ls -al | grep myshell
-rwxr-xr-x 1 user user 67784 Apr 15 10:00 myshell
-rw-r--r-- 1 user user  6656 Apr 15 10:00 myshell.c
-rw-r--r-- 1 user user   694 Apr 15 10:00 myshell.h
```

### 다중 파이프

```bash
# ls 결과에서 myshell 필터링 후 줄 수 세기
CSE4100-SP-P2> ls -al | grep myshell | wc -l
4

# 파일 내용에서 특정 문자열 검색
CSE4100-SP-P2> cat myshell.c | grep include
#include "csapp.h"
#include "myshell.h"
```

### grep 옵션 사용

```bash
# -i 옵션: 대소문자 무시
CSE4100-SP-P2> cat myshell.c | grep -i "int"
int read_input(char *cmdline)
int split_pipes(char *buf, char *cmds[], int max)
int parseline(char *buf, char **argv)
int builtin_command(char **argv)
int main(void)
```

### Phase 1 명령어도 그대로 동작

```bash
CSE4100-SP-P2> ls
Makefile  myshell  myshell.c  myshell.h

CSE4100-SP-P2> cd ..
CSE4100-SP-P2> pwd
/home/user

CSE4100-SP-P2> echo "hello world"
hello world

CSE4100-SP-P2> mkdir testdir
CSE4100-SP-P2> rmdir testdir
```

### 존재하지 않는 명령어

```bash
CSE4100-SP-P2> asdfgh | grep test
asdfgh: command not found
```

### 쉘 종료

```bash
CSE4100-SP-P2> exit
```

## 배운 점

- pipe() 시스템 콜로 프로세스 간 통신하는 방법을 배웠다
- dup2()로 표준 입출력을 다른 파일 디스크립터로 리다이렉션하는 방법을 알게 됐다
- 재귀적으로 파이프라인을 처리하는 방식이 코드를 깔끔하게 만들어준다는 걸 느꼈다
- 파이프 사용 후 꼭 close()를 해줘야 한다는 걸 배웠다 (안 하면 EOF가 전달 안 됨)