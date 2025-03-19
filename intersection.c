/*
 * Operating Systems (2INC0) Practical Assignment
 * Threading
 *
 * Intersection Part [REPLACE WITH PART NUMBER]
 * 
 * STUDENT_NAME_1 (STUDENT_NR_1)
 * STUDENT_NAME_2 (STUDENT_NR_2)
 * STUDENT_NAME_3 (STUDENT_NR_3)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

#include "arrivals.h"
#include "intersection_time.h"
#include "input.h"

// TODO: Global variables: mutexes, data structures, etc...
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t traffic_light_threads[4][4]; 
static pthread_t supply_thread;

typedef struct {
  int side; 
  int direction;
} ManageLightArgs;

/* 
 * curr_car_arrivals[][][]
 *
 * A 3D array that stores the arrivals that have occurred
 * The first two indices determine the entry lane: first index is Side, second index is Direction
 * curr_arrivals[s][d] returns an array of all arrivals for the entry lane on side s for direction d,
 *   ordered in the same order as they arrived
 */
static Car_Arrival curr_car_arrivals[4][4][20];

/*
 * car_sem[][]
 *
 * A 2D array that defines a semaphore for each entry lane,
 *   which are used to signal the corresponding traffic light that a car has arrived
 * The two indices determine the entry lane: first index is Side, second index is Direction
 */
static sem_t car_sem[4][4];

/*
 * supply_cars()
 *
 * A function for supplying car arrivals to the intersection
 * This should be executed by a separate thread
 */
static void* supply_cars()
{
  int t = 0;
  int num_curr_arrivals[4][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};

  // for every arrival in the list
  for (int i = 0; i < sizeof(input_car_arrivals)/sizeof(Car_Arrival); i++)
  {
    // get the next arrival in the list
    Car_Arrival arrival = input_car_arrivals[i];
    // wait until this arrival is supposed to arrive
    sleep(arrival.time - t);
    t = arrival.time;
    // store the new arrival in curr_arrivals
    curr_car_arrivals[arrival.side][arrival.direction][num_curr_arrivals[arrival.side][arrival.direction]] = arrival;
    num_curr_arrivals[arrival.side][arrival.direction] += 1;
    // increment the semaphore for the traffic light that the arrival is for
    sem_post(&car_sem[arrival.side][arrival.direction]);
  }

  return(0);
}


/*
 * manage_light(void* arg)
 *
 * A function that implements the behaviour of a traffic light
 */
static void* manage_light(void* arg)
{
  // TODO:
  // while not all arrivals have been handled, repeatedly:
  //  - wait for an arrival using the semaphore for this traffic light
  //  - lock the right mutex(es)
  //  - make the traffic light turn green
  //  - sleep for CROSS_TIME seconds
  //  - make the traffic light turn red
  //  - unlock the right mutex(es)

  ManageLightArgs* args = (ManageLightArgs*)arg;
  int number_arrivals = 0;

  while (get_time_passed() < END_TIME) {
    if(get_time_passed() > END_TIME) {
      break;
    } else {
      sem_wait(&car_sem[args->side][args->direction]);
    }
   
    
    pthread_mutex_lock(&mutex);
    

    Car_Arrival* arrivals = curr_car_arrivals[args->side][args->direction];
    
    Car_Arrival current = arrivals[number_arrivals];
    int car_id = current.id;    
    printf("traffic light %d %d turns green at time %d for car %d.\n", args->side, args->direction, get_time_passed(), car_id);

    sleep(CROSS_TIME);

    printf("traffic light %d %d turns red at time %d \n", args->side, args->direction, get_time_passed());

    number_arrivals += 1; 

    // release the car
    pthread_mutex_unlock(&mutex);

  }

  free(arg);
  return NULL;
}


int main(int argc, char * argv[])
{
  // create semaphores to wait/signal for arrivals
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      sem_init(&car_sem[i][j], 0, 0);
    }
  }

  // start the timer
  start_time();
  
  // setup struct 
  for (int i = 0; i < 4; i++) {    // i starts at 0
    for (int j = 0; j < 4; j++) { // j starts at 0
      ManageLightArgs* args = malloc(sizeof(ManageLightArgs));
      args->side = i;
      args->direction = j;
      pthread_create(&traffic_light_threads[i][j], NULL, manage_light, args);
    }
  }

  // TODO: create a thread that executes supply_cars()
  pthread_create(&supply_thread, NULL, supply_cars, NULL);

  pthread_join(supply_thread, NULL);

  printf("%d\n", get_time_passed());
  
  // TODO: wait for all threads to finish
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      pthread_join(traffic_light_threads[i][j], NULL);
    }
  }

  printf("%d\n", get_time_passed());


  // destroy semaphores
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      sem_destroy(&car_sem[i][j]);
    }
  }

  return 0; 
}
