#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <pthread.h>

int N, num_threads, **matrix1, **matrix2, **matrix_row, **matrix_col, **matrix_block;
pthread_mutex_t mutex;

int ** allocate_matrix(int N){
  int * vals = (int *) malloc( N * N * sizeof(int));
  int ** ptrs = (int **) malloc( N * sizeof(int*));
  for (int i = 0; i < N; ++i){
    ptrs[i] = &vals[i*N];
  }
  return ptrs;
}

void init_matrix( int **matrix, int N , int zeros){
  if (zeros != 0){
    for (int i = 0; i < N; ++i){
      for (int j = 0; j < N; ++j){
        matrix[i][j] = 0;
      }
  }
  }
  else{
    for (int i = 0; i < N; ++i){
      for (int j = 0; j < N; ++j){
        matrix[i][j] = rand() % 10;
      }
    }
  }
}

void print_matrix( int **matrix, int N ){
  for (int i = 0; i < N; ++i){
    for (int j = 0; j < N-1; ++j){
      printf("%d,", matrix[i][j]);
    }
    printf("%d", matrix[i][N-1]);
    putchar( '\n' );
  }
}

void * worker_row( void *arg ){  
  int tid = *(int *)(arg); 
  int portion_size = N / num_threads;
  int row_start = tid * portion_size;
  int row_end = (tid+1) * portion_size;
  
  int sum;
  for (int i = row_start; i < row_end; ++i){
    for (int j = 0; j < N; ++j){
      sum = 0;
      for (int k = 0; k < N; ++k){ 
	      sum += matrix1[i][k] * matrix2[k][j];
      }
      matrix_row[i][j] = sum;
    }
  }
}

void * worker_col( void *arg ){  
  int tid = *(int *)(arg); 
  int portion_size = N / num_threads;/// 4 / 2 = 2
  int row_start = tid * portion_size; // 0 * 2 = 0
  int row_end = (tid+1) * portion_size; // 1 * 2 = 2
  
  //int sum;
  for (int j = row_start; j < row_end; ++j){
    for (int i = 0; i < N; ++i){
      ///sum = 0;
      for (int k = 0; k < N; ++k){
          //mutex?
          matrix_col[i][k] += matrix1[i][j] * matrix2[j][k];
          //mutex?
      }
      //matrix_col[ i ][ j ] = sum;
    }
  }
}

void calculate_block(int m1_i_start, int m1_j_start, int m2_i_start, int m2_j_start, int m3_i_start, int m3_j_start, int block_size){
  for(int i = 0; i < block_size; ++i){
    for(int j = 0; j < block_size; ++j){
      for(int k = 0; k < block_size; ++k){
        matrix_block[m3_i_start + i][m3_j_start + j] += matrix1[m1_i_start + i][m1_j_start + k] * matrix2[m2_i_start + k][m2_j_start + j];
      }
    }
  }
}


void * worker_block(void *arg){
  int tid = *(int *)(arg);
  int block_size = N / num_threads;

  int i_block = tid; // j_block in cycle (0, num_threads)
  int i = i_block * block_size;

  //int block_i_start = tid * portion_size; // 1 * 2 = 2 ?// i of block = tid, cycle with 
  //int block_i_end = (tid+1) * portion_size; // 4

  for (int j_block=0; j_block < num_threads; ++j_block){
    int j = j_block * block_size;
    for (int k_block = 0; k_block < num_threads; ++k_block){
      int k=k_block * block_size;
      calculate_block(i, k, k, j, i, j, block_size);
    }
  }
}

