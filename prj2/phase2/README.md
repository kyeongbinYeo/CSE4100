CSE4100 - System Programming Project 2: Phase 1
==============================================

Project Title
-------------
Simple Unix Shell Implementation - Phase 1

Files Provided
--------------
1. myshell.c         : Main shell implementation source file.
2. myshell.h         : Header file containing constants and function prototypes.
3. csapp.c / csapp.h : Helper library (CS:APP3e) providing wrapper functions.
4. Makefile          : Compilation instructions to build the shell program.

Compilation
-----------
To compile the shell:
    $ make

This will produce an executable named:
    myshell

Usage
-----
To run the shell:
    $ ./myshell

You will see a prompt:
    CSE4100-SP-P2>

At this prompt, you can enter commands as you would in a normal Unix shell.

Supported Features (Phase 1)
----------------------------
The following functionalities are fully supported:

1. **External Command Execution**
    - Supports both absolute and relative paths.
    - Example:
        ```
        CSE4100-SP-P2> /bin/ls
        CSE4100-SP-P2> ./a.out
        ```

2. **PATH Search**
    - Commands like `ls`, `cat`, etc. are found in PATH directories.
    - Example:
        ```
        CSE4100-SP-P2> ls
        ```

3. **Built-in Commands**
    - `cd [directory]`: Change working directory.
        ```
        CSE4100-SP-P2> cd ..
        ```
    - `exit` / `quit`: Exit the shell.
        ```
        CSE4100-SP-P2> exit
        ```
    - `history`: Show past executed commands with line numbers.
        ```
        CSE4100-SP-P2> history
        ```
    - `&`: Background execution support.
        ```
        CSE4100-SP-P2> ls &
        [1234] ls &
        ```

4. **Error Handling**
    - Invalid commands will show:
        ```
        somecmd: 명령어를 찾을 수 없습니다.
        ```
    - `cd` without argument or to non-existent directory shows error message.

Limitations
-----------
- Pipe (`|`), history re-execution (`!!`, `!N`), redirection (`<`, `>`) are not supported in Phase 1.
- Job control (`fg`, `bg`, job listing) is not supported.

How to Clean
------------
To remove all compiled files:
    $ make clean

Author
------
cse20231566 (여경빈)

Date
----
2024년 4월 18일
