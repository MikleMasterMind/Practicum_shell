#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLENGTH 101 // guaranteed maximum string length

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


int main() {

    char* line = calloc(MAXLENGTH, sizeof(char));
    char* saveptr = NULL;
    pid_t pid;

    while (true) {
        putchar('>');

        fgets(line, MAXLENGTH, stdin);

        if (strncmp(line, "exit", 4) == 0) {
            printf("end program\n");
            break;
        }

        int blanks = count_symbol(line, ' ');
        char** args = calloc(blanks+1, sizeof(char)*strlen(line));

        for (int i = 0; (args[i] = strtok_r(line, " ", &saveptr)) != NULL; ++i) {
            free(line); line = NULL;
            if (args[i][strlen(args[i])] == '\n') {
                args[i][strlen(args[i]) - 1] = 0; 
            } else {
                args[i][strlen(args[i])] = 0; 
            }
        }

        pid = fork();
        if (pid == -1) {// error
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            execvp(args[0], args);
            perror("execlp");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
        }

        line = calloc(MAXLENGTH, sizeof(char));

        //putchar('\n');
    }

    //free(line);

    return 0;
}