int main( int argc, char *argv[] ) {
  int i;
  int sum = 0;
  struct timespec tstart_row, tend_row, tstart_col, tend_col, tstart_block, tend_block;
  double exectime_row, exectime_col, exectime_block;
  pthread_t * threads;

  if (argc != 3) {
    fprintf(stderr, "%s <matrix size>, %s <number of threads>\n", argv[0], argv[1]);
    return -1;
  }
  //printf("%s %s %s", argv[1], argv[2]);
  N = atoi(argv[1]);
  num_threads = atoi(argv[2]);

  if (N % num_threads != 0) {
    fprintf(stderr, "N %d must be a multiple of num of threads %d\n", N, num_threads);
    return -1;
  }

  threads = (pthread_t *) malloc(num_threads * sizeof(pthread_t));

  matrix1 = allocate_matrix(N);
  matrix2 = allocate_matrix(N);
  matrix_row = allocate_matrix(N);
  matrix_col = allocate_matrix(N);
  matrix_block = allocate_matrix(N);
  
  init_matrix(matrix1, N, 0);
  init_matrix(matrix2, N, 0);
  init_matrix(matrix_col, N , 1);
  
  printf("Matrix 1:\n");
  //print_matrix(matrix1, N);
  printf("\n");
  printf("Matrix 2:\n");
  //print_matrix(matrix2, N);
  printf("\n");
  

  //tstart_row = time(0);
  clock_gettime(CLOCK_MONOTONIC_RAW, &tstart_row);
  for (i = 0; i < num_threads; ++i){
    int *tid;
    tid = (int *) malloc(sizeof(int));
    *tid = i;
    pthread_create(&threads[i], NULL, worker_row, (void *)tid);
  }

  for (i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  //tend_row = time(0);
  clock_gettime(CLOCK_MONOTONIC_RAW, &tend_row);

  printf("Matrix row:\n");
  //print_matrix(matrix_row, N);
  printf("\n");

  //tstart_col = time(0);
  clock_gettime(CLOCK_MONOTONIC_RAW, &tstart_col);
  for (i = 0; i < num_threads; ++i) {
    int *tid;
    tid = (int *) malloc(sizeof(int));
    *tid = i;
    pthread_create(&threads[i], NULL, worker_col, (void *)tid);
  }

  for (i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  //tend_col = time(0);
  clock_gettime(CLOCK_MONOTONIC_RAW, &tend_col);

  printf("Matrix col:\n");
  //print_matrix(matrix_col, N);
  printf("\n");

  // tstart_block = time(0);
  clock_gettime(CLOCK_MONOTONIC_RAW, &tstart_block);
  for (i = 0; i < num_threads; ++i) {
    int *tid;
    tid = (int *) malloc(sizeof(int));
    *tid = i;
    pthread_create(&threads[i], NULL, worker_block, (void *)tid);
  }

  for (i = 0; i < num_threads; ++i) {
    pthread_join(threads[i], NULL);
  }

  // tend_block = time(0);
  clock_gettime(CLOCK_MONOTONIC_RAW, &tend_block);

  printf("Matrix block:\n");
  //print_matrix(matrix_block, N);
  printf("\n");
  
  exectime_row = (tend_row.tv_sec - tstart_row.tv_sec) * 1000000 + (tend_row.tv_nsec - tstart_row.tv_nsec) / 1000;;
  exectime_col = (tend_col.tv_sec - tstart_col.tv_sec) * 1000000 + (tend_col.tv_nsec - tstart_col.tv_nsec) / 1000;
  exectime_block = (tend_block.tv_sec - tstart_block.tv_sec) * 1000000 + (tend_block.tv_nsec - tstart_block.tv_nsec) / 1000;

  printf("Number of MPI ranks: 0\tNumber of threads: %d\nExecution time row:%.6lf nanosec\tExecution time column:%.6lf nanosec\tExecution Time block:%.6lf nanosec\n", num_threads, exectime_row, exectime_col, exectime_block);

  FILE *F1;
  F1 = fopen("log.csv", "a");
  //fprintf(F1, "size, threads, time_col, time_row, time_block\n"); 
  
  fprintf(F1, "%d,%d,%.6lf,%.6lf, %.6lf\n", N, num_threads, exectime_row, exectime_col, exectime_block);

  fclose(F1);

  return 0;
}