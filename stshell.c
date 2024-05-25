#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ARGLENGTH 20

// Signal handler function for SIGINT
void handle_sigint(int signum)
{
    // caught the ctrl c
}

// reorgenize the stdin and stdout to a file if we get >
void stdChangeRight(char *fileName, int isAppend) // 0 means > , 1 means >>
{
    int f;

    // Set the file permissions using octal representation (e.g., 0644)
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // File permissions: rw-r--r--

    if (isAppend)
    {
        f = open(fileName, O_WRONLY | O_APPEND | O_CREAT, mode);
    }
    else
    {
        f = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, mode);
    }
    dup2(f, 1); // making the file to be the output
    close(f);   // closing the file in the fd array
}

int main(int argc, char *argv[])
{
    // Register the signal handler
    signal(SIGINT, handle_sigint);

    int exitFlag = 1;

    while (exitFlag)
    {
        printf("\033[1;34mMaorAndRaz~stshell: \033[0m");
        // getting the command to run:
        char input[128] = {'\0'};
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0'; // remove trailing newline

        // if "exit" we exit the shell:
        if (strcmp("exit", input) == 0)
        {
            return 0;
        }

        // making an args list
        char *inputArgs[ARGLENGTH] = {NULL};
        char *token;
        int i = 0;
        token = strtok(input, " ");
        while (token != NULL && i < ARGLENGTH)
        {
            inputArgs[i++] = token;
            token = strtok(NULL, " ");
        }
        inputArgs[i] = NULL; // last element must be NULL

        // making a child to run the command:
        pid_t pid = fork();

        // the child proccess, we run the exec here
        if (pid == 0)
        {
            // scanning the args for any of the following: < ,> ,>> ,|
            // indexes for these signs:
            int isRight = -1, isDoubleRight = -1, isPipe[2] = {-1}, pipeIter = 0, pipeGate[2];

            for (int i = 0; i < ARGLENGTH; i++)
            {
                if (inputArgs[i] == NULL) // no need to run through all the 20 slots
                {
                    break;
                }
                else if (strcmp(">", inputArgs[i]) == 0)
                {
                    isRight = i;
                }
                else if (strcmp(">>", inputArgs[i]) == 0)
                {
                    isDoubleRight = i;
                }
                else if (strcmp("|", inputArgs[i]) == 0)
                {
                    if (pipeIter < 2)
                    {
                        isPipe[pipeIter++] = i;
                    }
                }
            }

            if (isRight != -1)
            {
                stdChangeRight(inputArgs[isRight + 1], 0);
                inputArgs[isRight] = NULL;
            }
            if (isDoubleRight != -1)
            {
                stdChangeRight(inputArgs[isDoubleRight + 1], 1);
                inputArgs[isDoubleRight] = NULL;
            }

            if (pipeIter == 1) // one pipe needed
            {
                pipe(pipeGate);
                pid_t granchild_pid = fork();

                if (granchild_pid == 0) // grandchild
                {
                    close(pipeGate[0]);
                    dup2(pipeGate[1], 1);

                    char *grandchildCommand[ARGLENGTH] = {NULL};

                    // gathering the grandchild's command to execute as a new array
                    for (int i = 0; i < isPipe[0]; i++)
                    {
                        grandchildCommand[i] = inputArgs[i];
                    }

                    // running the command:
                    execvp(grandchildCommand[0], grandchildCommand);
                }

                else // child
                {
                    close(pipeGate[1]);
                    dup2(pipeGate[0], 0);

                    char *childCommand[ARGLENGTH] = {NULL};

                    // gathering the child's command to execute as a new array
                    int gcItor = 0;
                    for (int i = isPipe[0] + 1; i < ARGLENGTH; i++)
                    {
                        childCommand[gcItor++] = inputArgs[i];
                    }

                    // executing the command:
                    execvp(childCommand[0], childCommand);
                }
            }

            else if (pipeIter == 2)
            {
                int pipe1[2], pipe2[2];
                pid_t ls_pid, sort_pid, wc_pid;

                if (pipe(pipe1) == -1)
                {
                    perror("pipe1");
                    return 1;
                }

                if (pipe(pipe2) == -1)
                {
                    perror("pipe2");
                    return 1;
                }

                ls_pid = fork();

                if (ls_pid == -1)
                {
                    perror("fork");
                    return 1;
                }
                else if (ls_pid == 0)
                {
                    // Child process for ls
                    close(pipe1[0]);               // Close read end of pipe1
                    dup2(pipe1[1], STDOUT_FILENO); // Redirect stdout to pipe1

                    char *firstCommand[ARGLENGTH] = {NULL};

                    // gathering the greatgrandchild's command to execute as a new array
                    for (int i = 0; i < isPipe[0]; i++)
                    {
                        firstCommand[i] = inputArgs[i];
                    }

                    // running the command:
                    execvp(firstCommand[0], firstCommand);
                }

                sort_pid = fork();
                if (sort_pid == -1)
                {
                    perror("fork");
                    return 1;
                }
                else if (sort_pid == 0)
                {
                    // Child process for sort
                    close(pipe1[1]);               // Close write end of pipe1
                    close(pipe2[0]);               // Close read end of pipe2
                    dup2(pipe1[0], STDIN_FILENO);  // Redirect stdin to pipe1
                    dup2(pipe2[1], STDOUT_FILENO); // Redirect stdout to pipe2

                    char *secondCommand[ARGLENGTH] = {NULL};

                    // gathering the greatgrandchild's command to execute as a new array
                    int gcItor = 0;
                    for (int i = isPipe[0] + 1; i < isPipe[1]; i++)
                    {
                        secondCommand[gcItor++] = inputArgs[i];
                    }

                    // running the command:
                    execvp(secondCommand[0], secondCommand);
                }

                wc_pid = fork();

                if (wc_pid == -1)
                {
                    perror("fork");
                    return 1;
                }
                else if (wc_pid == 0)
                {
                    // Child process for wc
                    close(pipe1[0]);              // Close read end of pipe1
                    close(pipe1[1]);              // Close write end of pipe1
                    close(pipe2[1]);              // Close write end of pipe2
                    dup2(pipe2[0], STDIN_FILENO); // Redirect stdin to pipe2

                    char *thirdCommand[ARGLENGTH] = {NULL};
                    int cItor = 0;

                    // gathering the child's command to execute as a new array int gcItor = 0;
                    for (int i = isPipe[1] + 1; i < ARGLENGTH; i++)
                    {
                        thirdCommand[cItor++] = inputArgs[i];
                    }

                    // executing the command:
                    execvp(thirdCommand[0], thirdCommand);
                }

                // Parent process
                close(pipe1[0]); // Close read end of pipe1
                close(pipe1[1]); // Close write end of pipe1
                close(pipe2[0]); // Close read end of pipe2
                close(pipe2[1]); // Close write end of pipe2

                // Wait for all child processes to complete
                waitpid(ls_pid, NULL, 0);
                waitpid(sort_pid, NULL, 0);
                waitpid(wc_pid, NULL, 0);

                return 0;
            }

            else
            {
                // running the command:
                execvp(inputArgs[0], inputArgs);
                perror("execvp"); // print error message if execvp fails
                return EXIT_FAILURE;
            }
        }
        // parent's proccess:
        else
        {
            int status;

            // waiting for child to finish, and checking status:
            pid_t wpid = wait(&status);
            if (wpid == -1)
            {
                perror("wait");
            }
        }
    }
}