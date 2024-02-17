#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAXLENGTH 101 // guaranteed maximum string length
#define DEBUG

// the number of occurences of symbol in string
int count_symbol(const char* string, char symbol) {
    int count = 0; // result to return 
    int len = strlen(string);
    for (int i = 0; i < len; ++i) {
        if (string[i] == symbol) ++count;
    }
    return count;
}

// parse line into args
char** get_args(const char* line, int size) {
    char** args = calloc(size, sizeof(char)*strlen(line)); // result to return
    char* buf = (char*)line; // for parsing
    char* saveptr = NULL; // for parsing
    for (int i = 0; i < size; ++i) {
        args[i] = strtok_r(buf, " ", &saveptr);
        buf = NULL;
        if (args[i][strlen(args[i]) - 1] == '\n') args[i][strlen(args[i]) - 1 ] = 0; // correct form of arg
        else args[i][strlen(args[i])] = 0; // correct form of arg
    }
    return args;
}

// cut filename from line and open fd
char* get_flow(char* line, char* arrow, int* file) {
    int mark1, mark2; // begin and end of "> filename"
    char* buf = calloc(MAXLENGTH, sizeof(char));
    int i;
    for (i = 0; i < (int)strlen(line); ++i) { // find filename begin
        if (line[i] == arrow[0] && line[i + 1] == ' ') break;
    }
    mark1 = i - strlen(arrow) + 1; 
    i += 2; // skip blank
    for (int j = 0; line[i] != ' ' && i < (int)strlen(line); ++i, ++j) { // get filename
        buf[j] = line[i];
    }
    mark2 = i + 1;

    if (buf[strlen(buf) - 1] == '\n') buf[strlen(buf) - 1] = 0; // correct form of filename
    else buf[strlen(buf)] = 0; // correct form of filename
    if (arrow[0] == '<') *file = open(buf, O_RDONLY); // input case
    else if (strlen(arrow) == 1) *file = open(buf, O_WRONLY | O_TRUNC | O_CREAT, 0777); // new output case
    else *file = open(buf, O_WRONLY | O_APPEND | O_CREAT, 0777); // append output case
    free(buf);

    // remove input filename from line
    buf = calloc(MAXLENGTH, sizeof(char));
    for (int j = 0; j < mark1; ++j) {
        buf[j] = line[j];
    }
    for (int j = mark1; j < (int)strlen(line); ++j) {
        buf[j] = line[j + mark2 - mark1];
    }

    if (buf[strlen(buf) - 1] == ' ') buf[strlen(buf) - 1] = '\0';

    return buf;
}

// parse line into progs
char** get_progs(const char* line, int size) {
    char* buf = (char*)line;
    char* raw1;
    char* raw2 = calloc(strlen(line), sizeof(char));
    char** progs = calloc(size, sizeof(raw2));
    for (int i = 0; i < size; ++i) {
        progs[i] = calloc(strlen(line), sizeof(char));
    }
    char* saveptr = NULL;
    for (int i = 0; i < size; ++i) {
        raw1 = strtok_r(buf, "|", &saveptr);
        if (raw1[0] == ' ') {
            strcpy(raw2, raw1 + 1);
            raw1 = raw2;
        }
        if (raw1[strlen(raw1) - 1] == ' ') {
            raw1[strlen(raw1) - 1] = '\0';
        }
        strcpy(progs[i], raw1);
        buf = NULL;
        if (progs[i][strlen(progs[i]) - 1] == '\n') progs[i][strlen(progs[i]) - 1 ] = 0; 
        else progs[i][strlen(progs[i])] = 0; 
    }

    free(raw2);
    
    return progs;
}

// run one extern programm
void oneprog(char* line, int file0, int file1, bool isdemon) {
    // get args
    int blanks = count_symbol(line, ' ');
    if (isdemon) --blanks;
    char** args = get_args(line, blanks + 1);

    // exec
    pid_t pid = fork();
    if (pid == -1) {// error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // child
        if (isdemon) {
            if (file0 == -1) file0 = open("/dev/null", O_RDONLY);
            if (file1 == -1) file1 = open("/dev/null", O_WRONLY);
            setpgid(0, 0);
        }
        if (file0 != -1) dup2(file0, 0); // input
        if (file1 != -1) dup2(file1, 1);// output
        if (file1 != -1) dup2(file1, 2); // errors
        execvp(args[0], args);
        close(file0);
        close(file1);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // parrent
        close(file1);
        file1 = -1;
        close(file0);
        file0 = -1;
        free(args);
        if (!isdemon) wait(NULL);
    }
    return;
}

