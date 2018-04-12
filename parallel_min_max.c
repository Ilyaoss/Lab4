#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

int active_child_processes = 0;
int pnum;
pid_t *child_arr;

static void sig_alarm(int sigg)
{
    printf("Time's up!\n");
    for(int i = 0; i < pnum; ++i)
    {
        kill(child_arr[i], SIGTERM);
    }
    while (active_child_processes > 0) {
        // your code here
        int wpid = waitpid(-1, NULL, WNOHANG);
        // printf("%d\n", active_child_processes);
        if(wpid == -1)
        {
            if(errno == ECHILD) break;
        }
        else
        {
            active_child_processes -= 1;
        }
    }
    printf("Exiting!\n");
    exit(0);
    //return;
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  pnum = -1;
  bool with_files = false;
  int timeout;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"by_files", no_argument, 0, 'f'},
                                      {"timeout", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "f", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            // your code here
            // error handling
            if (seed <= 0) {
            printf("seed is a positive number\n");
            return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            // your code here
            // error handling
            if (array_size <= 0) {
            printf("array_size is a positive number\n");
            return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            // your code here
            // error handling
            if (pnum <= 0) {
            printf("pnum is a positive number\n");
            return 1;
            }
            break;
          case 3:
            with_files = true;
            break;
          case 4:
            timeout = atoi(optarg);
            if (timeout <= 0) {
            printf("timeout is a positive number\n");
            return 1;
            }
            break;
          defalut:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" \n",
           argv[0]);
    return 1;
  }

  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);

  int p1[2];
  int p2[2];
  pipe(p1);
  pipe(p2);
  struct timeval start_time;
  gettimeofday(&start_time, NULL);
  int size = array_size/pnum;
  
  FILE *fp=NULL;
  fp=fopen("MinMax.txt", "a+");
  if(!fp) {printf("not opned"); return 1;}
  child_arr = malloc(pnum*sizeof(pid_t));
  if(timeout)
  {
      if(signal(SIGALRM, sig_alarm))
      {
          printf("cannot catch SIG_ALARM!\n");
      }
      alarm(timeout);
  }
  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      child_arr[i] = child_pid;
      active_child_processes += 1;
      if (child_pid == 0) {
        struct MinMax min_max;
        // child process
        if(i==pnum-1){ 
        min_max = GetMinMax(array,i*size, array_size);
        }
        else{
        min_max = GetMinMax(array,i*size, (i+1)*size);
        }
        // parallel somehow
        sleep(11);
        if (with_files) {
            fprintf(fp, "%d %d\n",min_max.min,min_max.max);
            
        } else {
          // use pipe here
          write(p1[1],&min_max.min,sizeof(int));
          write(p2[1],&min_max.max,sizeof(int));
        }
        return 0;
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }
  fclose(fp);
  
  int stat;
  while (active_child_processes > 0) {
    // your code here
    wait(&stat);
    if (!WIFEXITED(stat)){
        printf("Exit status: %d\n", WEXITSTATUS(stat));
        return 1;
    }

    active_child_processes -= 1;
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;
  FILE *f = fopen("MinMax.txt","r");
  if(!f){
      printf("Error open MinMax.txt\n");
      return 1;
  }
  else{
  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;
    int minl = INT_MAX;
    int maxl = INT_MIN;
    if (with_files) {
        // read from files
        if(fscanf(f,"%d %d\n",&minl,&maxl)){
        min = minl<min?minl:min;
        max = maxl>max?maxl:max;
        }
    }
     else {
      // read from pipes
      read(p1[0],&minl,sizeof(int));
      read(p2[0],&maxl,sizeof(int));
      min = minl<min?minl:min;
      max = maxl>max?maxl:max;
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }
  }
  fclose(f);
  remove("MinMax.txt");
  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
