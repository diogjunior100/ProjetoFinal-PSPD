#include "jogodavida.h"
#include <omp.h> 

#define MASTER 0
#define TAG_CIMA_PARA_BAIXO 0
#define TAG_BAIXO_PARA_CIMA 1

void UmaVida(int *tabulIn, int *tabulOut, int tam_local, int tam_global) {
    int i, j, vizviv;
    #pragma omp parallel for private(j, vizviv) shared(tabulIn, tabulOut, tam_local, tam_global)
    for (i = 1; i <= tam_local; i++) {
        for (j = 1; j <= tam_global; j++) {
            vizviv = tabulIn[ind2d(i-1,j-1,tam_global)]+tabulIn[ind2d(i-1,j,tam_global)]+tabulIn[ind2d(i-1,j+1,tam_global)]+
                     tabulIn[ind2d(i,j-1,tam_global)]+tabulIn[ind2d(i,j+1,tam_global)]+tabulIn[ind2d(i+1,j-1,tam_global)]+
                     tabulIn[ind2d(i+1,j,tam_global)]+tabulIn[ind2d(i+1,j+1,tam_global)];
            if (tabulIn[ind2d(i, j, tam_global)] && vizviv < 2) tabulOut[ind2d(i,j,tam_global)]=0;
            else if (tabulIn[ind2d(i,j,tam_global)] && vizviv > 3) tabulOut[ind2d(i,j,tam_global)]=0;
            else if (!tabulIn[ind2d(i,j,tam_global)] && vizviv == 3) tabulOut[ind2d(i,j,tam_global)]=1;
            else tabulOut[ind2d(i,j,tam_global)] = tabulIn[ind2d(i,j,tam_global)];
        }
    }
}

void InitTabul(int *tabulIn, int tam) {
    for (int ij=0; ij<(tam+2)*(tam+2); ij++) tabulIn[ij] = 0;
    tabulIn[ind2d(1,2,tam)]=1; tabulIn[ind2d(2,3,tam)]=1; tabulIn[ind2d(3,1,tam)]=1;
    tabulIn[ind2d(3,2,tam)]=1; tabulIn[ind2d(3,3,tam)]=1;
}

int Correto(int *tabul, int tam) {
    int cnt=0;
    for (int ij=0; ij<(tam+2)*(tam+2); ij++) cnt+=tabul[ij];
    return(cnt==5&&tabul[ind2d(tam-2,tam-1,tam)]&&tabul[ind2d(tam-1,tam,tam)]&&
           tabul[ind2d(tam,tam-2,tam)]&&tabul[ind2d(tam,tam-1,tam)]&&tabul[ind2d(tam,tam,tam)]);
}

void troca_fronteiras(int* tabul, int tam_local, int tam_global, int rank, int nprocs) {
    int vizinho_cima = (rank==MASTER) ? MPI_PROC_NULL : rank-1;
    int vizinho_baixo = (rank==nprocs-1) ? MPI_PROC_NULL : rank+1;
    MPI_Status status;
    int largura_linha = tam_global + 2;
    MPI_Sendrecv(&tabul[ind2d(1,0,tam_global)],largura_linha,MPI_INT,vizinho_cima,TAG_BAIXO_PARA_CIMA,
                 &tabul[ind2d(0,0,tam_global)],largura_linha,MPI_INT,vizinho_cima,TAG_CIMA_PARA_BAIXO,MPI_COMM_WORLD,&status);
    MPI_Sendrecv(&tabul[ind2d(tam_local,0,tam_global)],largura_linha,MPI_INT,vizinho_baixo,TAG_CIMA_PARA_BAIXO,
                 &tabul[ind2d(tam_local+1,0,tam_global)],largura_linha,MPI_INT,vizinho_baixo,TAG_BAIXO_PARA_CIMA,MPI_COMM_WORLD,&status);
}