// run pipeline of programms
void pipeline(char* line, bool isdemon) {
    int numprogs = count_symbol(line, '|') + 1; // number of programms
    int file0 = -1; // input
    int file1 = -1; // output
    int fd[2][2]; // pipes
    int blanks;
    pid_t pid;
    char* buf;
    char** progs = get_progs(line, numprogs); // programms to run
    
    char** args;
    
    // firs prog
    if (count_symbol(progs[0], '<') == 1) { // scan <- file
        buf = progs[0];
        progs[0] = get_flow(progs[0], "<", &file0);
        free(buf);
    }    
    blanks = count_symbol(progs[0], ' ');
    args = get_args(progs[0], blanks + 1);
    pipe(fd[0]);
    pid = fork();
    if (pid == -1) { // error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // child
        close(fd[0][0]);
        if (file0 != -1) dup2(file0, 0); // input
        dup2(fd[0][1], 1); // output
        execvp(args[0], args);
        perror("exec");
        exit(EXIT_FAILURE);
    } else { // parent
        close(fd[0][1]);
        free(args);
        //wait(NULL);
    }

    // [2, n-1] progs
    int i;
    for (i = 1; i < numprogs - 1; ++i) {
        blanks = count_symbol(progs[i], ' ');
        args = get_args(progs[i], blanks + 1);
        pipe(fd[i % 2]);
        pid = fork();
        if (pid == -1) { // error
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // child
            close(fd[i % 2][0]); // close input flow
            dup2(fd[(i - 1)% 2][0], 0); // input
            dup2(fd[i % 2][1], 1); // output
            execvp(args[0], args);
            perror("exec");
            exit(EXIT_FAILURE);
        } else { // parent
            close(fd[(i - 1) % 2][0]); // close prev input flow
            close(fd[i % 2][1]); // close output flow
            free(args);
            //wait(NULL);
        }
    }

    // last prog
    if (count_symbol(progs[numprogs - 1], '>') == 1) { // printf -> file new
        buf = progs[numprogs - 1];
        progs[numprogs - 1] = get_flow(progs[numprogs - 1], ">", &file1);
        free(buf);
    }    
    else if (count_symbol(progs[numprogs - 1], '>') == 2) { // printf -> file append
        buf = progs[numprogs - 1];
        progs[numprogs - 1] = get_flow(progs[numprogs - 1], ">>", &file1);
        free(buf);
    }   
    blanks = count_symbol(progs[numprogs - 1], ' ');
    if (isdemon) --blanks;
    args = get_args(progs[numprogs - 1], blanks + 1);
    pid = fork();
    if (pid == -1) { // error
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // child
        dup2(fd[(i - 1) % 2][0], 0);
        if (isdemon && file1 == -1) file1 = open("/dev/nul", O_WRONLY);
        if (file1 != -1) dup2(file1, 1);
        execvp(args[0], args);
        perror("exec");
        exit(EXIT_FAILURE);
    } else { // parent
        close(fd[(i - 1) % 2][0]);
        free(args);
        if (!isdemon) wait(NULL);
    }

    // free memory
    for (int i = 0; i < numprogs; ++i) {
        free(progs[i]);
    }
    free(progs);
    exit(0);
}

int main() {
    char* line = calloc(MAXLENGTH, sizeof(char)); // input line
    char* buf;
    int file0 = -1; // input flow
    int file1 = -1; // output flow

    while (true) {
        printf("> ");
        fgets(line, MAXLENGTH, stdin);

        if (strncmp(line, "exit", 4) == 0) { // finish work
            printf("end program\n");
            break;
        }

        if (count_symbol(line, '|')) { // pipeline
            if (fork() == 0) pipeline(line, count_symbol(line, '&') > 0); // run pipeline
            if (count_symbol(line, '&') == 0) wait(NULL);
        } else { // one programm
            if (count_symbol(line, '<') == 1) { // scan <- file
                buf = line;
                line = get_flow(line, "<", &file0);
                free(buf);
            }
            if (count_symbol(line, '>') == 1) { // printf -> file new
                buf = line;
                line = get_flow(line, ">", &file1);
                free(buf);
            }
            else if (count_symbol(line, '>') == 2) { // printf -> file append
                buf = line;
                line = get_flow(line, ">>", &file1);
                free(buf);
            }
            oneprog(line, file0, file1, (count_symbol(line, '&') > 0)); // run prog
        }
    }

    free(line);

    return 0;
}