# Phase 1: 기본 쉘 구현

## 프로젝트 소개

이번 Phase 1에서는 간단한 Linux 쉘을 만들었다. 쉘이란 사용자가 명령어를 입력하면 그걸 실행해주는 프로그램이다.

평소에 터미널에서 `ls`나 `cd` 같은 명령어를 치면 실행되는 게 바로 쉘 덕분이다.

## 구현한 기능

### 내장 명령어
- `cd <경로>`: 디렉토리 이동 (chdir 함수 사용)
- `exit`: 쉘 종료
- `quit`: 쉘 종료 (exit과 동일)

### 외부 명령어
`ls`, `mkdir`, `rmdir`, `touch`, `cat`, `echo` 등 일반적인 리눅스 명령어들은 fork()로 자식 프로세스를 만들고 execvp()로 실행했다.

## 주요 함수 설명

### main()
쉘의 메인 루프다. 프롬프트 출력 → 입력 받기 → 실행을 무한 반복한다.

### eval()
입력받은 명령어를 실행하는 함수다. 내장 명령어면 바로 실행하고, 아니면 fork() 해서 자식 프로세스에서 실행한다.

### parseline()
사용자 입력을 공백 기준으로 잘라서 argv 배열에 저장한다. 따옴표로 묶인 문자열은 하나의 토큰으로 처리해서 `echo "hello world"` 같은 명령어도 제대로 동작한다.

### builtin_command()
cd, exit 같은 내장 명령어인지 확인하고 맞으면 실행한다.

## 실행 원리

1. 부모 프로세스(쉘)가 fork()로 자식 프로세스 생성
2. 자식 프로세스가 execvp()로 명령어 실행
3. 부모는 waitpid()로 자식이 끝날 때까지 대기

예를 들어 `ls` 명령어를 치면:
```
쉘(부모) --fork()--> 자식 프로세스 --execvp()--> ls 실행
   |                                              |
   +-------waitpid()로 대기 <-------- 종료 -------+
```

## 컴파일 및 실행

```bash
make        # 컴파일
./myshell   # 실행
```

## 사용 방법

### 디렉토리 관련 명령어

```bash
# 현재 디렉토리 파일 목록 보기
CSE4100-SP-P2> ls
Makefile  myshell  myshell.c  myshell.h

# 자세한 파일 정보 보기
CSE4100-SP-P2> ls -al
total 48
drwxr-xr-x 2 user user 4096 Apr 15 10:00 .
drwxr-xr-x 5 user user 4096 Apr 15 09:00 ..
-rw-r--r-- 1 user user  336 Apr 15 10:00 Makefile
-rwxr-xr-x 1 user user 62616 Apr 15 10:00 myshell

# 현재 경로 확인
CSE4100-SP-P2> pwd
/home/user/phase1

# 디렉토리 이동
CSE4100-SP-P2> cd ..
CSE4100-SP-P2> pwd
/home/user

# 디렉토리 생성
CSE4100-SP-P2> mkdir testdir
CSE4100-SP-P2> ls
phase1  testdir

# 디렉토리 삭제
CSE4100-SP-P2> rmdir testdir
```

### 파일 관련 명령어

```bash
# 빈 파일 생성
CSE4100-SP-P2> touch hello.txt
CSE4100-SP-P2> ls
hello.txt  Makefile  myshell  myshell.c

# 파일 내용 보기
CSE4100-SP-P2> cat myshell.c
#include "csapp.h"
#include "myshell.h"
...
```

### echo 명령어

```bash
# 일반 출력
CSE4100-SP-P2> echo hello
hello

# 따옴표로 묶으면 공백 포함 가능
CSE4100-SP-P2> echo "hello world"
hello world

# 여러 단어 출력
CSE4100-SP-P2> echo hello world test
hello world test
```

### 쉘 종료

```bash
# exit 또는 quit으로 종료
CSE4100-SP-P2> exit
```

### 존재하지 않는 명령어

```bash
CSE4100-SP-P2> asdfgh
asdfgh: command not found
```

## 배운 점

- fork()와 exec()가 어떻게 같이 동작하는지 이해했다
- 쉘이 내부적으로 어떤 구조로 되어있는지 알게 됐다
- 문자열 파싱하는 게 생각보다 까다로웠다 (특히 따옴표 처리)