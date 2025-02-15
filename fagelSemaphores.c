#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_CHICKS 5
#define MAX_NUM_WORMS 10

int currentNumWorms = MAX_NUM_WORMS;

sem_t availableWorms; /*counting semaphore used to count how many worms are available for the baby birds to eat*/
sem_t mutex; /*binary semaphore used as a mutex lock on the "currentNumWorms" variable in the critical section*/
sem_t empty; /*binary semaphore to signal mama bird to fetch more worms*/

void *bird_thread(void *arg) {
    while (1) {
        sem_wait(&empty); /*wait for baby bird thread to signal with the "empty" semaphore that more worms are needed.*/
        sem_wait(&mutex); /*request critical section lock*/
        printf("Mama bird goes looking for worms\n");
        sleep(1);
        currentNumWorms = MAX_NUM_WORMS;
        printf("Mama bird deposits %d worms.\n", MAX_NUM_WORMS);
        sem_post(&mutex); /*unlock the critical resource*/
        for (int i = 0; i < MAX_NUM_WORMS; i++) {
            sem_post(&availableWorms); /*unlock up to max amount of worms for chicks to consume.*/
        }
    }
}

void *baby_bird_thread(void *arg) {
    int id = *(int *)arg;
    while (1) {
        sem_wait(&availableWorms); /*wait for there to be worms to consume.*/
        sem_wait(&mutex); /*request critical resource lock for the "currentNumWorms" variable.*/
        currentNumWorms--;
        printf("Chick #%d eats a worm. Worms left: %d\n", id, currentNumWorms);
        sleep(1);
        if (!currentNumWorms) {
            sem_post(&empty); /*if we have consumed the last worm, notify mama bird with the "empty" flag semaphore to fetch more.*/
        }
        sem_post(&mutex); /*unlock critical resource*/
    }
}

int main() {
    pthread_t mamaBird;
    pthread_t chicks[NUM_CHICKS];
    int idn[NUM_CHICKS];

    /*init the semaphores, available worms starts at max amount so chicks can start eating right away. mutex starts at 1 because it is available, and empty is 0.*/
    sem_init(&availableWorms, 0, MAX_NUM_WORMS);
    sem_init(&mutex, 0, 1);
    sem_init(&empty, 0, 0);

    /*create the threads, one mama bird and as many chicks as we want.*/
    pthread_create(&mamaBird, NULL, bird_thread, NULL);

    for (int i = 0; i < NUM_CHICKS; i++) {
        idn[i] = i + 1;
        pthread_create(&chicks[i], NULL, baby_bird_thread, &idn[i]);
    }  

    /*cleanup, join threads when we finish (no finishing condition added yet)*/
    pthread_join(mamaBird, NULL);
    for (int i = 0; i < NUM_CHICKS; i++) {
        pthread_join(chicks[i], NULL);
    }

    return 0;
}
