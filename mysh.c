/*
 * mysh.c - A Simple UNIX Shell implemented in C
 *
 * Features:
 *   - External command execution (fork + execvp + wait)
 *   - Built-in commands: cd, exit
 *   - I/O redirection: <, >
 *   - Single pipe: |
 *   - Background execution: &
 *   - SIGCHLD handler for automatic zombie reaping
 *
 * Author: [Benny Tong]
 * Date: 2026-10
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>

#define MAX_ARGS 64

/*
 * SIGCHLD handler: reap all terminated child processes.
 *
 * Uses WNOHANG to avoid blocking when no child has exited.
 * Loops because multiple children may exit between signal deliveries.
 * Saves and restores errno to avoid corrupting the main program's errno.
 */
void sigchld_handler(int sig) {
    (void)sig;
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        /* reap all terminated children */
    }
    errno = saved_errno;
}

/*
 * execute_command - Execute a single command with optional redirection.
 *
 * @args:        NULL-terminated array of command and arguments
 * @input_file:  input file path for redirection, or NULL
 * @output_file: output file path for redirection, or NULL
 * @background:  1 if run in background, 0 if foreground
 *
 * Returns 0 on success, -1 on fork failure.
 */
int execute_command(char **args, char *input_file, char *output_file, int background) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* Input redirection: make fd 0 point to the input file */
        if (input_file != NULL) {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        /* Output redirection: make fd 1 point to the output file */
        if (output_file != NULL) {
            int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("open");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        execvp(args[0], args);
        perror("execvp");
        exit(1);  /* exit child if execvp fails */
    }

    /* Parent: wait for foreground, print PID for background */
    if (!background) {
        waitpid(pid, NULL, 0);
    } else {
        printf("[1] %d\n", pid);
    }
    return 0;
}

/*
 * execute_pipe - Execute a pipeline: args1 | args2
 *
 * Creates a pipe and forks two children:
 *   - child 1: stdout -> pipe write-end, runs args1
 *   - child 2: stdin  -> pipe read-end,  runs args2
 *
 * The parent must close both pipe ends. Otherwise, child 2's read-end
 * will never receive EOF because the parent still holds the write-end.
 *
 * @args1:      left command
 * @args2:      right command
 * @background: 1 if run in background, 0 if foreground
 *
 * Returns 0 on success, -1 on pipe or fork failure.
 */
int execute_pipe(char **args1, char **args2, int background) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("fork");
        return -1;
    }
    if (pid1 == 0) {
        /* Child 1: stdout -> pipe write-end */
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(args1[0], args1);
        perror("execvp");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("fork");
        return -1;
    }
    if (pid2 == 0) {
        /* Child 2: stdin -> pipe read-end */
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(args2[0], args2);
        perror("execvp");
        exit(1);
    }

    /* Parent must close both ends; otherwise child 2 never sees EOF */
    close(pipefd[0]);
    close(pipefd[1]);

    if (!background) {
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);
    } else {
        printf("[1] %d\n", pid1);
        printf("[2] %d\n", pid2);
    }
    return 0;
}

/*
 * parse_redirect - Extract < and > redirection from args.
 *
 * Uses char** output parameters to modify the caller's variables.
 * Removes redirection tokens and their filenames from args.
 *
 * @args:        token array
 * @argc:        token count
 * @input_file:  output parameter for input filename
 * @output_file: output parameter for output filename
 *
 * Returns the new argc after removing redirection tokens.
 */
int parse_redirect(char **args, int argc, char **input_file, char **output_file) {
    int new_argc = 0;
    int i;
    for (i = 0; i < argc; i++) {
        if (strcmp(args[i], "<") == 0 && i + 1 < argc) {
            *input_file = args[i + 1];
            i++;
        } else if (strcmp(args[i], ">") == 0 && i + 1 < argc) {
            *output_file = args[i + 1];
            i++;
        } else {
            args[new_argc++] = args[i];
        }
    }
    args[new_argc] = NULL;
    return new_argc;
}

int main() {
    char line[1024];

    /*
     * Register SIGCHLD handler with SA_RESTART.
     * SA_RESTART ensures fgets() auto-restarts if interrupted by SIGCHLD.
     * Without it, fgets() returns NULL and the shell exits unexpectedly.
     */
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    while (1) {
        printf("mysh> ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            printf("\n");
            break;
        }

        /* Strip trailing newline */
        line[strcspn(line, "\n")] = '\0';

        /* Tokenize input */
        char *args[MAX_ARGS];
        int argc = 0;
        char *token = strtok(line, " ");
        while (token != NULL && argc < MAX_ARGS - 1) {
            args[argc++] = token;
            token = strtok(NULL, " ");
        }
        args[argc] = NULL;

        if (argc == 0) continue;

        /* Built-in: exit */
        if (strcmp(args[0], "exit") == 0) break;

        /*
         * Built-in: cd
         * cd must run in the shell process, not a child.
         * chdir() only affects the calling process, so forking
         * would change the child's cwd, not the shell's.
         */
        if (strcmp(args[0], "cd") == 0) {
            if (argc < 2) {
                char *home = getenv("HOME");
                if (home != NULL) chdir(home);
            } else {
                if (chdir(args[1]) != 0) perror("cd");
            }
            continue;
        }

        /* Check for background execution (&) */
        int background = 0;
        if (argc > 0 && strcmp(args[argc - 1], "&") == 0) {
            background = 1;
            args[argc - 1] = NULL;
            argc--;
        }
        if (argc == 0) continue;

        /* Check for pipe (|) */
        int has_pipe = 0;
        int pipe_pos = 0;
        int k;
        for (k = 0; k < argc; k++) {
            if (strcmp(args[k], "|") == 0) {
                has_pipe = 1;
                pipe_pos = k;
                break;
            }
        }

        if (has_pipe) {
            /* Split into args1 | args2 */
            char *args1[MAX_ARGS];
            char *args2[MAX_ARGS];
            int argc1 = 0, argc2 = 0;

            for (k = 0; k < pipe_pos; k++) {
                args1[argc1++] = args[k];
            }
            args1[argc1] = NULL;

            for (k = pipe_pos + 1; k < argc; k++) {
                args2[argc2++] = args[k];
            }
            args2[argc2] = NULL;

            if (argc1 == 0 || argc2 == 0) {
                fprintf(stderr, "mysh: syntax error near '|'\n");
                continue;
            }

            execute_pipe(args1, args2, background);

        } else {
            /* No pipe: parse redirection then execute */
            char *input_file = NULL;
            char *output_file = NULL;
            argc = parse_redirect(args, argc, &input_file, &output_file);

            if (argc == 0) continue;

            execute_command(args, input_file, output_file, background);
        }
    }

    return 0;
}
