#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <sys/types.h>

#define INITIAL_SIZE 10  
#define WORD_LENGTH 100  
#define MIN_NUM_THREADS 4
#define MAX_NUM_THREADS 32

char **dictionaryInArray = NULL; 
int capacity = INITIAL_SIZE;     
int wordCount = 0;             

/*function to read dictionary file into dynamic array*/
void read_file_into_array(FILE *file) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        /*remove newline character from each line*/
        line[strcspn(line, "\n")] = '\0';

        /*check if we need to resize the dynamically sized array of strings, char* is a string rememeber so char ** is array of strings*/
        if (wordCount >= capacity) {
            capacity *= 2;
            char **temp = realloc(dictionaryInArray, capacity * sizeof(char *));
            dictionaryInArray = temp;
        }

        /*save a copy of the word. needs to be freed at end of program because string duplicate allocates on the heap*/
        dictionaryInArray[wordCount] = strdup(line);
        wordCount++;
    }
    free(line);
}

/*helper function to reverse any given string*/
char *reverse_string(const char *str) {
    if(str == NULL){
        return NULL;
    }
    int len = strlen(str);
    char *rev = malloc(len + 1); 
    for (int i = 0; i < len; i++) {
        rev[i] = str[len - i - 1];
    }
    rev[len] = '\0';
    return rev;
}

/*helper function to compare two strings, needed for binary search parameters*/
int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

int main(int argc, char *argv[]) {
    /*begin by defining number of threads from the command line*/
    int nthreads = (argc > 1 & argc < MAX_NUM_THREADS)? atoi(argv[1]) : MIN_NUM_THREADS; // Number of OpenMP threads
    omp_set_num_threads(nthreads); /*tell open mp to use this many threads in its scheduling*/

    /*load in the dictionary*/
    dictionaryInArray = malloc(capacity * sizeof(char *));
    FILE *dictionary_file = fopen("dictionary2.txt", "r"); /*options are words, dictionary2.txt*/
    FILE *output_file = fopen("palindromesOutput.txt", "w"); /*open the output file in write mode. we truncate old results each time the program is run*/
    read_file_into_array(dictionary_file);
    fclose(dictionary_file);

    /*now we perform the parallel part*/
    double start_time, end_time;
    start_time = omp_get_wtime();
    #pragma omp parallel
    {
        int id = omp_get_thread_num();
    #pragma omp for
        for(int i = 0; i < wordCount; i++){
            char *reverseWord = reverse_string(dictionaryInArray[i]);
            char **palindromeOrSemordnilaps = (char **)bsearch(&reverseWord, dictionaryInArray, wordCount, sizeof(char *), compare_strings);
            if(palindromeOrSemordnilaps){ /*this block is entered if the above function did not return NULL (result was found)*/
            #pragma omp critical
            {
                printf("Thread %d found pair: %s -> %s\n", id, dictionaryInArray[i], *palindromeOrSemordnilaps);
                fprintf(output_file, "%s -> %s\n", dictionaryInArray[i], *palindromeOrSemordnilaps);
            }
            }
            free(reverseWord);
        }
    }
    end_time = omp_get_wtime();
    printf("Elapsed time: %d\n", end_time - start_time);

    fclose(output_file);
    printf("All done.\n");

    /*cleanup, free memory of each string on the heap*/
    for (int i = 0; i < wordCount; i++) {
        free(dictionaryInArray[i]);
    }
    free(dictionaryInArray);

    return 0;
}
