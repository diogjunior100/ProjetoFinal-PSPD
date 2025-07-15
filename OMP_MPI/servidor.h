#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "resultadosimulacao.h" // Precisa conhecer o struct ResultadoSimulacao

void iniciar_servidor(int rank, int nprocs);
char* formatar_relatorio_final(ResultadoSimulacao* resultados, int n_resultados);
void enviar_metricas_para_log(ResultadoSimulacao* resultados, int n_resultados, const char* client_ip);

#endif