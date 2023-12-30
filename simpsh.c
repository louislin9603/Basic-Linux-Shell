#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <dirent.h>

#define ARG_SIZE 128
#define MAX_LINE 256

void get_input();
void execute_command();
int parse_line(char *command, char **arguments);
int search_bin(char *command);


pid_t pid;
char command[MAX_LINE];
char *arguments[MAX_LINE];
char buffer[MAX_LINE];
int status;     //background or foreground
int last_status = 0;

char *input_file = NULL;
char *output_file = NULL;

char *environment[] = 
{
    "PATH=.:/usr/bin",
    "SHELL=simpsh",
    "?=0",
    NULL
};

int main()
{
    int i = 0;
    while (environment[i] != NULL)
    {
        putenv(environment[i]);
        i++;
    }
    while (1)
    {
        get_input();
        
        execute_command();
    }
    return 0;
}

void get_input()
{
    printf("lshell> ");
    fgets(command, MAX_LINE, stdin);
    
    if (feof(stdin))
    {
        exit(0);
    }
    
}

void execute_command()
{
    strcpy(buffer, command);
    
    status = parse_line(buffer, arguments);
    
    if (arguments[0] == NULL)
    {
        return;     //ignores empty lines
    }
    
    if (!strcmp(arguments[0], "cd"))
    {
        if (chdir(arguments[1]) != 0)
        {
            perror("chdir() error");
        }
        return;
    }
    
    pid = fork();
    
    if (pid < 0)
    {
        printf("Forked failed");
        exit(1);
    }
    else if(pid == 0)
    {
        
    //check for input/output redirection
    for (int i = 0; arguments[i] != NULL; i++)
    {
        if (strcmp(arguments[i], "<") == 0)
        {
            input_file = arguments[i+1];
            arguments[i] = NULL;
        }
        else if (strcmp(arguments[i], ">") == 0)
        {
            output_file = arguments[i+1];
            arguments[i] = NULL;
        }
    }
        if (input_file != NULL)
        {
            int fd = open(input_file, O_RDONLY);
            if (fd < 0 )
            {
                fprintf(stderr, "%s: Failed to open. %s\n", input_file, strerror(errno));
                exit(1);
                dup2(fd, 0);
                close(fd);
            }
        }
        
        if(output_file != NULL)
        {
            int fd = creat(output_file, 0666);
            if (fd < 0) 
            {
                fprintf(stderr, "%s: Failed to create. %s\n", output_file, strerror(errno));
                exit(1);
            }
            
            dup2(fd, 1);
            close(fd);
        }
        
        if (access(arguments[0], X_OK) == 0 && execve(arguments[0], arguments, environment) == 0)
        {
            printf("%s: Command not found. \n", arguments[0]);
            exit(1);
        }
        
        /*used to execute stuff like /usr/bin/ls
        if (search_bin(arguments[0]) == 1 && execve(arguments[0], arguments, environment) == 0)
        {
            printf("%s: Command not found. \n", arguments[0]);
            exit(1);
        }
        */
    }
    else //if pid > 0
    { //parent process
    
    wait(NULL);
    /*
        int child_status;
        wait(&child_status);
        if (WIFEXITED(child_status)) { // child exited normally
            status = WEXITSTATUS(child_status); // update the value of '?'
            char char_status;
            setenv("?", itoa(child_status, char_status), 1);
        }
        */
    }
}

int parse_line(char *buffer, char **arguments)
{
    char *delimiter;
    int argc = 0;
    //int status exists
    
    buffer[strlen(buffer)-1] = ' ';
    while (*buffer && (*buffer == ' '))
    {
        buffer++;
    }
    
    while ((delimiter = strchr(buffer, ' ')))
    {
        arguments[argc++] = buffer;
        *delimiter = '\0';
        buffer = delimiter + 1;
        
        while (*buffer && (*buffer == ' '))
        {
            buffer++;
        }
    }
    
    arguments[argc] = NULL;
    
    if (argc == 0)
    {
        return 1;
    }
    
    if ((status = (*arguments[argc-1] == '&')) != 0)
    {
        arguments[argc--] = NULL;
    }
    
    return status;
}

int search_bin(char *command) {
    DIR *dir;
    struct dirent *entry;

    // open /usr/bin directory
    if ((dir = opendir("/usr/bin")) == NULL) {
        printf("Error: Failed to open directory\n");
        return -1;
    }

    // iterate over entries in /usr/bin
    while ((entry = readdir(dir)) != NULL) {
        // check if entry matches command
        if (!strcmp(entry->d_name, command)) {
            char *full_path = malloc(strlen("/usr/bin/") + strlen(command) + 1);
            strcpy(full_path, "/usr/bin/");
            strcat(full_path, command);
            arguments[0] = full_path;
            closedir(dir);
            return 1;
        }
    }

    closedir(dir);
    printf("%s: Command not found\n", command);
    return -1;
}
