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
#include "threads.h"
#include "mplib3.h"


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
| FUNCTION: main
---------------------------------------------------------------------*/

int main (int argc, char** argv) {

  if(argc != 9) {
    fprintf(stderr, "\nNumero invalido de argumentos.\n");
    fprintf(stderr, "Uso: heatSim N tEsq tSup tDir tInf iteracoes tarefas csz\n\n");
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
  int csz = parse_integer_or_exit(argv[8], "csz");


  fprintf(stderr, "\nArgumentos:\n"
  " N=%d tEsq=%.1f tSup=%.1f tDir=%.1f tInf=%.1f iteracoes=%d\n tarefas=%d\n csz=%d\n",
  N, tEsq, tSup, tDir, tInf, iter, trab, csz);

  if(N < 1 || tEsq < 0 || tSup < 0 || tDir < 0 || tInf < 0 || iter < 1 || trab < 1 || (N % trab) != 0 ) {
    fprintf(stderr, "\nErro: Argumentos invalidos.\n"
  " Lembrar que N >= 1, temperaturas >= 0, iteracoes >= 1, tarefas >= 1, trab e multiplo de N\n\n");
    return 1;
  }


/*--------------------------------------------------------------------
| Inicializacao da matriz mestre
---------------------------------------------------------------------*/
  DoubleMatrix2D *matriz;
  matriz = dm2dNew(N+2, N+2);

  if (matriz == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para a matriz.\n\n");
    return -1;
  }
  dm2dSetLineTo (matriz, 0, tSup);
  dm2dSetLineTo (matriz, N+1, tInf);
  dm2dSetColumnTo (matriz, 0, tEsq);
  dm2dSetColumnTo (matriz, N+1, tDir);



/*--------------------------------------------------------------------
| Paralelizacao do programa
---------------------------------------------------------------------*/

  int k = N / trab; //k igual ao tamanho das linhas da fatia interior de cada thread
  int i, res, tam;

  thread_data *array = (thread_data*)malloc(trab * sizeof(thread_data)); //array de argumentos para cada thread
  pthread_t *tid = (pthread_t*)malloc(trab * sizeof(pthread_t));

  if (array == NULL || tid == NULL) {
    fprintf(stderr, "\nErro: Nao foi possivel alocar memoria para os arrays.\n\n");
    return -1;
  }


  if (inicializarMPlib(csz,trab+1) == -1) { //verifica se os canais foram bem inicializados
    fprintf(stderr, "\nErro: Nao foi possivel inicializar a MPlib.\n\n");
    return 1;
  }

  createThreads(tid, array, trab, N, iter); //cria as threads e paraleliza o programa


  for (i = 0; i < trab; i++) {  //tarefa mestre envia mensagens as outras tarefas com os seus valores iniciais
    for (tam = (k * i); tam <= (k * (i+1) + 1); tam++) {
      res = enviarMensagem(0, i + 1, dm2dGetLine(matriz, tam), sizeof(double)*(N + 2));
      checkError(res);
    }
  } 



/*--------------------------------------------------------------------
| Juncao das fatias todas de cada thread na matriz mestre
---------------------------------------------------------------------*/

  for (i = 0; i < trab; i++) {  //tarefa mestre recebe mensagens das outras tarefas com os seus valores finais
    for (tam = (k * i); tam <= (k * (i+1) + 1); tam++) {
      res = receberMensagem(i + 1, 0, dm2dGetLine(matriz, tam), sizeof(double)*(N + 2));
      checkError(res);
    }
  }


  for (i = 0; i < trab; i++) {
    if (pthread_join(tid[i], NULL)) {
      fprintf(stderr, "\nErro ao esperar por um escravo.\n");    
      return -1;
    }  
  }
 

  dm2dPrint(matriz);


  dm2dFree(matriz);
  libertarMPlib();

  free(array);
  free(tid);

  return 0;
}