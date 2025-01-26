/* matrix summation using pthreads

   features: uses a barrier; the Worker[0] computes
             the total sum from partial sums computed by Workers
             and prints the total sum to the standard output

   usage under Linux:
     gcc matrixSum.c -lpthread
     a.out size numWorkers

*/
#ifndef _REENTRANT 
#define _REENTRANT 
#endif 
#include <pthread.h>
/*Added limits include.*/
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#define MAXSIZE 10000  /* maximum matrix size */
#define MAXWORKERS 10   /* maximum number of workers */

pthread_mutex_t barrier;  /* mutex lock for the barrier */
pthread_cond_t go;        /* condition variable for leaving */
int numWorkers;           /* number of workers */ 
int numArrived = 0;       /* number who have arrived */

/* a reusable counter barrier */
void Barrier() {
  pthread_mutex_lock(&barrier);
  numArrived++;
  if (numArrived == numWorkers) {
    numArrived = 0;
    pthread_cond_broadcast(&go);
  } else
    pthread_cond_wait(&go, &barrier);
  pthread_mutex_unlock(&barrier);
}

/* timer */
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

double start_time, end_time; /* start and end times */
int size, stripSize;  /* assume size is multiple of numWorkers */
int sums[MAXWORKERS]; /* partial sums */

/*New code for question a), for finding min and max and their positions. We lock the critical section. Written by Sebastian Taavo Ek*/
pthread_mutex_t barrierMin;  /* mutex lock for the barrier */
pthread_mutex_t barrierMax;  /* mutex lock for the barrier */
pthread_mutex_t barrierSum;  /* mutex lock for the barrier */
int min = INT_MAX;
int minRow;
int minCol;
int max = INT_MIN;
int maxRow;
int maxCol;
void checkIfNewMin(int i, int row, int col){
  if(i < min){
    pthread_mutex_lock(&barrierMin);
    if(i < min){
      min = i;
      minRow = row;
      minCol = col;
    }
    pthread_mutex_unlock(&barrierMin);
  }
}

void checkIfNewMax(int i, int row, int col){
  if(i > max){
    pthread_mutex_lock(&barrierMax);
    if(i > max){
      max = i;
      maxRow = row;
      maxCol = col;
    }
    pthread_mutex_unlock(&barrierMax);
  }
}
/*new code for b), written in order to update sum while adressing race conditions and not using partial sums in an array.*/
int sumOfSums = 0;
void updateSum(int term){
  pthread_mutex_lock(&barrierSum);
  sumOfSums += term;
  pthread_mutex_unlock(&barrierSum);
}
/*New code for question c).*/
/*For bag of tasks*/
int nextRow = 0; /*Inspired by slide 29 in lecture 5 on Barriers, flags.*/
int rowsDone = 0;
pthread_mutex_t barrierNextRow;  /* mutex lock for the barrier */
pthread_mutex_t barrierRowsDone;  /* mutex lock for the barrier */
/*End of new code*/
int matrix[MAXSIZE][MAXSIZE]; /* matrix */

void *Worker(void *);

/* read command line, initialize, and create threads */
int main(int argc, char *argv[]) {
  srand(time(NULL)); /*Randomly seed the our PRNG. This line was added for a). */
  int i, j;
  long l; /* use long in case of a 64-bit system */
  pthread_attr_t attr;
  pthread_t workerid[MAXWORKERS];

  /* set global thread attributes */
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  /* initialize mutex and condition variable */
  pthread_mutex_init(&barrier, NULL);
  pthread_cond_init(&go, NULL);

  /*New code for a). Written by Sebastian Taavo Ek*/
  pthread_mutex_init(&barrierMin, NULL);
  pthread_mutex_init(&barrierMax, NULL);
  /*New code for b). Written by Sebastian Taavo Ek*/
  pthread_mutex_init(&barrierSum, NULL);
  /*New code for c). Written by Sebastian Taavo Ek*/
  pthread_mutex_init(&barrierNextRow, NULL);
  pthread_mutex_init(&barrierRowsDone, NULL);

  /* read command line args if any */
  size = (argc > 1)? atoi(argv[1]) : MAXSIZE;
  numWorkers = (argc > 2)? atoi(argv[2]) : MAXWORKERS;
  if (size > MAXSIZE) size = MAXSIZE;
  if (numWorkers > MAXWORKERS) numWorkers = MAXWORKERS;
  stripSize = size/numWorkers;

  /* initialize the matrix */
  for (i = 0; i < size; i++) {
	  for (j = 0; j < size; j++) {
          matrix[i][j] = rand()%99;
	  }
  }

  /* print the matrix */
#ifdef DEBUG
  for (i = 0; i < size; i++) {
	  printf("[ ");
	  for (j = 0; j < size; j++) {
	    printf(" %d", matrix[i][j]);
	  }
	  printf(" ]\n");
  }
#endif

  /* do the parallel work: create the workers */
  start_time = read_timer();
  for (l = 0; l < numWorkers; l++)
    pthread_create(&workerid[l], &attr, Worker, (void *) l);
  pthread_exit(NULL);
}

/* Each worker sums the values in one strip of the matrix.
   After a barrier, worker(0) computes and prints the total */
void *Worker(void *arg) { /*-----------------------------------------------------------------------------------Worker begins here for refernce ----------------------------*/
  long myid = (long) arg;
  int i, j;
  int row;
  int total = 0;
#ifdef DEBUG
  printf("worker %d (pthread id %d) has started\n", myid, pthread_self());
#endif
  
  /*New for c), bag of tasks implementation*/
  while(rowsDone < size) {
    pthread_mutex_lock(&barrierNextRow);
    if (nextRow >= size) {
        pthread_mutex_unlock(&barrierNextRow);
        break;
    }
    row = nextRow;
    //printf("Worker %d picked up row %d of the matrix.\n", myid, row);  //enable this to track how the threads pick up work from the bag
    nextRow++;
    pthread_mutex_unlock(&barrierNextRow);
    for (j = 0; j < size; j++) {
        total += matrix[row][j];
        checkIfNewMin(matrix[row][j], row, j);
        checkIfNewMax(matrix[row][j], row, j);
    }
    pthread_mutex_lock(&barrierRowsDone);
    rowsDone++;
    pthread_mutex_unlock(&barrierRowsDone);
  }

  updateSum(total); /*this line replaces the array with partial results.*/
  Barrier();
  if (myid == 0) {/*This is where the main thread (thread 0) finalizes computing I think*/
    /* get end time */
    end_time = read_timer();
    /* print results */
    printf("The total is %d\n", sumOfSums);
    /*Added code for b). Printing final results of min and max*/
    printf("We processed %d rows.", rowsDone);
    printf("The min value is %d and it was first found at position (%d, %d)\n", min, minCol, minRow);
    printf("The max value is %d and it was first found at position (%d, %d)\n", max, maxCol, maxRow);
    printf("The execution time is %g sec\n", end_time - start_time);
  }
}
