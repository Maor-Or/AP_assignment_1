#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define ARGLENGTH 20
#define PROMPTLENGTH 30
#define PROMPTCOMMANDLENGTH 3
#define AMOUNTOFSAVEDCOMMANDS 21

// for saving variables:
#define MAX_VAR_NAME 128
#define MAX_VAR_VALUE 128
#define MAX_VARS 100

typedef struct Variable
{
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} Variable;

// for saving variables:
Variable variables[MAX_VARS];
int var_count = 0;

// Signal handler function for SIGINT
void handle_sigint(int signum)
{
    printf("You typed Control-C!\n");
}

// reorgenize the stdin and stdout to a file if we get >
void stdChangeRight(char *fileName, int isAppend) // 0 means > , 1 means >>
{
    int fd;

    // Set the file permissions using octal representation (e.g., 0644)
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // File permissions: rw-r--r--

    if (isAppend)
    {
        fd = open(fileName, O_WRONLY | O_APPEND | O_CREAT, mode);
    }
    else
    {
        fd = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, mode);
    }
    dup2(fd, 1); // making the file to be the output
    close(fd);   // closing the file in the fd array
}
void stdChangeErrOutput(char *fileName)
{
    int fd;

    // Set the file permissions using octal representation (e.g., 0644)
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // File permissions: rw-r--r--

    fd = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, mode);
    dup2(fd, STDERR_FILENO); // making the file to be the output
    close(fd);               // closing the file in the fd array
}

char *intToStr(int num)
{
    // Calculate the maximum length needed for the string representation
    // +2 accounts for possible negative sign and null terminator
    int bufferSize = snprintf(NULL, 0, "%d", num) + 1;
    char *str = (char *)malloc(bufferSize);

    if (str != NULL)
    {
        snprintf(str, bufferSize, "%d", num);
    }

    return str;
}

void set_variable(const char *name, const char *value)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(variables[i].name, name) == 0)
        {
            strncpy(variables[i].value, value, MAX_VAR_VALUE);
            return;
        }
    }

    if (var_count < MAX_VARS)
    {
        strncpy(variables[var_count].name, name, MAX_VAR_NAME);
        strncpy(variables[var_count].value, value, MAX_VAR_VALUE);
        var_count++;
    }
    else
    {
        fprintf(stderr, "Error: Maximum number of variables reached.\n");
    }
}

char *get_variable(const char *name)
{
    for (int i = 0; i < var_count; i++)
    {
        if (strcmp(variables[i].name, name) == 0)
        {
            return variables[i].value;
        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    // for changing the shell's prompt:
    char prompt[PROMPTLENGTH] = "hello:";

    int amper;                                          // for the "&" sign, to know if the command should be running in parallel
    int errFdFlag = -1;                                 // for the "2>" sign, to know if the error output should be redirected
    int prevCommandStatus;                              // for keeping track on the previous command's status
    int isPrevFlag = 0;                                 // for knowing if there was a command already
    char lastInputCommands[AMOUNTOFSAVEDCOMMANDS][128]; // for saving last AMOUNTOFSAVEDCOMMANDS in history
    int lastInputCommandsIndex = 0;
    int isHistoryCommand = 0;
    char *historyCommandToExec;

    // Register the signal handler
    signal(SIGINT, handle_sigint);

    int exitFlag = 1;

    while (exitFlag)
    {
        if (isHistoryCommand == 0 )
        {
            printf("\033[1;34m%s \033[0m", prompt);
        }

        // getting the command to run:
        char input[128] = {'\0'};

        if (isHistoryCommand == 0)
        {
            fgets(input, sizeof(input), stdin);
        }
        else // isHistoryCommand == 1:
        {
            strcpy(input, historyCommandToExec);
        }

        // get the input into the history array:
        strcpy(lastInputCommands[lastInputCommandsIndex], input);
        lastInputCommandsIndex = (lastInputCommandsIndex + 1) % AMOUNTOFSAVEDCOMMANDS;

        // proccessing the data:
        input[strcspn(input, "\n")] = '\0'; // remove trailing newline

        // if "quit" we exit the shell:
        if (strcmp("quit", input) == 0)
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

        /* Is command empty */
        if (inputArgs[0] == NULL)
            continue;

        // checking for "prompt = something"
        if (!strcmp(inputArgs[0], "prompt") && i > 1 && !strcmp(inputArgs[1], "=") && i == PROMPTCOMMANDLENGTH)
        {
            strcpy(prompt, inputArgs[i - 1]);
            continue;
        }

        // checking for "cd dir" command:
        if (!strcmp(inputArgs[0], "cd") && i == 2)
        {
            if (chdir(inputArgs[1]) != 0)
            {
                perror("chdir");
            }
            continue;
        }

        // checking for "!!" command:
        if (!strcmp(inputArgs[0], "!!") && i == 1)
        {
            char *lastCommand = lastInputCommands[(lastInputCommandsIndex - 2) % AMOUNTOFSAVEDCOMMANDS];

            isHistoryCommand = 1;
            historyCommandToExec = lastCommand;
            continue;
        }
        isHistoryCommand = 0;

        if (inputArgs[0][0] == '$' &&!strcmp(inputArgs[1], "=")&&i == 3)
        {
            // Extract variable name
            char *varName = inputArgs[0] + 1; // Skip the '$' character
            
            set_variable(varName, inputArgs[2]);

            continue;
        }

        //checking for "read" command:
        if (!strcmp(inputArgs[0], "read") && i == 2)
        {
            char inValue[MAX_VAR_NAME];
            fgets(inValue, MAX_VAR_NAME, stdin);
            set_variable(inputArgs[1],inValue);
            continue;
        }
        

        /* Does command line end with & */
        if (!strcmp(inputArgs[i - 1], "&"))
        {
            amper = 1;
            inputArgs[i - 1] = NULL;
        }
        else
            amper = 0;

        // making a child to run the command:
        pid_t pid = fork();

        // the child proccess, we run the exec here
        if (pid == 0)
        {
            // scanning the args for any of the following: < ,> ,>> ,|, 2>
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
                else if (strcmp("2>", inputArgs[i]) == 0)
                {
                    errFdFlag = i;
                }
                else if (strcmp("$?", inputArgs[i]) == 0)
                {
                    if (isPrevFlag == 1)
                    {
                        (strcpy(inputArgs[i], intToStr(prevCommandStatus)));
                    }
                }
                else if(inputArgs[i][0] == '$')
                {
                    char * varName = inputArgs[i]+1;
                    (strcpy(inputArgs[i], get_variable(varName)));
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

            if (errFdFlag != -1)
            {
                stdChangeErrOutput(inputArgs[errFdFlag + 1]);
                inputArgs[errFdFlag] = NULL;
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
            if (amper == 0)
            {
                pid_t wpid = wait(&status);
                prevCommandStatus = status;
                isPrevFlag = 1;
                if (wpid == -1)
                {
                    perror("wait");
                }
            }
        }
    }
}