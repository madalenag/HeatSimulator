#ifndef THREADS_H
#define THREADS_H


#include "matrix2d.h"

/*--------------------------------------------------------------------
| Type thread_data
---------------------------------------------------------------------*/

typedef struct {
  int id, trab, iter, N;
} thread_data;



/*--------------------------------------------------------------------
| Functions
---------------------------------------------------------------------*/

void 			createThreads(pthread_t *tid, thread_data *array, int trab, int N, int iter);
void 			checkError(int a);
void* 			fnThread(void *a);
DoubleMatrix2D 	*simul(DoubleMatrix2D *m, DoubleMatrix2D *aux, int linhas, int colunas);


#endif

