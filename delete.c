#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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

    char* line = calloc(100, sizeof(char));
    char* saveptr = NULL;

    fgets(line, 100, stdin);

    line[strlen(line) - 1] = 0;

    int blanks = count_symbol(line, ' ');
        char** args = calloc(blanks+1, sizeof(char)*strlen(line));

        for (int i = 0; (args[i] = strtok_r(line, " ", &saveptr)) != NULL; ++i, line = NULL) {
            args[i][strlen(args[i])] = 0; 
            printf("%s\n", args[i]);
        }

}