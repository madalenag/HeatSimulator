
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "threads.h"
#include "matrix2d.h"
#include "mplib3.h"

/*--------------------------------------------------------------------
| Function: simul
---------------------------------------------------------------------*/

DoubleMatrix2D *simul(DoubleMatrix2D *m, DoubleMatrix2D *aux, int linhas, int colunas) {

  int i, j;
  double value;

  if(linhas < 2 || colunas < 2)
    return NULL;

  for (i = 1; i < linhas - 1; i++){
    for (j = 1; j < colunas - 1; j++) {
      value = ( dm2dGetEntry(m, i-1, j) + dm2dGetEntry(m, i+1, j) +
    dm2dGetEntry(m, i, j-1) + dm2dGetEntry(m, i, j+1) ) / 4.0;
        dm2dSetEntry(aux, i, j, value);
      }
  }

  return aux;
}


/*--------------------------------------------------------------------
| Function: fnThread
| Description: calcula os novos valores da fatia de matriz que 
| corresponde a cada thread (tamanho dessa fatia dado por k), 
| faz isto iter vezes
---------------------------------------------------------------------*/

void *fnThread(void *a) {

  thread_data *arg;
  arg = (thread_data*)a;

  int id = arg->id; //id de cada thread
  int trab = arg->trab; //num de trabalhadoras
  int N = arg->N; //num de colunas
  int iter = arg->iter; //num de iteracoes

  int k = N / trab; //tamanho da fatia interior de cada thread
  int tam, ret;

  DoubleMatrix2D* matrix = dm2dNew(k + 2, N + 2);
  DoubleMatrix2D* aux = dm2dNew(k + 2, N + 2);

  for (tam = 0; tam <= k + 1; tam++) {
    ret = receberMensagem(0, id, dm2dGetLine(matrix, tam), sizeof(double)*(N + 2));
    // cada thread recebe os valores iniciais da thread mestre
    checkError(ret);
  }

  //para saber os valores das arestas
  double tSup = dm2dGetEntry(matrix,0, 1);
  double tEsq = dm2dGetEntry(matrix, 1, 0);
  double tDir = dm2dGetEntry(matrix, 1, N+1);
  double tInf = dm2dGetEntry(matrix, k+1, 1);

  /*copia so os valores das arestas para aux, 
  para nao ter que copiar a matrix toda (util em matrizes muito grandes) */
  dm2dSetLineTo (aux, 0, tSup);
  dm2dSetLineTo (aux, k+1, tInf);
  dm2dSetColumnTo (aux, 0, tEsq);
  dm2dSetColumnTo (aux, N+1, tDir);


  DoubleMatrix2D* temp;

  for (int i = 0; i < iter; i++) {
    aux = simul(matrix, aux, k + 2, N + 2); 
    //mete em aux os valores calculados com o algoritmo simul da fatia interior
    if (trab != 1) {
      if (id != 1) {
      	if ( id % 2 == 0) {  /*threads pares enviam as mensagens primeiro e as impares recebem primeiro, 
      	para ser possivel a comunicacao entre as varias threads quando csz = 0 */
        	ret = enviarMensagem(id, id - 1, dm2dGetLine(aux, 1), sizeof(double)*(N + 2));
        	//envia mensagem a fatia a cima (se existir)
        	checkError(ret);

        	ret = receberMensagem(id - 1, id, dm2dGetLine(aux, 0), sizeof(double)*(N + 2)); 
        	//recebe mensagem da fatia a cima
        	checkError(ret);
        }
        else {
        	ret = receberMensagem(id - 1, id, dm2dGetLine(aux, 0), sizeof(double)*(N + 2)); 
        	//recebe mensagem da fatia a cima
        	checkError(ret);

        	ret = enviarMensagem(id, id - 1, dm2dGetLine(aux, 1), sizeof(double)*(N + 2));
        	//envia mensagem a fatia a cima (se existir)
        	checkError(ret);
        }
      }

      if (id != trab) {
      	if ( id % 2 == 0) {
        	ret = enviarMensagem(id, id + 1, dm2dGetLine(aux, k), sizeof(double)*(N + 2)); 
        	//envia mensagem a fatia a baixo (se existir)
        	checkError(ret);

        	ret = receberMensagem(id + 1, id, dm2dGetLine(aux, k + 1), sizeof(double)*(N + 2));
        	//recebe mensagem da fatia a baixo
        	checkError(ret);
        }
        else {
        	ret = receberMensagem(id + 1, id, dm2dGetLine(aux, k + 1), sizeof(double)*(N + 2));
        	//recebe mensagem da fatia a baixo
        	checkError(ret);

            ret = enviarMensagem(id, id + 1, dm2dGetLine(aux, k), sizeof(double)*(N + 2)); 
        	//envia mensagem a fatia a baixo (se existir)
        	checkError(ret);
        }
      }
    }

    temp = aux;
    aux = matrix;
    matrix = temp;
  }
  


  for (tam = 0; tam <= k + 1; tam++) {
    ret = enviarMensagem(id, 0, dm2dGetLine(matrix, tam), sizeof(double)*(N + 2)); 
    // cada fatia envia os valores finais a thread mestre
    checkError(ret);
  }

  dm2dFree(aux);
  dm2dFree(matrix);

  return 0;
}


/*--------------------------------------------------------------------
| Function: createThreads
---------------------------------------------------------------------*/
void createThreads(pthread_t *tid, thread_data *array, int trab, int N, int iter) {
  int i;

  for (i = 0; i < trab; ++i) {
    array[i].id = i + 1;
    array[i].N = N;
    array[i].trab = trab;
    array[i].iter = iter;

    if (pthread_create (&tid[i], NULL, fnThread, &array[i]) != 0)
      printf("Erro ao criar tarefa.\n");
  }
}


/*--------------------------------------------------------------------
| Function: checkError
| Description: Verifica se o retorno das funcoes de troca de mensagens
| da erro, se sim sai da funcao.
---------------------------------------------------------------------*/

void checkError (int val) { 
  if ( val == -1) {
    fprintf(stderr, "\nErro: Threads nao conseguiram trocar mensagens \n\n");
    exit(1);
  }
}