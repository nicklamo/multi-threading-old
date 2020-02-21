#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#include <sys/syscall.h>
#include <time.h>
#include <sys/time.h>

#include "util.h"

#ifndef MULTI_LOOKUP_H
#define MULTI_LOOKUP_H

//define global variables
#define MAX_INPUT_FILES 10
#define MAX_RESOLVER_THREADS 10
#define MAX_REQUESTER_THREADS 5
#define MAX_NAME_LENGTH 1025
#define MAX_IP_LENGTH INET6_ADDRSTRLEN

#define BUFFER_SIZE 25

// structure to pass in thread data
struct thread_data {
  char *shared_buffer[BUFFER_SIZE];
  int write_pos;
  int read_pos;
  int curr_size;
  int req_threads_left;
  FILE *req_log;
  FILE *res_log;

  pthread_mutex_t m;
  pthread_mutex_t m_write;
  pthread_cond_t cons;
  pthread_cond_t prod;
};

struct req_thread {
  pthread_t thr;
  char *ifile_names[MAX_INPUT_FILES];
  int num_files_read;
  struct thread_data *data;
};

//define functions and structs
void *request(void *arg);
void *resolve(void *arg);


#endif
