# mysh - A UNIX Shell in C

A UNIX Shell implemented from scratch in C, supporting external command execution, built-in commands, I/O redirection, pipes, background execution, and automatic zombie reaping via SIGCHLD handler.

## Features

- External command execution (ls, cat, echo, etc.)
- Built-in commands: `cd`, `exit`
- I/O redirection: `<`, `>`
- Single pipe: `|`
- Background execution: `&`
- SIGCHLD handler for automatic zombie reaping

## Build

```
make
```

Or compile directly with gcc:

```
gcc -Wall -Wextra -g -o mysh mysh.c
```

## Run

```
./mysh
```

## Usage Examples

```
mysh> ls -l
mysh> echo hello world
mysh> echo hello > output.txt
mysh> cat < output.txt
mysh> ls | wc -l
mysh> cat file.txt | grep hello
mysh> sleep 30 &
mysh> cd /tmp
mysh> pwd
mysh> exit
```

## Architecture

```
+-----------------------------------------------------+
|                     main loop                        |
|  1. Print prompt "mysh> "                            |
|  2. Read input (fgets)                               |
|  3. Strip newline (strcspn)                          |
|  4. Tokenize input (strtok)                          |
|  5. Handle built-in commands (cd, exit)              |
|  6. Check for background execution (&)               |
|  7. Check for pipe (|)                               |
|  8. Dispatch:                                        |
|     - pipe present -> execute_pipe()                 |
|     - no pipe      -> parse_redirect() + execute_command() |
|  9. Loop back to step 1                              |
+-----------------------------------------------------+
```

## System Calls Used

| System Call | Purpose |
|-------------|---------|
| `fork()` | Duplicate a process |
| `execvp()` | Execute an external program |
| `waitpid()` | Wait for a child process |
| `pipe()` | Create a unidirectional pipe |
| `dup2()` | Duplicate a file descriptor |
| `open()` | Open a file |
| `close()` | Close a file descriptor |
| `chdir()` | Change working directory |
| `sigaction()` | Register SIGCHLD handler |

## Technical Stack

C, Linux system programming, fork, exec, wait, pipe, dup2, open, close, signal, sigaction, SIGCHLD, file descriptors

## Debugging Tools

- `gdb`: trace fork, exec, pipe
- `strace`: inspect system calls
- `valgrind`: check for memory leaks

## Notes

- Single pipe supported (multi-pipe not implemented)
- Quotes and environment variable expansion not supported
- Uses fixed-size arrays (no dynamic memory allocation)
