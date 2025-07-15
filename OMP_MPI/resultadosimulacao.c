#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <mpi/mpi.h>
#include "resultadosimulacao.h"
#include "jogodavida.h"

#define MASTER 0

double wall_time(void) {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    return (tv.tv_sec + tv.tv_usec / 1000000.0);
}

ResultadoSimulacao executar_simulacao(int tam, int rank, int nprocs) {
    int i;
    int *tabulIn_global = NULL, *tabulIn_local, *tabulOut_local;
    ResultadoSimulacao res = {0}; // Inicializa o struct de resultado

    double t_total_inicio;
    if (rank == MASTER) t_total_inicio = wall_time();
    
    int *elementos_por_processo = NULL, *deslocamentos = NULL;
    int linhas_base = tam / nprocs;
    int linhas_extras = tam % nprocs;
    int tam_local = linhas_base + (rank < linhas_extras ? 1 : 0);

    if (rank == MASTER) {
        double time_init_start = wall_time();
        elementos_por_processo = (int *) malloc(nprocs * sizeof(int));
        deslocamentos = (int *) malloc(nprocs * sizeof(int));
        int deslocamento_atual = 0;
        int largura_linha = tam + 2;
        for (i = 0; i < nprocs; i++) {
            int linhas_por_worker = linhas_base + (i < linhas_extras ? 1 : 0);
            elementos_por_processo[i] = linhas_por_worker * largura_linha;
            deslocamentos[i] = deslocamento_atual;
            deslocamento_atual += elementos_por_processo[i];
        }
        tabulIn_global = (int *) malloc((tam + 2) * (tam + 2) * sizeof(int));
        InitTabul(tabulIn_global, tam);
        res.t_init = wall_time() - time_init_start;
    }

    tabulIn_local = (int *)calloc((tam_local + 2) * (tam + 2), sizeof(int));
    tabulOut_local = (int *)calloc((tam_local + 2) * (tam + 2), sizeof(int));

    MPI_Scatterv((rank == MASTER) ? &tabulIn_global[ind2d(1, 0, tam)] : NULL, elementos_por_processo, deslocamentos, MPI_INT,
                 &tabulIn_local[ind2d(1, 0, tam)], tam_local * (tam + 2), MPI_INT, 0, MPI_COMM_WORLD);


    MPI_Barrier(MPI_COMM_WORLD);
    double time_comp_start = 0;
    if (rank == MASTER) time_comp_start = wall_time();


    int n_geracoes = 2 * (tam - 3);
    for (i = 0; i < n_geracoes; i++) {
        troca_fronteiras(tabulIn_local, tam_local, tam, rank, nprocs);
        UmaVida(tabulIn_local, tabulOut_local, tam_local, tam);

        troca_fronteiras(tabulOut_local, tam_local, tam, rank, nprocs);
        UmaVida(tabulOut_local, tabulIn_local, tam_local, tam); // E esta também
    }
    
    MPI_Barrier(MPI_COMM_WORLD);
    if (rank == MASTER) res.t_comp = wall_time() - time_comp_start;
    
    double time_fim_start = 0;
    if(rank == MASTER) time_fim_start = wall_time();

    MPI_Gatherv(&tabulIn_local[ind2d(1, 0, tam)], tam_local * (tam + 2), MPI_INT,
                (rank == MASTER) ? &tabulIn_global[ind2d(1, 0, tam)] : NULL, elementos_por_processo, deslocamentos, MPI_INT, 0, MPI_COMM_WORLD);

    free(tabulIn_local);
    free(tabulOut_local);

    if (rank == MASTER) {
        res.t_fim = wall_time() - time_fim_start;
        res.correto = Correto(tabulIn_global, tam);
        res.t_total = wall_time() - t_total_inicio;
        res.tam = tam;
        res.nprocs = nprocs;
        res.nthreads = omp_get_max_threads();

        free(tabulIn_global);
        free(elementos_por_processo);
        free(deslocamentos);
    }
    MPI_Barrier(MPI_COMM_WORLD); 
    return res;
}