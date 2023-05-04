/*MIT License

Copyright (c) 2023 Khoa Nguyen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/


#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <stdint.h>
#include "utility.h"
#include "star.h"
#include "float.h"
#include <pthread.h>

#define NUM_STARS 50000 
#define MAX_LINE 1024
#define DELIMITER " \t\n"

struct Star star_array[ NUM_STARS ];
uint8_t   (*distance_calculated)[NUM_STARS];

pthread_mutex_t lock;
int threadcounter = 1;
double threadmean = 0;
double min,max,mean;

struct thread_args{
  int thread_id;
  int start_index;
  int end_index;
};

void showHelp(){
  printf("Use: findAngular [options]\n");
  printf("Where options are:\n");
  printf("-t          Number of threads to use\n");
  printf("-h          Show this help\n");
}

// to calculate all entries one another to determine the
// average angular separation between any two stars and min max  
// passing value to thread arguments to calculate
void* worker(void* arg){
  struct thread_args* args = (struct thread_args*)arg;
  int i,j;
  for (i = args->start_index; i < args->end_index; i++){
    for (j = i; j < NUM_STARS; j++) {
      if (i != j && distance_calculated[i][j] == 0) {
        double distance = calculateAngularDistance(star_array[i].RightAscension, star_array[i].Declination,
                                                  star_array[j].RightAscension, star_array[j].Declination);
        
        distance_calculated[i][j] = 1;
        distance_calculated[j][i] = 1;
        pthread_mutex_lock(&lock);
        threadcounter++;
        if ( min > distance){
          min = distance;
        }
        if (max < distance){
          max = distance;
        }
        threadmean = threadmean + (distance - threadmean) / (i * NUM_STARS + j + 1); 
        pthread_mutex_unlock(&lock);
      }
    }
  }    
  return NULL;

}
int main( int argc, char * argv[] ){
  FILE *fp;
  uint32_t star_count = 0;
  uint32_t thread_count = 1;
  uint32_t n;
  uint32_t begin = 0;
  uint32_t stop = 0;
  distance_calculated = malloc(NUM_STARS * sizeof(uint8_t[NUM_STARS]));
  if( distance_calculated == NULL ){
    uint64_t num_stars = NUM_STARS;
    uint64_t size = num_stars * num_stars * sizeof(uint8_t);
    printf("Could not allocate %ld bytes\n", size);
    exit( EXIT_FAILURE );
  }
  
  int i,j;
  memset(distance_calculated, 0, sizeof(distance_calculated));
  /*for (i = 0; i < NUM_STARS; i++)
  {
    for (j = 0; j < NUM_STARS; j++)
    {
      distance_calculated[i][j] = 0;
    }
  }*/
  // default every thing to 0 so we calculated the distance.
  // This is really inefficient and should be replace by a memset
  //memset(distance_calculated, 0, NUM_STARS * sizeof(uint8_t[NUM_STARS]));
  int opt;
  // adding -t parameter
  while ((opt = getopt(argc, argv, "t:h")) != -1){
    switch (opt){
      case 't':
        thread_count = atoi(optarg);
        break;
      case 'h':
      default:
        showHelp();
        exit(0); 
    }
  }
  fp = fopen( "data/tycho-trimmed.csv", "r" );
  if( fp == NULL ){
    printf("ERROR: Unable to open the file data/tycho-trimmed.csv\n");
    exit(1);
  }
  char line[MAX_LINE];
  while (fgets(line, 1024, fp)){
    uint32_t column = 0;
    char* tok;
    for (tok = strtok(line, " ");
            tok && *tok;
            tok = strtok(NULL, " ")){
       switch( column ){
          case 0:
              star_array[star_count].ID = atoi(tok);
              break;
       
          case 1:
              star_array[star_count].RightAscension = atof(tok);
              break;
       
          case 2:
              star_array[star_count].Declination = atof(tok);
              break;

          default: 
             printf("ERROR: line %d had more than 3 columns\n", star_count );
             exit(1);
             break;
       }
       column++;
    }
    star_count++;
  }
  
  printf("%d records read\n", star_count );
  fclose(fp);
  //pthread_mutex_init(&lock, NULL);
  
  pthread_t threads[thread_count];
  struct thread_args threading[thread_count];
  clock_t start = clock();
  //seperate work to thread 
  //int stars_per_thread = NUM_STARS / thread_count;
  for (int i = 0; i < thread_count; i++) {
    stop = (star_count/thread_count)*i - 1;
    threading[i].thread_id = i;
    threading[i].start_index = begin;
    threading[i].end_index = stop;
    begin = stop + 1;
    pthread_create(&threads[i], NULL, worker, &threading[i]);
  }
  for (i = 0; i < thread_count; i++) {
    pthread_join(threads[i], NULL);
  }
  printf("Mean Distance: %f degrees\n", threadmean);
  printf("Minimum Distance: %f degrees\n", min);
  printf("Maximum Distance: %f degrees\n", max);
  printf("Time elapsed: %f seconds\n", ((double)clock() - start) / CLOCKS_PER_SEC);

  pthread_mutex_destroy(&lock);

  

  return 0;
}