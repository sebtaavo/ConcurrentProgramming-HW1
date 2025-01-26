#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>

#define MAX_ARRAY_SIZE 10000
#define MAX_WORKERS 10
int currentThreads = 0;
/*threadsCreated and threadsCulled are just here for debugging and seeing that the program works like we think it does*/
/*recursion is pretty difficult to grasp for us so this helps in confirming that everything works as it should.*/
int threadsCreated = 0;
int threadsCulled = 0;
int maxCurrentThreads = 0;
pthread_mutex_t threadCountMutex = PTHREAD_MUTEX_INITIALIZER;

/*Timer function borrowed from the previous exercise. Not written by us. Part of course material.*/
/*we use this to capture program runtime.*/
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

/*Used to check and make sure we never exceed total allowed worker threads. Only used for debugging.*/
void checkIfMaxCurrentThreads(int num){
    if(num > maxCurrentThreads){
        maxCurrentThreads = num;
    }
}

/*We use this struct as a container to let us pass multiple parameters to a thread function. Since each pthread_create*/
/*takes a single argument of type void * and our quicksort function needs multiple inputs, we can bundle them together here.*/
/*The depth is used to determine the current recursion depth in the quicksort.*/
typedef struct {
    int *array;
    int lower;
    int upper;
    int depth;
} ThreadParams;

/*Helper function which lets us switch two values in an array, used in our quicksort*/
void swap(int *a, int *b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

/*Helper function which finds a new pivot position and swaps the elements in current partition*/
/*such that all elements smaller than or equal to the pivot (upper input parameter) are moved to the left of it*/
/*while all elements to the right of the pivot are greater than the pivot. This does not sort it on its own.*/
int partition(int *array, int lower, int upper) {
    int pivot = array[upper];
    int i = lower - 1;
    for (int j = lower; j < upper; j++) {
        if (array[j] <= pivot) {
            i++;
            swap(&array[i], &array[j]);
        }
    }
    swap(&array[i + 1], &array[upper]);
    return i + 1;
}

/*Base case if we are at a recursive depth deeper than amount of allowed threads.*/
void quicksort(int *array, int lower, int upper) {
    if (lower < upper) {
        int pi = partition(array, lower, upper);
        quicksort(array, lower, pi - 1);
        quicksort(array, pi + 1, upper);
    }
}

/*Our quicksort algorithm with threads. For each call, we define our params struct which we need for the next recursive call, */
/*and then perform a partition if lower < upper. If lower >=  upper, it means that the quicksort is complete.*/
/*Once partitioned, we create two new threads*/
void *quicksort_with_threads(void *args) {
    ThreadParams *threadParams = (ThreadParams *)args;

    /*Extract the relevant values from the bundled up params.*/
    int lower = threadParams->lower;
    int upper = threadParams->upper;
    int depth = threadParams->depth;
    int *array = threadParams->array;

    /*If there is still work to do.*/
    if (lower < upper) {
        int pi = partition(array, lower, upper);

        /*Create new right and left thread with the new partition arguments - but only if we dont exceed maximum allowed workers.*/
        pthread_t leftThread, rightThread;
        ThreadParams leftArgs = {array, lower, pi - 1, depth + 1};
        ThreadParams rightArgs = {array, pi + 1, upper, depth + 1};

        int createLeftThread = 0; /*these are our bools to flag if a thread should be created for the next recursion or not.*/
        int createRightThread = 0;
        pthread_mutex_lock(&threadCountMutex);
        if (currentThreads < MAX_WORKERS) {
            createLeftThread = 1;
            currentThreads++;
            threadsCreated++;
            checkIfMaxCurrentThreads(currentThreads);
        }
        if (currentThreads < MAX_WORKERS) {
            createRightThread = 1;
            currentThreads++;
            threadsCreated++;
            checkIfMaxCurrentThreads(currentThreads);
        }
        pthread_mutex_unlock(&threadCountMutex);
        
        /*If we are allowed to create the left thread, proceed with creating it. If we weren't, perform a quicksort with current thread.*/
        if (createLeftThread) {
            pthread_create(&leftThread, NULL, quicksort_with_threads, &leftArgs);
        } else {
            quicksort(array, lower, pi - 1);
        }
        /*If we are allowed to create the right thread, proceed with creating it. If we weren't, perform a quicksort with current thread.*/
        if (createRightThread) {
            pthread_create(&rightThread, NULL, quicksort_with_threads, &rightArgs);
        } else {
            quicksort(array, pi + 1, upper);
        }

        /*If we created the threads above, wait for them to complete and then join them to create space for new ones*/
        /*, again using the mutex lock to make sure we address race conditions.*/
        if (createLeftThread) {
            pthread_join(leftThread, NULL);
            pthread_mutex_lock(&threadCountMutex);
            currentThreads--;
            threadsCulled++;
            pthread_mutex_unlock(&threadCountMutex);
        }
        if (createRightThread) {
            pthread_join(rightThread, NULL);
            pthread_mutex_lock(&threadCountMutex);
            currentThreads--;
            threadsCulled++;
            pthread_mutex_unlock(&threadCountMutex);
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    /*Fetch arguments from run command if there are any*/
    int arraySize = (argc > 1)? atoi(argv[1]) : MAX_ARRAY_SIZE;
    int numThreads = (argc > 2)? atoi(argv[2]) : MAX_WORKERS;
    if (arraySize > MAX_ARRAY_SIZE || arraySize <= 0) {
        fprintf(stderr, "Array size must be between 1 and %d.\n", MAX_ARRAY_SIZE);
        return 1;
    }
    if (numThreads > MAX_WORKERS || numThreads <= 0) {
        fprintf(stderr, "Number of threads must be between 1 and %d.\n", MAX_WORKERS);
        return 1;
    }

    /*Seed the random number generator and populate our array with random values*/
    srand(time(NULL));
    int *array = (int *)malloc(arraySize * sizeof(int));
    for (int i = 0; i < arraySize; i++) {
        array[i] = rand() % arraySize;
    }

    /*Print the unsorted array for debugging*/
    printf("Unsorted array:\n");
    for (int i = 0; i < arraySize; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    /*Create the base thread input at recursion depth 0 and encompassing the whole array from index 0 to length - 1.*/
    pthread_mutex_init(&threadCountMutex, NULL);
    ThreadParams args = {array, 0, arraySize - 1, 0};
    double startTime, endTime;
    startTime = read_timer();
    quicksort_with_threads(&args);
    endTime = read_timer();
    /*Print the sorted array once we complete the step above.*/
    printf("Sorted array:\n");
    for (int i = 0; i < arraySize; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    printf("During the course of the program, we created: %d threads\n", threadsCreated);
    printf("During the course of the program, we culled: %d threads\n", threadsCulled);
    printf("The maximum number of threads running at a time was: %d threads\n", maxCurrentThreads);
    printf("The execution time was %g sec\n", endTime - startTime);
    free(array);
    return 1;
}