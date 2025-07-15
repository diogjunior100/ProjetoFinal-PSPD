#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mpi/mpi.h>
#include <sys/time.h>
#include <omp.h>

#include "cJSON.h"

#define PORTA 6666
#define TAM_BUFFER 1024

#define ind2d(i, j, largura_total) (i) * ((largura_total) + 2) + (j)

#define MASTER 0
#define TAG_CIMA_PARA_BAIXO 0
#define TAG_BAIXO_PARA_CIMA 1

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

double wall_time(void) {
    struct timeval tv;
    struct timezone tz;
    gettimeofday(&tv, &tz);
    return (tv.tv_sec + tv.tv_usec / 1000000.0);
}

void UmaVida(int *tabulIn, int *tabulOut, int tam_local, int tam_global) {
    int i, j, vizviv;
    #pragma omp parallel for private(j, vizviv) shared(tabulIn, tabulOut, tam_local, tam_global)
    for (i = 1; i <= tam_local; i++) {
        for (j = 1; j <= tam_global; j++) {
            vizviv = tabulIn[ind2d(i - 1, j - 1, tam_global)] + tabulIn[ind2d(i - 1, j, tam_global)] +
                     tabulIn[ind2d(i - 1, j + 1, tam_global)] + tabulIn[ind2d(i, j - 1, tam_global)] +
                     tabulIn[ind2d(i, j + 1, tam_global)] + tabulIn[ind2d(i + 1, j - 1, tam_global)] +
                     tabulIn[ind2d(i + 1, j, tam_global)] + tabulIn[ind2d(i + 1, j + 1, tam_global)];
            if (tabulIn[ind2d(i, j, tam_global)] && vizviv < 2)
                tabulOut[ind2d(i, j, tam_global)] = 0;
            else if (tabulIn[ind2d(i, j, tam_global)] && vizviv > 3)
                tabulOut[ind2d(i, j, tam_global)] = 0;
            else if (!tabulIn[ind2d(i, j, tam_global)] && vizviv == 3)
                tabulOut[ind2d(i, j, tam_global)] = 1;
            else
                tabulOut[ind2d(i, j, tam_global)] = tabulIn[ind2d(i, j, tam_global)];
        }
    }
}

void InitTabul(int *tabulIn, int tam) {
    int ij;
    for (ij = 0; ij < (tam + 2) * (tam + 2); ij++) 
        tabulIn[ij] = 0;

    tabulIn[ind2d(1, 2, tam)] = 1;
    tabulIn[ind2d(2, 3, tam)] = 1; 
    tabulIn[ind2d(3, 1, tam)] = 1;
    tabulIn[ind2d(3, 2, tam)] = 1;
    tabulIn[ind2d(3, 3, tam)] = 1;
}

int Correto(int *tabul, int tam) {
    int ij, cnt = 0;
    for (ij = 0; ij < (tam + 2) * (tam + 2); ij++)
        cnt = cnt + tabul[ij];
    
    return (cnt == 5 && tabul[ind2d(tam - 2, tam - 1, tam)] && 
            tabul[ind2d(tam - 1, tam, tam)] && tabul[ind2d(tam, tam - 2, tam)] &&
            tabul[ind2d(tam, tam - 1, tam)] && tabul[ind2d(tam, tam, tam)]);
}

