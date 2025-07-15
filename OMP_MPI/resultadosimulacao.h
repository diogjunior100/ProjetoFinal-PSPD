#ifndef SIMULACAO_H
#define SIMULACAO_H

#include <sys/time.h> // Para wall_time

// Estrutura para armazenar o resultado de UMA simulação.
typedef struct {
    int tam;
    int nprocs;
    int nthreads;
    double t_init;
    double t_comp;
    double t_fim;
    double t_total;
    int correto;
} ResultadoSimulacao;

double wall_time(void);
ResultadoSimulacao executar_simulacao(int tam, int rank, int nprocs);

#endif // SIMULACAO_H