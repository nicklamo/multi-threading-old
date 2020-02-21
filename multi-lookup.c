#include "multi-lookup.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
// requestor aka producer, should block when buffer is full
void *requestor(void *arg){
  struct req_thread *thread = (struct req_thread*) arg;
  struct thread_data *data = thread->data;
  int count = 0;
  for (int i = 0; i < thread->num_files_read; i++){
    FILE *ifile;
    if ((ifile = fopen(thread->ifile_names[i], "r"))){
      //read lines
      while(1){
        char *line = (char *)calloc(MAX_NAME_LENGTH, sizeof(char));
        if (fgets(line, MAX_NAME_LENGTH, ifile) != NULL ){
          line[strlen(line)-1] = '\0';
          //lock access to buffer
          pthread_mutex_lock(&data->m);
          while (data->curr_size == BUFFER_SIZE) {  /* block if buffer is full */
              pthread_cond_wait(&data->prod, &data->m);
          }
          //write line to buffer and increment size
          data->shared_buffer[data->write_pos] = line;
          printf("Adding %s to the buffer at position %d \n", data->shared_buffer[data->write_pos], data->write_pos);
          data->write_pos = (data->write_pos+1) % BUFFER_SIZE;
          data->curr_size++;
          pthread_cond_signal(&data->cons);
          pthread_mutex_unlock(&data->m);
        }
        else {
          free(line);
          break;
        }
      }
      fclose(ifile);
      count++;
    }
  }

  pthread_mutex_lock(&data->m);
  data->req_threads_left--;
  pthread_mutex_unlock(&data->m);
  int *retval = malloc(sizeof(*retval));
  *retval = count;
  //return how many files have been read by this thread
  pthread_exit(retval);
  }

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Resolver aka consumer, should block when buffer is empty
void *resolver(void *arg){
  struct thread_data *data = (struct thread_data*) arg;

  while(1) {
    //lock access to buffer
    pthread_mutex_lock(&data->m);
    while (data->curr_size == 0 && data->req_threads_left != 0){  /* block if buffer empty and still reading request*/
				pthread_cond_wait(&data->cons, &data->m);
    }
    if (data->req_threads_left == 0 && data->curr_size == 0){
      pthread_mutex_unlock(&data->m);
      pthread_exit(NULL);
    }
    //take domain from buffer
    char *domain = data->shared_buffer[data->read_pos];
    printf("Taking %s from the buffer at position %d\n", domain, data->read_pos);
    data->read_pos = (data->read_pos+1) % BUFFER_SIZE;
    data->curr_size--;
    pthread_cond_signal(&data->prod);
    pthread_mutex_unlock(&data->m);
    //consume item
    // do a look up of the ip address based on the domain name

    char *IP = (char *)malloc(MAX_IP_LENGTH);
    if (dnslookup(domain, IP, MAX_IP_LENGTH) == 0){
      printf("Result for %s: %s\n", domain, IP);
      pthread_mutex_lock(&data->m_write);
      fprintf(data->res_log, "%s: %s\n", domain, IP );
      pthread_mutex_unlock(&data->m_write);
    }
    else {
      printf("Error looking up DNS for %s\n", domain);
      pthread_mutex_lock(&data->m_write);
      fprintf(data->res_log, "%s: \n", domain);
      pthread_mutex_unlock(&data->m_write);
    }
    free(domain);
    free(IP);

  }
  pthread_exit(NULL);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[]){
  struct timeval start, end;
  gettimeofday(&start, 0);
  //check that there are an appropriate amount of command line args
  if(argc < 6 || argc > 15) {
    printf("USAGE: ./multi-lookup <# requester> <# resolver> <requester log> <resolver log> [ <data file> ...]\n");
    exit(-1);
  }

  // store command line args in variables
  int num_req_threads = atoi(argv[1]);
  int num_res_threads = atoi(argv[2]);
  char *req_log_name = argv[3];
  char *res_log_name = argv[4];

  //check that the files are valid
  FILE *file;
  if ((file = fopen(req_log_name, "w"))){
    fclose(file);
  }
  else{
    printf("Invalid requestor file name\n");
    exit(1);
  }
  if ((file = fopen(res_log_name, "w"))){
    fclose(file);
  }
  else{
    printf("Invalid resover file name\n");
    exit(1);
  }

  //check that there are a proper amount of threads
  if (num_req_threads < 1 || num_req_threads > MAX_REQUESTER_THREADS) {
    printf("Allowed 1-%d requestor threads\n", MAX_REQUESTER_THREADS);
    exit(-1);
  }
  if (num_res_threads < 1 || num_res_threads > MAX_RESOLVER_THREADS) {
    printf("Allowed 1-%d resolver threads\n", MAX_RESOLVER_THREADS);
    exit(-1);
  }

  //initialize thread data to be passed into threads
  struct thread_data data;
  data.write_pos = 0;
  data.read_pos = 0;
  data.curr_size = 0;
  data.req_threads_left = atoi(argv[1]);
  data.req_log = fopen(req_log_name, "w");
  data.res_log = fopen(res_log_name, "w");

  //create array of filepaths
  printf("File Path Names = ");
  int num_files = argc-5;
  char *files[num_files];
  for (int i = 5; i < argc; i++){
    char filepath[24];
    strcpy(filepath, "input/");
    strcat(filepath, argv[i]);
    files[i-5] = strdup(filepath);
    printf("%s ", files[i-5]);
  }
  printf("\n");
  //init condition variables and mutexes
  pthread_mutex_init(&data.m, NULL);
  pthread_mutex_init(&data.m_write, NULL);
  pthread_cond_init(&data.cons, NULL);
  pthread_cond_init(&data.prod, NULL);

  // create arry of files for each request thread to read
  struct req_thread req_threads[num_req_threads];
  for (int i = 0; i < num_req_threads; i++){
      req_threads[i].data = &data;
      req_threads[i].num_files_read = 0;
  }
  for (int j = 0; j < num_files; j++){
    req_threads[j % num_req_threads].ifile_names[ (int)(j / num_req_threads) ] = files[j];
    req_threads[j % num_req_threads].num_files_read++;
  }
  //create req threads
  for (int i = 0; i < num_req_threads; i++){
      req_threads[i].data = &data;
      pthread_create(&req_threads[i].thr, NULL, requestor, &req_threads[i]);
  }

  //create res threads
  pthread_t res_threads[num_res_threads];
  for (int i = 0; i < num_res_threads; i++){
    pthread_create(&res_threads[i], NULL, resolver, &data);
  }

  // join all threads, free memory
  for (int i = 0; i < num_req_threads; i++){
    int *count;
    pthread_join(req_threads[i].thr, (void**)&count);
    fprintf(data.req_log, "Thread %d serviced %d file(s)\n",(int)req_threads[i].thr, *count);
    free(count);
  }
  for (int i = 0; i < num_res_threads; i++){
    pthread_join(res_threads[i], NULL);
  }
  for (int i = 0; i < num_files; i++){
    free(files[i]);
  }
  fclose(data.req_log);
  fclose(data.res_log);

  pthread_cond_destroy(&data.cons);
  pthread_cond_destroy(&data.prod);
  gettimeofday(&end, 0);
  float elapsed = (double)(end.tv_sec-start.tv_sec) + (double)(end.tv_usec-start.tv_usec)/ 1000000;
  printf("Time elapsed = %f\n",  elapsed);
  FILE * perf = fopen("performance.txt", "a");
  fprintf(perf, "\n");
  fprintf(perf, "Number of requestor threads: %d\n", num_req_threads);
  fprintf(perf, "Number of resolver threads: %d\n", num_res_threads);
  fprintf(perf, "Total Run Time: %f\n", elapsed);
  for (int i = 0; i < num_req_threads; i++){
    fprintf(perf, "Thread %d serviced %d file(s)\n",(int)req_threads[i].thr, req_threads[i].num_files_read);
  }
  fclose(perf);
  return 0;
}
