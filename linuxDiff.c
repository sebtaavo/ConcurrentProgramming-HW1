#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_LINE_LENGTH 1024
#define MAX_WORKERS 10
FILE *file1;
FILE *file2;
int nextRow = 0;
int linesFile1, linesFile2;
pthread_mutex_t mutexNextRow; /*mutex to protect our progress pointer*/

/*Looks at nextRow which is a global pointer to the next task in the bag*/
/*, when a thread calls fetchLine to process a new line pair we use the mutex lock*/
/*to ensure that no two workers every process the same line pair.*/
/*The method can be called concurrently since we release the lock as soon as we've*/
/*fetched a row value and incremented the task pointer.*/
int fetchLine(char *line1, char *line2) {
    /*Mutex lock to make sure no two threads process the same line pair*/
    pthread_mutex_lock(&mutexNextRow);
    int currentRow = nextRow++;
    pthread_mutex_unlock(&mutexNextRow);

    /*Stop condition. No more lines to process*/
    if (currentRow >= linesFile1 && currentRow >= linesFile2) {
        return 0;
    }
    /*If current row exists in file 1, read that row using fgets. If we fail, set to an empty string.*/
    if (currentRow < linesFile1) {
        if (fgets(line1, MAX_LINE_LENGTH, file1) == NULL) {
            line1[0] = '\0'; // Empty string if no line
        }
    } else { /*If exceeding the total lines in file1 (evaluated before starting), set to an empty string instead.*/
    /*This section catches the edge cases where one file has more lines to process but the other doesn't.*/
        line1[0] = '\0';
    }
    /*Same as above, if current row exists in file 2 we read that row using fgets. If we fail, set to an empty string.*/
    if (currentRow < linesFile2) {
        if (fgets(line2, MAX_LINE_LENGTH, file2) == NULL) {
            line2[0] = '\0';
        }
    } else {/*If exceeding the total lines in file2 (evaluated before starting), set to an empty string instead.*/
    /*This section catches the edge cases where one file has more lines to process but the other doesn't.*/
        line2[0] = '\0';
    }
    return 1;/*Successfully return*/
}

/*This is the worker function that each thread runs. It initializes two line containers and then repeatedly fetches content*/
/*for them. Printing out differences using */
void *thread_start(void *args) {
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH]; /*Init containers for lines to compare*/
    while (fetchLine(line1, line2)) { /*Fetch lines and use success code to determine if we analyze them or break*/
        line1[strcspn(line1, "\n")] = '\0'; /*Here we use strcspn, meaning string complement span, to find first occurence of a character.*/
        line2[strcspn(line2, "\n")] = '\0'; /*Purpose is to trim newline characters*/
        if (strcmp(line1, line2) != 0) { /*And here we use strcmp, meaning string compare. If line1 and line2 are equal it returns 0, if different 1.*/
            if (line1[0] != '\0') {
                printf("< %s\n", line1);
            }
            if (line2[0] != '\0') {
                printf("> %s\n", line2);
            }
        }
    }
    return NULL;
}

/*Helper method to let us count the number of lines in each file. This is how we measure when we finish in our bag of tasks*/
int count_lines(FILE *file) {
    int lines = 0;
    char buffer[MAX_LINE_LENGTH];
    while (fgets(buffer, MAX_LINE_LENGTH, file)) {
        lines++;
    }
    rewind(file);/*Needed to add this to move file position indicator of the file stream to the beginning of file at some point. Don't know if still necessary but keeping it.*/
    return lines;
}

int main(int argc, char *argv[]) {
    /*Open files from input command line*/
    file1 = fopen(argv[1], "r");
    file2 = fopen(argv[2], "r");

    if (!file1 || !file2) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    /*We count the lines since we need that info to know the ceiling of our bag of tasks. Also needed for when one file is longer than other.*/
    linesFile1 = count_lines(file1);
    linesFile2 = count_lines(file2);

    /*Init our bag of tasks mutex and threads*/
    pthread_t threads[MAX_WORKERS];
    pthread_mutex_init(&mutexNextRow, NULL);
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_create(&threads[i], NULL, thread_start, NULL);
    }

    /*When we ask the threads to join we also ask the program (main thread) to wait with proceeding until they've completed their tasks*/
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /*Cleanup closing the files we opened */
    fclose(file1);
    fclose(file2);
    return 0;
}
