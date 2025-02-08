#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>

#define INITIAL_SIZE 10  
#define WORD_LENGTH 100  
#define MIN_NUM_THREADS 4
#define MAX_NUM_THREADS 32

char **dictionaryInArray = NULL; 
int capacity = INITIAL_SIZE;     
int wordCount = 0;

/*read timer function from first exercise, given by KTH*/
double read_timer() {
    static bool initialized = false;
    static struct timeval start;
    struct timeval end;
    if( !initialized )
    {
        gettimeofday( &start, NULL );
        initialized = true;
    }
    gettimeofday( &end, NULL );
    return (end.tv_sec - start.tv_sec) + 1.0e-6 * (end.tv_usec - start.tv_usec);
}

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
    FILE *dictionary_file = fopen(argv[2], "r"); /*options are words, dictionary2.txt and dictionary3.txt*/
    /*dictionary 3: 466k words, dictionary 2: 69k words, words: 25k*/
    FILE *output_file = fopen("palindromesOutput.txt", "w"); /*open the output file in write mode. we truncate old results each time the program is run*/
    read_file_into_array(dictionary_file);
    fclose(dictionary_file);

    /*variables to store results in an array and keep track of our index*/
    char **results = malloc(INITIAL_SIZE * sizeof(char *));
    int results_capacity = INITIAL_SIZE;
    int results_count = 0;

    /*now we perform the parallel part*/
    double start_time, end_time;
    start_time = read_timer();
    #pragma omp parallel
    {
        int id = omp_get_thread_num();
        char resultsBuffer[WORD_LENGTH * 2]; /*temp buffer for formatting the end result string to %s -> %s format*/

        /*local results storage init, to avoid contention over a single results array within a mutex*/
        char **results_local = malloc(INITIAL_SIZE * sizeof(char *));
        int results_local_capacity = INITIAL_SIZE;
        int results_local_count = 0;

        #pragma omp for nowait /*we use nowait because each thread can report its subresult without having to wait for other threads to finish*/
        for(int i = 0; i < wordCount; i++){
            char *reverseWord = reverse_string(dictionaryInArray[i]);
            char **palindromeOrSemordnilaps = (char **)bsearch(&reverseWord, dictionaryInArray, wordCount, sizeof(char *), compare_strings);
            if(palindromeOrSemordnilaps){ /*this block is entered if the above function did not return NULL (result was found)*/
                snprintf(resultsBuffer, sizeof(resultsBuffer), "%s -> %s", dictionaryInArray[i], *palindromeOrSemordnilaps); /*save %s -> %s as a string for pair to a buffer*/
            
            /*dynamically resize our local results array if it needs more space*/
            if (results_local_count >= results_local_capacity) {
                results_local_capacity *= 2;
                results_local = realloc(results_local, results_local_capacity * sizeof(char *));
            }

            results_local[results_local_count] = strdup(resultsBuffer);
            results_local_count++;
            }
            free(reverseWord);
        } /*end of for loop*/

        /*once the for looping is done, we reach a critical section where we merge all the partial results!!*/
        #pragma omp critical
        {
            /*resize the global results array if we need more space*/
            if (results_count + results_local_count >= results_capacity) {
                results_capacity = results_count + results_local_count + INITIAL_SIZE;
                results = realloc(results, results_capacity * sizeof(char *));
            }

            /*copy the local results into the global results array*/
            for (int j = 0; j < results_local_count; j++) {
                results[results_count++] = results_local[j];
            }
            free(results_local);
        }
    }/*end of parallelized section of the program*/


    end_time = read_timer();
    /*saving pairs to local file palindromesOutput.txt*/
    for (int i = 0; i < results_count; i++) {
        fprintf(output_file, "%s\n", results[i]);
        free(results[i]);
    }

    printf("All done.\n");
    printf("Elapsed time: %f seconds \n", (end_time - start_time));
    printf("Number of words in dictionary: %d\n", wordCount);
    printf("Number of palindromes/semordnilapses found: %d\n", results_count);
    fclose(output_file);

    /*cleanup, free memory of each string on the heap*/
    for (int i = 0; i < wordCount; i++) {
        free(dictionaryInArray[i]);
    }
    free(dictionaryInArray);
    free(results);

    return 0;
}
