#ifndef JOGO_DA_VIDA_H
#define JOGO_DA_VIDA_H

#include <mpi/mpi.h>

#define ind2d(i, j, largura_total) (i) * ((largura_total) + 2) + (j)

void UmaVida(int *tabulIn, int *tabulOut, int tam_local, int tam_global);
void InitTabul(int *tabulIn, int tam);
int Correto(int *tabul, int tam);
void troca_fronteiras(int* tabul, int tam_local, int tam_global, int rank, int nprocs);

#endif 