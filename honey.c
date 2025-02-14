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
#define END_PROGRAM_AFTER_THIS_MUCH_HONEY 100
int num_bees;
int honey_capacity;
int pot = 0; /*how much honey is currently in the pot*/

/*semaphores*/
sem_t mutex; /*we use this to guard the shared resource - the pot*/
sem_t empty_slots; /*empty slots in the pot. used by bees to ensure we dont overfill the pot*/
sem_t full_pot; /*to signal bear when the pot is full.*/

/*semaphore mutex for total honey collected tally, just needed for statistics not for program to run*/
sem_t tally_mutex;

/* Worker thread function to fetch and compare lines */
void *bee_thread(void *args) {
    int id = *((int*)args);
    int myCollectedHoney = 0;
    printf("Bee #%d is born!\n", id);
    while(1){
        printf("Bee #%d goes to fetch some honey!\n", id);
        sleep(3); /*arbitrary, just to let bees fetch honey for some time*/
        sem_wait(&empty_slots); /*wait for an empty slot in the pot*/
        sem_wait(&mutex); /*wait for the critical resource to be available (honey pot)*/
    
        /*protect the total tally of honey*/
        sem_wait(&tally_mutex);
        totalHoneyTally++;
        sem_post(&tally_mutex);
        myCollectedHoney++;
        /*this pot is protected by the "mutex" semaphore*/
        pot++; /*we add some honey*/

        printf("Honey was added to the pot by bee #%d. New current capacity is: %d/%d\n", id, pot, honey_capacity);
        if(pot == honey_capacity){ /*pot is full, so we need to awaken the bear*/
            printf("Pot is now entirely full thanks to bee #%d! He awakens the bear.\n", id);
            sem_post(&full_pot);  /*we call V(full pot semaphore), bouncing the ball to the bear thread.*/
        }
        sem_post(&mutex);
        /*we DONT sem_post the "empty slots" semaphore because this is the responsibility of the bear.*/
        
        /*check if we've got all the honey needed for program to finish*/
        sem_wait(&tally_mutex);
        if(totalHoneyTally>END_PROGRAM_AFTER_THIS_MUCH_HONEY){
            printf("DONE: Bee with id #%d finished work with %d honey collected!\n", id, myCollectedHoney);
            sem_post(&full_pot);  /*we call V(full pot semaphore), bouncing the ball to the bear thread so that it can finish too and not deadlock*/
            sem_post(&tally_mutex); 
            break;
        }
        sem_post(&tally_mutex);
    }
    free(args);
    return NULL;
}

void *bear_thread(void *args) {
    while (1) {
        sem_wait(&tally_mutex);
        if(totalHoneyTally>END_PROGRAM_AFTER_THIS_MUCH_HONEY){
            printf("DONE: Bear is closing its thread too!\n");
            sem_post(&tally_mutex);
            break;
        }
        sem_post(&tally_mutex);

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
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    /*define bees and honey potfrom commandline*/
    num_bees = (argc > 1) ? atoi(argv[1]) : MAX_BEES;
    honey_capacity = (argc > 1) ? atoi(argv[2]) : MAX_HONEY_CAPACITY;

    /*init the random seed for our random times to sleep for the bear and bees*/
    srand(time(NULL));

    /*init semaphores*/
    sem_init(&mutex, 0, 1); /*1 here means we init to "1", as in resource is available. binary semaphore that can only be 0 or 1. avail or not avail.*/
    sem_init(&empty_slots, 0, honey_capacity); /*counting semaphore, used to keep track of empty slots in the honey pot.*/
    sem_init(&full_pot, 0, 0);  /*flag to signal that the pot is full. default is 0 so that the bear waits until a honey tells it to GO.*/
    sem_init(&tally_mutex, 0, 1); /*mutex lock for when we tally up total amount of honey gathered.*/

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

    /*cleanup*/
    sem_destroy(&mutex);
    sem_destroy(&empty_slots);
    sem_destroy(&full_pot);
    sem_destroy(&tally_mutex);

    printf("In total, %d honey was collected by %d bees this run.", totalHoneyTally, num_bees);
    return 0;
}
