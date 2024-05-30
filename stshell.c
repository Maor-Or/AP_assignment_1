#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>


#define ARGLENGTH 20
#define PROMPTLENGTH 30
#define PROMPTCOMMANDLENGTH 3
#define AMOUNTOFSAVEDCOMMANDS 21

// for saving variables:
#define MAX_VAR_NAME 128
#define MAX_VAR_VALUE 128
#define MAX_VARS 100

// for max piping amount:
#define MAX_PIPES 20

typedef struct Variable
{
    char name[MAX_VAR_NAME];
    char value[MAX_VAR_VALUE];
} Variable;

// for saving variables:
Variable variables[MAX_VARS];
int var_count = 0;

// for changing the shell's prompt:
char promptDefault[PROMPTLENGTH] = "\033[1;34m \033[0m";
char prompt[PROMPTLENGTH] = "hello: ";
char fullPromt[PROMPTLENGTH] = {'\0'};

// Signal handler function for SIGINT
void handle_sigint(int signum)
{
    printf("\nYou typed Control-C!\n");
    fflush(stdout); // Ensure the message is displayed immediately
    printf("%s", prompt);
    fflush(stdout); // Ensure the message is displayed immediately
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

void editPromt(char * promptWord)
{

}

int main(int argc, char *argv[])
{

    int amper;                                          // for the "&" sign, to know if the command should be running in parallel
    int errFdFlag = -1;                                 // for the "2>" sign, to know if the error output should be redirected
    int prevCommandStatus;                              // for keeping track on the previous command's status
    int isPrevFlag = 0;                                 // for knowing if there was a command already
    char lastInputCommands[AMOUNTOFSAVEDCOMMANDS][128]; // for saving last AMOUNTOFSAVEDCOMMANDS in history
    int lastInputCommandsIndex = 0;
    int isHistoryCommand = 0;
    char *historyCommandToExec;
    int status;
    int if_flag = 0;
    char input[128] = {'\0'};
    char inputThen[128]= {'\0'};
    char inputElse[128]= {'\0'};
    int isIfTrue = -1;
    int isElseDetected = 0;
    char* input2;

    // Register the signal handler
    signal(SIGINT, handle_sigint);

    int exitFlag = 1;

    while (exitFlag)
    {
        if (if_flag) // in case of if-then-else replace the prompt with the '>' sign"
        {
        //instead of fgets:
        input2 = readline("> ");

        if (!input2) {
            break; // EOF received
        }

        // Add input to history
        add_history(input2);
                strcpy(input, input2);

        }

        else if (isHistoryCommand == 0)
        {
            //instead of fgets:
        input2 = readline(prompt);

        if (!input2) {
            break; // EOF received
        }

        // Add input to history
        add_history(input2);
        strcpy(input, input2);

        }


        else // isHistoryCommand == 1:
        {
            //printf("%s",prompt);
            strcpy(input, historyCommandToExec);
        }

        /* if-then-else-fi part start*/

        if (if_flag == 1)
        { // after if search for then
        isIfTrue = 0;
            if (strncmp(input, "then", 4) == 0)
            {
                strncpy(inputThen, input, sizeof(input) - 1);
                strncpy(inputThen, inputThen+4, sizeof(inputThen) - 5);
                if_flag = 2;
            }
            else
            {
                printf("no 'then' detected\n");
            }
            continue;
        }

        // after then, save the command only if the if-command was 0
        if (if_flag == 2)
        {
            if_flag = 3;
            if (status == 0)
            {
                isIfTrue = 1;
            }

        }
        if (if_flag == 3)
        { // after then-input, search for else
            if (strncmp(input, "else", 4) == 0)
            {
                strncpy(inputElse, input, sizeof(input) - 1);
                strncpy(inputElse, inputElse+4, sizeof(inputElse) - 5);

                isElseDetected = 1;
                if_flag = 5;
                continue;
            }
            else
            {
                isElseDetected = 0;
                if_flag = 5; // after then-input, if there is no else go to search fi
            }
        }

        if (if_flag == 4)
        { // after else, save the command only if the if-command was not success
            if_flag = 5;
            if (status != 0)
            {
                strncpy(inputElse, input, sizeof(input) - 1);
                isIfTrue = 0;
            }
            continue;
        }

        if (if_flag == 5)
        { // search for fi
            if (strncmp(input, "fi", 2) == 0)
            {
                if_flag = 0;
                if (isIfTrue == 1)
                {
                strncpy(input, inputThen, sizeof(input) - 1);
                }
                else
                {
                    if (isElseDetected == 0)
                    {
                        if_flag = 0;
                        continue;
                    }
                    
                strncpy(input, inputElse, sizeof(input) - 1);
                }
                inputThen[0] = '\0';
                inputElse[0] = '\0';
            }
            else
            {
                printf(" no 'fi' detected\n");
                continue;
            }
        }
        /*if-then-else-fi part end*/

        // checking for "if" in the input:
        if (strncmp(input, "if", 2) == 0)
        {
            if_flag = 1;
            // remove if from the input
            strncpy(input, input + 3, sizeof(input) - 1);

        }

        if (if_flag == 0)
        {
            // get the input into the history array:
            strcpy(lastInputCommands[lastInputCommandsIndex], input);
            lastInputCommandsIndex = (lastInputCommandsIndex + 1) % AMOUNTOFSAVEDCOMMANDS;
        }

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

        if (inputArgs[0][0] == '$' && !strcmp(inputArgs[1], "=") && i == 3)
        {
            // Extract variable name
            char *varName = inputArgs[0] + 1; // Skip the '$' character

            set_variable(varName, inputArgs[2]);

            continue;
        }

        // checking for "read" command:
        if (!strcmp(inputArgs[0], "read") && i == 2)
        {
            char inValue[MAX_VAR_NAME];
            fgets(inValue, MAX_VAR_NAME, stdin);
            set_variable(inputArgs[1], inValue);
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
            int isRight = -1, isDoubleRight = -1;

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
                    // if we have |
                    if (strcmp(inputArgs[i], "|") == 0)
                    {
                        int fd[2];
                        if (pipe(fd) < 0)
                        {
                            perror("Error");
                            return 1;
                        }
                        int id2 = fork();
                        if (id2 < 0)
                        {
                            perror("Error");
                            return 1;
                        }
                        if (id2 == 0)
                        {
                            // the first command: <first> | <second>
                            close(fd[0]);
                            int j = 0;
                            char *argv1[MAX_PIPES];
                            while (strcmp(inputArgs[j], "|"))
                            {
                                argv1[j] = inputArgs[j];
                                j++;
                            }
                            argv1[j] = NULL;
                            if (dup2(fd[1], 1) < 0) // stdout of grandchild == fd[1] -> write to pipe
                            {
                                perror("Error");
                                return 1;
                            }
                            close(fd[1]);
                            execvp(argv1[0], argv1);
                        }
                        else
                        {
                            // the second command: <first> | <second>
                            int j = 0;
                            int k = i + 1;
                            while (inputArgs[k] != NULL)
                            {
                                inputArgs[j] = inputArgs[k];
                                j++;
                                k++;
                            }
                            i = -1;
                            inputArgs[j] = NULL;
                            close(fd[1]);
                            if (dup2(fd[0], 0) < 0) // stdin of child == fd[0] -> read of pipe
                            {
                                perror("Error");
                                return 1;
                            }
                            close(fd[0]);
                            wait(NULL);
                        }
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
                else if (inputArgs[i][0] == '$')
                {
                    char *varName = inputArgs[i] + 1;
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

            else
            {
                // Redirect stdout to /dev/null if if_flag is 1
                if (if_flag == 1)
                {
                    int devNull = open("/dev/null", O_WRONLY);
                    if (devNull == -1)
                    {
                        perror("open");
                        exit(EXIT_FAILURE); // Exit the child process with failure
                    }
                    dup2(devNull, STDOUT_FILENO);
                    close(devNull);
                }
                // running the command:
                execvp(inputArgs[0], inputArgs);
                perror("execvp"); // print error message if execvp fails
                return EXIT_FAILURE;
            }
        }
        // parent's proccess:
        else
        {

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