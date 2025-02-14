#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>

/*reminder to self: sem_wait is P(s)   and sem_post is V(s). wait for resource then acquire lock, and release lock respectively.*/

/*used to know when to finish the program*/
int totalHoneyTally = 0;

#define MAX_BEES 32 /*change this number to test different number of threads*/
#define MAX_HONEY_CAPACITY 50
int num_bees;
int honey_capacity;
int pot = 0; /*how much honey is currently in the pot*/

/*semaphores*/
sem_t mutex; /*we use this to guard the shared resource - the pot*/
sem_t empty_slots; /*empty slots in the pot. used by bees to ensure we dont overfill the pot*/
sem_t full_pot; /*to signal bear when the pot is full.*/

/* Worker thread function to fetch and compare lines */
void *bee_thread(void *args) {
    int id = *((int*)args);
    int myCollectedHoney = 0;
    printf("Bee #%d is born!\n", id);
    while(1){
        printf("Bee #%d goes to fetch some honey!\n", id);
        sleep(3); /*arbitrary, just to let bees fetch honey for some time*/
        sem_wait(&empty_slots); /*wait for an empty slot in the pot*/
        sem_wait(&mutex); /*wait for the critical resource to be available (honey pot tally)*/

        pot++; /*we add some honey*/
        totalHoneyTally++;
        myCollectedHoney++;
        printf("Honey was added to the pot by bee #%d. New current capacity is: %d/%d\n", id, pot, honey_capacity);
        if(pot == honey_capacity){ /*pot is full, so we need to awaken the bear*/
            printf("Pot is now entirely full thanks to bee #%d! He awakens the bear.\n", id);
            sem_post(&full_pot);  /*we call V(full pot semaphore), bouncing the ball to the bear thread.*/
        }
        sem_post(&mutex);
        /*we DONT sem_post the empty slots because this is the responsibility of the bear.*/

        if(totalHoneyTally>300){
            printf("DONE: Bee with id #%d finished work with %d honey collected!\n", id, myCollectedHoney);
            break;
        }
    }
    return NULL;
}

void *bear_thread(void *args) {
    while (1) {
        sem_wait(&full_pot); /*request lock  on full pot (default not available, made available by bees)*/
        sem_wait(&mutex);  /*lock critical section on honey pot contents*/
        printf("Bear is nomming down on all the honey!\n");
        pot = 0;
        printf("New current capacity is: %d/%d\n", pot, honey_capacity);
        sleep(3);  /*sleep to simulate the time it takes for bear to eat the honey. bees cant deposit during this*/
        printf("Bear goes back to sleep\n");

        /*increment the empty slots semaphore to full capacity*/
        for (int i = 0; i < honey_capacity; i++) {
            sem_post(&empty_slots);  // Release empty slots for bees to fill
        }
        sem_post(&mutex);  // Release the lock on the pot

        if(totalHoneyTally>300){
            printf("DONE: Bear is closing its thread too!\n");
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    /*define bees and honey potfrom commandline*/
    num_bees = (argc > 1) ? atoi(argv[1]) : MAX_BEES;
    honey_capacity = (argc > 1) ? atoi(argv[2]) : MAX_HONEY_CAPACITY;

    /*init semaphores*/
    sem_init(&mutex, 0, 1); /*1 here means we init to "1", as in resource is available. binary semaphore that can only be 0 or 1. avail or not avail.*/
    sem_init(&empty_slots, 0, honey_capacity); /*counting semaphore, used to keep track of empty slots in the honey pot.*/
    sem_init(&full_pot, 0, 0);  /*flag to signal that the pot is full. default is 0 so that the bear waits until a honey tells it to GO.*/

    /*create bees*/
    pthread_t bees[num_bees];
    for (int i = 0; i < num_bees; i++) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&bees[i], NULL, bee_thread, id);
    }
    pthread_t bears[1];
    pthread_create(&bears[0], NULL, bear_thread, NULL);

    
    /*wait for them to finish*/
    for (int i = 0; i < num_bees; i++) {
        pthread_join(bees[i], NULL);
    }
    pthread_join(bears[0], NULL);
    return 0;
}
