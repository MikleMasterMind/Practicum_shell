#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLENGTH 101 // guaranteed maximum string length
#define DEBUG

// the number of occurences of symbol in string
int count_symbol(char* string, char symbol) {
    int count = 0; // result to return 
    int len = strlen(string);
    for (int i = 0; i < len; ++i) {
        if (string[i] == symbol) 
            ++count;
    }
    return count;
}

// parse line into args
char** get_args(char* line, int size) {
    char** args = calloc(size, sizeof(char)*strlen(line));
    char* buf = line;
    char* saveptr;
    for (int i = 0; i < size; ++i) {
        args[i] = strtok_r(buf, " ", &saveptr);
        buf = NULL;
        if (args[i][strlen(args[i]) - 1] == '\n') {
            args[i][strlen(args[i]) - 1 ] = 0; 
        } else { 
            args[i][strlen(args[i])] = 0; 
        }
    }
    return args;
}

int main() {

    char* line = calloc(MAXLENGTH, sizeof(char));
    char* saveptr = NULL;
    char* buf;
    pid_t pid;

    while (true) {
        putchar('>');

        fgets(line, MAXLENGTH, stdin);

        if (strncmp(line, "exit", 4) == 0) { // finish work
            printf("end program\n");
            break;
        }

        int blanks = count_symbol(line, ' ');
        
        char** args = get_args(line,blanks + 1);

        pid = fork();
        if (pid == -1) {// error
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            execvp(args[0], args);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else {
            free(args);
            wait(NULL);
        }

    }

    free(line);

    return 0;
}