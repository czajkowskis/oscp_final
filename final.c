#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>
#include <sys/time.h>
#include <errno.h>

#define A_HALL_CAPACITY 5
#define B_HALL_CAPACITY 2
#define VISITING_B_PROBABILITY 0.9 // Probabilty that the visitor will decide to visit hall B - must be between 0 and 1
#define NUMBER_OF_VISITORS 30
#define SECONDS_TO_LEAVE_FROM_B 1 // Time that the vistitor leaving from hall B spends in hall A while leaving the museum in seconds

pthread_cond_t A_hall_cond_var;
pthread_cond_t B_hall_cond_var;
pthread_mutex_t A_hall_mutex;
pthread_mutex_t B_hall_mutex;

int A_free = A_HALL_CAPACITY; // Initialy all slots in Hall A are free
int B_free = B_HALL_CAPACITY; // Initialy all slots in Hall B are free

bool visits_B(){

    double random_value = (double)rand() / RAND_MAX; // Generates random number from 0 to 1
    if(random_value <= VISITING_B_PROBABILITY){
        return true;
    }
    return false;

}

void enter_museum(int id){
    printf("Visitor %d trying to enter Hall A\n", id);
    pthread_mutex_lock(&A_hall_mutex);
    while(A_free <= 1){ // New visitor can enter the museum only if there will still be at least one place for visitors leaving hall B
        pthread_cond_wait(&A_hall_cond_var, &A_hall_mutex);
    }
    A_free -= 1;
    pthread_mutex_unlock(&A_hall_mutex);
    printf("Vistor %d entered Hall A\n", id);
}

void leave_museum_from_hall_A(int id){
    pthread_mutex_lock(&A_hall_mutex);
    A_free++;   // After leaving there is one more place in Hall A
    pthread_cond_signal(&A_hall_cond_var);  //The signal is send to the first process waiting for entering hall 
    pthread_mutex_unlock(&A_hall_mutex);
    printf("Visitor %d left the museum\n", id);
}

void enter_hall_B(int id){ 
    printf("Visitor %d trying to enter Hall B\n", id);
    pthread_mutex_lock(&B_hall_mutex);
    while(B_free <= 0){
            pthread_cond_wait(&B_hall_cond_var, &B_hall_mutex); 
        }
    B_free -= 1;
    pthread_mutex_unlock(&B_hall_mutex);
    pthread_mutex_lock(&A_hall_mutex);
    A_free++;
    pthread_cond_signal(&A_hall_cond_var);
    pthread_mutex_unlock(&A_hall_mutex);
    printf("Visitor %d entered Hall B\n", id);

}

void leave_hall_B(int id){
    printf("Visitor %d trying to leave Hall B\n", id);
    pthread_mutex_lock(&A_hall_mutex);
    while(A_free <= 0){
        pthread_cond_wait(&A_hall_cond_var, &A_hall_mutex);
    }
    A_free -= 1;
    pthread_mutex_unlock(&A_hall_mutex);
    pthread_mutex_lock(&B_hall_mutex);
    B_free++;
    pthread_cond_signal(&B_hall_cond_var);
    pthread_mutex_unlock(&B_hall_mutex);
    printf("Visitor %d entered Hall A from Hall B\n", id);
}

void leave_museum_from_hall_B(int id){
    leave_hall_B(id);
    sleep(SECONDS_TO_LEAVE_FROM_B); //Assume that leaving the museum from hall B takes some amount of time (it is spend in hall A)
    pthread_mutex_lock(&A_hall_mutex);
    A_free++;
    pthread_cond_signal(&A_hall_cond_var);
    pthread_mutex_unlock(&A_hall_mutex);
    printf("Visitor %d left the museum from Hall B\n", id);

}

void tour_the_hall(){
    sleep(rand()%5 + 1); //waits from 1 to 5 seconds
}


void* visitor(void* info){
    int id = *(int*)info;
    while(true){
        enter_museum(id);
        tour_the_hall();
        if (!visits_B()){
            leave_museum_from_hall_A(id);
        } else{
                enter_hall_B(id);
                tour_the_hall();
                leave_museum_from_hall_B(id);
            } 
        sleep(rand()%3)+1; //The process waits 1-3 seconds and returns to the queue
    }
    return NULL;
}

void interrupt_handler(){
    printf("\nA_free: %d,   B_free: %d\n", A_free, B_free);
    exit(0);
}

int main(){
    if (B_HALL_CAPACITY >= A_HALL_CAPACITY){
        printf("The capacity of Hall A has to be greater than capacity of Hall B\n");
        printf("Current capacities:\n");
        printf("Hall A: %d,  Hall B: %d", A_HALL_CAPACITY, B_HALL_CAPACITY);
        return -1;
    }

    signal(SIGINT, interrupt_handler);

    pthread_t thread[NUMBER_OF_VISITORS];
    int id_table[NUMBER_OF_VISITORS];
     
    pthread_mutex_init(&A_hall_mutex, NULL); //Initializing mutex with deafault parameters
    pthread_mutex_init(&B_hall_mutex, NULL);

    pthread_cond_init(&A_hall_cond_var, NULL); //Initilizing conditional variables
    pthread_cond_init(&B_hall_cond_var, NULL);

    for(int i=0; i < NUMBER_OF_VISITORS; i++) {
        id_table[i] = i; //Fille the id table
		pthread_create(&thread[i], NULL, visitor, &id_table[i]); //creating threads
	}

    for(int i=0; i<NUMBER_OF_VISITORS; i++) {
		pthread_join(thread[i], NULL);
	}

    return 0;

}