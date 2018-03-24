/*
// Projeto SO - exercicio 1, version 03
// Sistemas Operativos, DEI/IST/ULisboa 2017-18
// Realizado por Madalena Galrinho (ist187546)
*/


/*--------------------------------------------------------------------
| Includes
---------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#include "matrix2d.h"


/*--------------------------------------------------------------------
| Defines
---------------------------------------------------------------------*/
#define FALSE 0
#define TRUE 1


/*--------------------------------------------------------------------
| Variavel global
---------------------------------------------------------------------*/
double maxD;

/*--------------------------------------------------------------------
| Estruturas
---------------------------------------------------------------------*/

typedef struct {
  int entries;
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} Barrier;

typedef struct {
  int trab, iter, N, id;
  int *next_iter, *curr_iter, *end;
  double *max_dif;
  DoubleMatrix2D *matrix, *aux;
  Barrier *barrier;
} thread_data;


/*--------------------------------------------------------------------
| FUNCTION: parse_integer_or_exit
---------------------------------------------------------------------*/

int parse_integer_or_exit(char const *str, char const *name)
{
  int value;
 
  if(sscanf(str, "%d", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}

/*--------------------------------------------------------------------
| FUNCTION: parse_double_or_exit
---------------------------------------------------------------------*/

double parse_double_or_exit(char const *str, char const *name)
{
  double value;

  if(sscanf(str, "%lf", &value) != 1) {
    fprintf(stderr, "\nErro no argumento \"%s\".\n\n", name);
    exit(1);
  }
  return value;
}


/*--------------------------------------------------------------------
| FUNCTION: simul
---------------------------------------------------------------------*/

double simul(DoubleMatrix2D *m, DoubleMatrix2D *aux, int colunas, int id, int k, double thread_dif) {

  int i, j;
  double value, dif;

  if( (k + 2) < 2 || colunas < 2)
    exit(1);

  for (i = (k * id) + 1 ; i <= (k * (id + 1)); i++){
    for (j = 1; j < colunas - 1; j++) {
        value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) +
    dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        dm2dSetEntry(aux, i, j, value);
        
        dif = value - dm2dGetEntry(m,i,j); 
        thread_dif = dif > thread_dif ? dif : thread_dif ; //atualiza a max diferenca da tarefa
      }
  }

  return thread_dif;
}

/*--------------------------------------------------------------------
| Function: waitBarrier
|
| Description: As n-1 threads que chamam esta funcao ficam a espera,
| quando a n-esima thread chama esta funcao prossegue, pondo a variavel
| next_iter a TRUE (que significa que ja pode prosseguir para a prox
| iteracao). A variavel iter serve cada thread ter o valor da sua 
| iteracao inicial que e igual a curr_iter, quando esta ultima for 
| alterada significa que ja existem threads noutras iteracoes a chamar
| esta funcao, pelo que as tarefas que estavam a espera na iteracao
| anterior conseguem prosseguir. A flag end serve para avisar as threads 
| que a maxima diferenca na matriz e menor que maxD.
---------------------------------------------------------------------*/

int waitBarrier(Barrier *barrier, int *next_iter, int limit, int *curr_iter, double thread_dif,double *max_dif, int *end) {
  if (pthread_mutex_lock(&barrier->mutex))
    return -1;

  int iter = *curr_iter; //cada thread guarda o valor inicial da sua iteracao
  barrier->entries++; //incrementa num de tarefas que chamaram esta funcao

  *max_dif = *max_dif > thread_dif ? *max_dif : thread_dif; //atualiza a max diferenca na matriz

  if (barrier->entries == limit) { //quando a n-esima thread chama a funcao
    *next_iter = TRUE; // threads podem prosseguir para a prox iteracao
    barrier->entries = 0; //mete o contador de tarefas que entram a 0

    if ((*max_dif) < maxD)
      *end = TRUE; //avisa as threads que a simulacao deve acabar
    *max_dif = 0; //faz reset para a prox iteracao

    if (pthread_cond_broadcast(&barrier->cond)) //acorda as tarefas que estao a espera
      return -1;
    
  } else {

    while (*next_iter == FALSE && iter == (*curr_iter)) //enquanto nao puder ir para a prox iteracao, esperam
      if (pthread_cond_wait(&barrier->cond, &barrier->mutex))
        return -1;
  }
  if (pthread_mutex_unlock(&barrier->mutex))
    return -1;

  return 0;
}

/*--------------------------------------------------------------------
| Function: fnThread
| Description: calcula os novos valores da fatia de matriz que 
| corresponde a cada thread (tamanho dessa fatia dado por k), 
| faz isto iter vezes
---------------------------------------------------------------------*/

void *fnThread(void *a) {

  thread_data *arg;
  arg = (thread_data*) a;
  int trab = arg->trab, id = arg->id, N = arg->N, iter = arg->iter;
  DoubleMatrix2D *matrix = arg->matrix, *aux = arg->aux, *temp;
  int k = N / trab;
  
  int *end = arg->end; //flag que indica se a simulacao deve terminar ou nao
  double *max_dif = arg->max_dif; //maxima diferenca na matriz
  double thread_dif; //maxima diferenca da fatia da matriz para cada thread

  Barrier *barrier = arg->barrier;
  int *next_iter = arg->next_iter; //flag que diz se podemos seguir para a prox iteracao
  int *curr_iter = arg->curr_iter; //variavel com o valor da iteracao atual


  for (int i = 0; i < iter; i++) {
    thread_dif = 0; 
    thread_dif = simul (matrix, aux, N + 2, id, k, thread_dif);
  
    *curr_iter = i;
    *next_iter = FALSE;

    if ( waitBarrier(barrier, next_iter, trab, curr_iter, thread_dif, max_dif, end) == -1) {
      printf("Erro ao esperar por tarefas com barreira\n");
      exit(1);
    } 

    temp = aux;
    aux = matrix;
    matrix = temp;
    

    if ((*end) == TRUE)  //quando a flag estiver a 1 termina a simulacao
      break;
  }
  
  return 0;
}

/*--------------------------------------------------------------------
| FUNCTION: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes tarefas maxD\n\n");
    return 1;
  }

  // argv[0] = program name 
  int N = parse_integer_or_exit(argv[1], "N");
  double tEsq = parse_double_or_exit(argv[2], "tEsq");
  double tSup = parse_double_or_exit(argv[3], "tSup");
  double tDir = parse_double_or_exit(argv[4], "tDir");
  double tInf = parse_double_or_exit(argv[5], "tInf");
  int iter = parse_integer_or_exit(argv[6], "iteracoes");
  int trab = parse_integer_or_exit(argv[7], "tarefas");
  maxD = parse_double_or_exit(argv[8], "maxD");


  fprintf(stderr, "\nArgumentos:\n"
  " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n tarefas=%d\n maxD=%.1f\n",
  N, tEsq, tSup, tDir, tInf, iter, trab, maxD);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || maxD < 0 ||(N % trab) != 0 ) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
  " Lembrar que N >= 1, temperaturas e maxD >= 0, iteracoes >= 1, tarefas >= 1, trab e multiplo de N\n\n");
    return 1;
  }


/*--------------------------------------------------------------------
| Inicializacoes
---------------------------------------------------------------------*/
  
  /* Inicializacao das matrizes */
  DoubleMatrix2D *matrix, *aux;
  matrix = dm2dNew(N+2, N+2);
  aux = dm2dNew(N+2, N+2);

  if (matrix == NULL || aux == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as matrizes.\n\n");
    return -1;
  }
  
  dm2dSetLineTo (matrix, 0, tSup);
  dm2dSetLineTo (matrix, N+1, tInf);
  dm2dSetColumnTo (matrix, 0, tEsq);
  dm2dSetColumnTo (matrix, N+1, tDir);

  dm2dSetLineTo (aux, 0, tSup);
  dm2dSetLineTo (aux, N+1, tInf);
  dm2dSetColumnTo (aux, 0, tEsq);
  dm2dSetColumnTo (aux, N+1, tDir);


  /* Inicializacao dos argumentos e do array de threads */
  thread_data *info = (thread_data*)malloc(trab * sizeof(thread_data)); //array de argumentos para cada thread
  pthread_t *tid = (pthread_t*)malloc(trab * sizeof(pthread_t));

  if (info == NULL || tid == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para os arrays.\n\n");
    return -1;
  }


  /* Inicializacao da barreira */
  Barrier *barrier = (Barrier*)malloc(sizeof(Barrier));
    
  if (barrier == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a barreira.\n\n");
    return -1;
  }

  barrier->entries = 0; //inicializa o  contador de tarefas a 0

  if (pthread_mutex_init(&barrier->mutex, NULL) != 0){
    fprintf(stderr, "\nErro: Nao foi possivel inicializar mutex da barreira.\n\n");
    return -1;
  }
  if (pthread_cond_init(&barrier->cond, NULL) != 0) {
    fprintf(stderr, "\nErro: Nao foi possivel inicializar variavel de condicao da barreira.\n\n");
    return -1;
  }
  
  int *next_iter = (int*) malloc (sizeof(int)), i;
  int *curr_iter = (int*) malloc (sizeof(int));
  int *end = (int*) malloc (sizeof (int));
  double *max_dif = (double*) malloc (sizeof(double));

  if (next_iter == NULL || curr_iter == NULL || max_dif == NULL || end == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para as variaveis.\n\n");
    return -1;
  }
  *max_dif = 0;
  *end = FALSE;



/*--------------------------------------------------------------------
| Criacao de threads
---------------------------------------------------------------------*/
  for (i = 0; i < trab; ++i) {
    info[i].N = N, info[i].id = i;
    info[i].trab = trab, info[i].iter = iter;
    info[i].matrix = matrix, info[i].aux = aux;
    info[i].next_iter = next_iter;
    info[i].curr_iter = curr_iter;
    info[i].end = end;
    info[i].barrier = barrier;
    info[i].max_dif = max_dif;

    if (pthread_create (&tid[i], NULL, fnThread, &info[i]) != 0)
      printf("Erro ao criar tarefa.\n");
  }

/*--------------------------------------------------------------------
| Terminacao de threads
---------------------------------------------------------------------*/
  for (i = 0; i < trab; i++) {
    if (pthread_join(tid[i], NULL)) {
      fprintf(stderr, "\nErro ao esperar por um escravo.\n");    
      return -1;
    }  
  }

  if (*curr_iter % 2 == 0)
  	dm2dPrint(aux);
  else
  	dm2dPrint(matrix);


/*--------------------------------------------------------------------
| Libertacao de memoria
---------------------------------------------------------------------*/

  /* Destroi barreira */
  if (pthread_mutex_destroy(&barrier->mutex) != 0) {
  	fprintf(stderr, "\nErro ao destruir mutex.\n");
  	return -1;
  }

  if (pthread_cond_destroy(&barrier->cond) != 0) {
  	fprintf(stderr, "\nErro ao destruir variavel de condicao.\n");
  	return -1;
  }

  free(barrier);
  free(next_iter);
  free(curr_iter);

  dm2dFree(matrix);
  dm2dFree(aux);
  free(max_dif);
  free(end);
  free(info);
  free(tid);
  return 0;
}