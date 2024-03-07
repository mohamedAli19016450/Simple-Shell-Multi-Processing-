#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/wait.h>
#include<sys/types.h>
#include<signal.h>
#include<sys/resource.h>

#define MAX_INPUT_LENGTH 100
#define APPEND_TO_FILE "a"

char input[MAX_INPUT_LENGTH]; 
char * parsedInput[MAX_INPUT_LENGTH];
int counter = 0, exportFlag = 0, echoFlag = 0, cdFlag = 0, pwdFlag = 0;
int backgroundIndex = 0, exitFlag = 0;

void GetUserInput(void) {
    char cwd[100];
    printf("%s shell >> ",getcwd(cwd, 100));
    scanf("%[^\n]%*c", input);
}

void CleanAndExport(char * token) {
    while(token != NULL) {
        parsedInput[counter] = token;
        token = strtok(NULL, "=");
        counter++;
    }
    parsedInput[counter] = '\0';
    counter = 0;
}

void ParseInput(void) {
    char* token = strtok(input, " ");
    if(strcmp(token, "export") == 0) {
        CleanAndExport(token);
        exportFlag = 1;
    }
    else {
        if(strcmp(token, "cd") == 0) cdFlag = 1;
        if(strcmp(token, "echo") == 0) echoFlag = 1;
        if(strcmp(token, "pwd") == 0) pwdFlag = 1;
        if(strcmp(token, "exit") == 0) exitFlag = 1;
        while (token != NULL) {
            parsedInput[counter] = token;
            token = strtok(NULL, " ");
            counter++;
        }   
        parsedInput[counter] = '\0';
        backgroundIndex = counter - 1;
        counter = 0;
    }
}

void ExecuteCD(void) {
    if((parsedInput[1] == NULL) || ((strcmp(parsedInput[1], "~") == 0))) {
        chdir(getenv("HOME"));
    }
    else {
        int flag = 0;
        flag = chdir(parsedInput[1]);
        if(flag != 0) {
            printf("Error, the directory is not found\n");
        }
    }
}

void ExecuteExport(void) {
    char* data = parsedInput[2];
    if(data[0] == '"') {
        data++;
        data[strlen(data)-1] = '\0';
        setenv(parsedInput[1], data, 1);
    }
    else {
        setenv(parsedInput[1], parsedInput[2], 1);
    }
}

void ExecuteEcho(void) {
    char* echoEnv = parsedInput[1];
    if(parsedInput[2] == NULL) {
        echoEnv++;
        echoEnv[strlen(echoEnv) - 1] = '\0';
        if(echoEnv[0] == '$') {
            echoEnv++;
            printf("%s\n", getenv(echoEnv));
        }
        else {
            printf("%s\n", echoEnv);
        }
    }
    else {
        char* temp = parsedInput[2];
        echoEnv++;
        if(echoEnv[0] == '$') {
            echoEnv++;
            printf("%s ", getenv(echoEnv));
            temp[strlen(temp)-1] = '\0';
            printf("%s\n", temp);
        }
        else {
            printf("%s ", echoEnv);
            temp++;
            temp[strlen(temp)-1] = '\0';
            printf("%s\n", getenv(temp));
        }
    }
}

void ExecuteShellBuiltIn(void) {
    if(cdFlag) {
        ExecuteCD();
    }
    else if(exportFlag) {
        ExecuteExport();
    }
    else if(echoFlag) {
        ExecuteEcho();
    }
    else if(pwdFlag) {
        printf("%s\n", getcwd(NULL, 0));
    }
}

void ExecuteCommand(void) {
    int status, foregroundId;
    int errorCommand = 1;
    int child_id = fork();
    if(child_id == -1) {
        printf("System Error!\n");
        exit(EXIT_FAILURE);
    }
    else if (child_id == 0) {
        if(parsedInput[1] == NULL) {
            errorCommand = execvp(parsedInput[0], parsedInput);
        }
        else if(parsedInput[1] != NULL) {
            char* env = parsedInput[1];
            if(env[0] == '$') {
                int i = 1;
                char* envTemp;
                env++;
                envTemp = getenv(env);
                char * exportTemp = strtok(envTemp, " ");
                while(exportTemp != NULL) {
                    parsedInput[i++] = exportTemp;    
                    exportTemp = strtok(NULL, " ");
                }
            }
            errorCommand = execvp(parsedInput[0], parsedInput);
        }
        if(errorCommand) {
            printf("Error! Unknown command\n");
            exit(EXIT_FAILURE);
        }
    }
    else {
        if(strcmp(parsedInput[backgroundIndex], "&") == 0) {
            return;
        }
        else {
            foregroundId = waitpid(child_id, &status, 0);
            if(foregroundId == -1) {
                perror("Error in waitpad function\n");
                return;
            }
            if(errorCommand) {
                FILE * file = fopen("log.text", APPEND_TO_FILE);
                fprintf(file, "%s", "Child process terminated\n");
                fclose(file);
            }
        }
    }
}

void WriteToLogFile(void) {
    FILE * file = fopen("log.text", APPEND_TO_FILE);
    if(file == NULL) {
        printf("Error in file\n");
        exit(EXIT_FAILURE);
    }
    else {
        fprintf(file, "%s", "Child process terminated\n");
        fclose(file);
    }
}

void ReapChildZombie(void) {
    int status;
    pid_t id = wait(&status);
    if(id == 0 || id == -1) {
        return;
    }
    else {
        WriteToLogFile();
    }
}

void OnChildExist() {
    ReapChildZombie();
}

void Shell(void) {
    while(1) {
        GetUserInput();
        ParseInput();
        if(exitFlag) {
            exitFlag = 0;
            exit(EXIT_FAILURE);
        }
        else if(cdFlag || pwdFlag || exportFlag || echoFlag) {
            ExecuteShellBuiltIn();
            cdFlag = 0;
            pwdFlag = 0;
            exportFlag = 0;
            echoFlag = 0;
        }
        else {
            ExecuteCommand();
        }
    }
}

void SetupEnvironment(void) {
    char arr[100];
    chdir(getcwd(arr , 100));
}

void RegisterChildSignal(void) {
    signal(SIGCHLD, OnChildExist);
}

int main() {
   RegisterChildSignal();
   SetupEnvironment();
   Shell();
   return 0;
}
