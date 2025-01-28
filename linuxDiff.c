#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>

#define MAX_LINE_LENGTH 1024 /*assumption about how large a line can be maximally. naive.*/
#define MAX_WORKERS 10 /*change this number to test different number of threads*/
#define MAX_LINES 10000/*this is our assumption about how many lines a file may contain. naive assumption and can be improved obviously*/
/*init for the files we will load*/
FILE *file1;
FILE *file2;
/*init for the arrays containing our lines from each file*/
char *fetchedLinesFile1[MAX_LINES];
char *fetchedLinesFile2[MAX_LINES];
/*nextRow determines the next task available in our bag of tasks*/
int nextRow = 0;
/*counter for how many lines are in each file. needed to determine if there are more lines in file 1 or 2, and when we finish.*/
int linesFile1 = 0, linesFile2 = 0;
/*mutex to protect our progress pointer in the bag of tasks*/
pthread_mutex_t mutexNextRow;

/* Helper method to read all lines from a file into a string array */
void *read_file_into_array_1(void *args) {
    FILE *file = (FILE *)args;
    char *line = NULL;
    size_t len = 0; /*these two are apparently needed to use getline instead of fgets which gets specific number of bytes*/
    ssize_t read; /*they are size type, and signed size type respectively. getline returns a negative value if an error occurs.*/
    int lineCount = 0;

    /*read all lines of the file line by line and store them. incremenenting our line count as we go.*/
    while ((read = getline(&line, &len, file)) != -1) {
        if (lineCount < MAX_LINES) {
            fetchedLinesFile1[lineCount] = strdup(line); // Save a copy of the line
            lineCount++;
        }
    }

    /*free up the line buffer*/
    free(line);

    /*update the final coutn of lines in file1.*/
    if (file == file1) {
        linesFile1 = lineCount;
    }
    return NULL;
}

void *read_file_into_array_2(void *args) {
    FILE *file = (FILE *)args;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int lineCount = 0;

    while ((read = getline(&line, &len, file)) != -1) {
        if (lineCount < MAX_LINES) {
            fetchedLinesFile2[lineCount] = strdup(line);
            lineCount++;
        }
    }
    free(line);
    if (file == file2) {
        linesFile2 = lineCount;
    }
    return NULL;
}

/* Worker thread function to fetch and compare lines */
void *worker_thread(void *args) {
    /*allocate two lines with dynamic size since we dont know the size of each line*/
    char *line1 = NULL;
    char *line2 = NULL;

    /*enter loop which breaks once all lines are processed.*/
    while (1) {
        /*mutex lock protects the nextRow incrementer so that no two threads fetch lines from the same index*/
        pthread_mutex_lock(&mutexNextRow);
        if (nextRow >= linesFile1 && nextRow >= linesFile2) {
            pthread_mutex_unlock(&mutexNextRow);
            break; // No more tasks to process
        }

        int currentRow = nextRow++;
        pthread_mutex_unlock(&mutexNextRow);
        
        /*Get lines from current index if they exist, otherwise get a NULL if one file has content in that line but the other doesn't*/
        if (currentRow < linesFile1) {
            line1 = fetchedLinesFile1[currentRow];
        } else {
            line1 = NULL;
        }

        /*Same for the other file.*/
        if (currentRow < linesFile2) {
            line2 = fetchedLinesFile2[currentRow];
        } else {
            line2 = NULL;
        }

        /*Both lines exist so we compare them*/
        if (line1 && line2) {
            line1[strcspn(line1, "\n")] = '\0'; /*uses string compare span to return a string without the second parameter in it, trims newline*/
            line2[strcspn(line2, "\n")] = '\0';
            if (strcmp(line1, line2) != 0) { /*uses string compare and returns 0 if they are the same, 1 if different*/
                printf("< %s\n", line1); /*the strings on the same line index are different so we report it*/
                printf("> %s\n", line2);
            }
        } else if (line1) { /*Only file 1 had content on this index so we report the difference by sending file 1's line*/
            line1[strcspn(line1, "\n")] = '\0';
            printf("< %s\n", line1);
        } else if (line2) { /*Only file 2 had content on this index so we report the difference by sending file 2's line*/
            line2[strcspn(line2, "\n")] = '\0';
            printf("> %s\n", line2);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    /*Begin by opening the files*/
    file1 = fopen(argv[1], "r");
    file2 = fopen(argv[2], "r");
    if (!file1 || !file2) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    /*Init the mutex we use to protect the bag of tasks pointre*/
    pthread_mutex_init(&mutexNextRow, NULL);
    /*Create two worker threads, one to read each file.*/
    pthread_t readThreads[2];
    pthread_create(&readThreads[0], NULL, read_file_into_array_1, file1);
    pthread_create(&readThreads[1], NULL, read_file_into_array_2, file2);
    /*Wait for them to finish reading before moving on*/
    pthread_join(readThreads[0], NULL);
    pthread_join(readThreads[1], NULL);

    /*Create maximum allowed worker threads to pick tasks from the bag of tasks until its empty*/
    pthread_t threads[MAX_WORKERS];
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }
    /*Wait for them to finish*/
    for (int i = 0; i < MAX_WORKERS; i++) {
        pthread_join(threads[i], NULL);
    }

    /*Cleanup, freeing memory and closing files. Not sure if need to destroy the mutex but saw some examples where they did*/
    fclose(file1);
    fclose(file2);
    for (int i = 0; i < linesFile1; i++) {
        free(fetchedLinesFile1[i]);
    }
    for (int i = 0; i < linesFile2; i++) {
        free(fetchedLinesFile2[i]);
    }
    pthread_mutex_destroy(&mutexNextRow);
    return 0;
}