void troca_fronteiras(int* tabul, int tam_local, int tam_global, int rank, int nprocs) {
    int vizinho_cima = (rank == MASTER) ? MPI_PROC_NULL : rank - 1;
    int vizinho_baixo = (rank == nprocs - 1) ? MPI_PROC_NULL : rank + 1;
    MPI_Status status;
    int largura_linha = tam_global + 2;
    
    MPI_Sendrecv(&tabul[ind2d(1, 0, tam_global)], largura_linha, MPI_INT, vizinho_cima, TAG_BAIXO_PARA_CIMA,
                 &tabul[ind2d(0, 0, tam_global)], largura_linha, MPI_INT, vizinho_cima, TAG_CIMA_PARA_BAIXO, MPI_COMM_WORLD, &status);
    
    MPI_Sendrecv(&tabul[ind2d(tam_local, 0, tam_global)], largura_linha, MPI_INT, vizinho_baixo, TAG_CIMA_PARA_BAIXO,
                 &tabul[ind2d(tam_local + 1, 0, tam_global)], largura_linha, MPI_INT, vizinho_baixo, TAG_BAIXO_PARA_CIMA, MPI_COMM_WORLD, &status);
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

char* formatar_relatorio_final(ResultadoSimulacao* resultados, int n_resultados) {
    char* buffer_relatorio = (char*) malloc(n_resultados * 200 * sizeof(char) + 500);
    char linha[200];
    
    strcpy(buffer_relatorio, "\n====================================================================================================\n");
    strcat(buffer_relatorio, "                                     RELATÓRIO DE EXECUÇÃO\n");
    strcat(buffer_relatorio, "====================================================================================================\n");
    strcat(buffer_relatorio, "| TAM   | Procs | Threads | t_init (s) | t_comp (s) | t_fim (s)  | t_total (s)| Resultado |\n");
    strcat(buffer_relatorio, "|-------|-------|---------|------------|------------|------------|------------|-----------|\n");

    for (int i = 0; i < n_resultados; i++) {
        sprintf(linha, "| %-5d | %-5d | %-7d | %-10.7f | %-10.7f | %-10.7f | %-10.7f | %-9s |\n",
               resultados[i].tam, resultados[i].nprocs, resultados[i].nthreads,
               resultados[i].t_init, resultados[i].t_comp, resultados[i].t_fim,
               resultados[i].t_total, resultados[i].correto ? "CORRETO" : "ERRADO");
        strcat(buffer_relatorio, linha);
    }
    strcat(buffer_relatorio, "====================================================================================================\n");
    return buffer_relatorio;
}


int main(int argc, char *argv[]) {
    int rank, nprocs;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int socket_servidor, socket_cliente;
    struct sockaddr_in endereco_servidor;

    if (rank == MASTER) {
        socket_servidor = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_servidor < 0) { perror("Erro ao criar socket"); MPI_Abort(MPI_COMM_WORLD, 1); }
        printf("Socket do servidor criado com sucesso.\n");

        memset(&endereco_servidor, '\0', sizeof(endereco_servidor));
        endereco_servidor.sin_family = AF_INET;
        endereco_servidor.sin_port = htons(PORTA);
        endereco_servidor.sin_addr.s_addr = INADDR_ANY;

        if (bind(socket_servidor, (struct sockaddr*)&endereco_servidor, sizeof(endereco_servidor)) < 0) {
            perror("Erro no bind"); MPI_Abort(MPI_COMM_WORLD, 1);
        }
        printf("Bind na porta %d feito com sucesso.\n", PORTA);

        if (listen(socket_servidor, 10) == 0) {
            printf("Aguardando conexões de clientes...\n");
        } else {
            perror("Erro no listen"); MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    while (1) {
        int pows[2] = {0, 0};

        if (rank == MASTER) {
            socket_cliente = accept(socket_servidor, NULL, NULL);
            if (socket_cliente < 0) { perror("Erro no accept, continuando..."); continue; }
            
            char buffer[TAM_BUFFER] = {0};
            read(socket_cliente, buffer, TAM_BUFFER);
            sscanf(buffer, "%d,%d", &pows[0], &pows[1]);
            printf("Recebido do cliente: pow_min=%d, pow_max=%d\n", pows[0], pows[1]);
        }

        MPI_Bcast(pows, 2, MPI_INT, MASTER, MPI_COMM_WORLD);
        
        if (pows[0] == 0) {
            if (rank == MASTER) printf("Comando de encerramento recebido. Desligando servidor.\n");
            break;
        }

        int n_simulacoes = pows[1] - pows[0] + 1;
        ResultadoSimulacao* todos_os_resultados = NULL;
        if (rank == MASTER) {
            todos_os_resultados = (ResultadoSimulacao*) malloc(n_simulacoes * sizeof(ResultadoSimulacao));
        }

        for (int i = 0; i < n_simulacoes; i++) {
            int pow = pows[0] + i;
            int tam = 1 << pow;
            ResultadoSimulacao resultado_atual = executar_simulacao(tam, rank, nprocs);
            if (rank == MASTER) {
                todos_os_resultados[i] = resultado_atual;
            }
        }

        if (rank == MASTER) {
            char* relatorio_string = formatar_relatorio_final(todos_os_resultados, n_simulacoes);
            
            printf("\n--- RESULTADO DA SIMULAÇÃO ---\n%s", relatorio_string);
            write(socket_cliente, relatorio_string, strlen(relatorio_string));
            printf("Resultados enviados para o cliente.\n");

            close(socket_cliente);
            printf("Conexão com o cliente fechada. Aguardando próximo.\n\n");
            
            free(relatorio_string);
            free(todos_os_resultados);
        }
    }

    if (rank == MASTER) {
        close(socket_servidor);
    }
    MPI_Finalize();
    return 0;
}