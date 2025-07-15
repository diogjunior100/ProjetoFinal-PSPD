#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mpi/mpi.h>
#include <time.h>

#include "cJSON/cJSON.h"
#include "servidor.h"

#define PORTA 6666
#define TAM_BUFFER 1024
#define MASTER 0

void iniciar_servidor(int rank, int nprocs) {
    int socket_servidor, socket_cliente;
    struct sockaddr_in endereco_servidor, endereco_cliente;
    socklen_t addr_size = sizeof(endereco_cliente);

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
        char client_ip[INET_ADDRSTRLEN];

        if (rank == MASTER) {
            socket_cliente = accept(socket_servidor, (struct sockaddr*)&endereco_cliente, &addr_size);
            if (socket_cliente < 0) { perror("Erro no accept, continuando..."); continue; }
            
            inet_ntop(AF_INET, &endereco_cliente.sin_addr, client_ip, sizeof(client_ip));
            printf("Cliente conectado do IP: %s\n", client_ip);

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
            // Imprime o relatório formatado na tela do servidor
            char* relatorio_string = formatar_relatorio_final(todos_os_resultados, n_simulacoes);
            printf("\n--- RESULTADO DA SIMULAÇÃO ---\n%s", relatorio_string);

            // Envia o mesmo relatório para o cliente via socket
            write(socket_cliente, relatorio_string, strlen(relatorio_string));
            printf("Resultados enviados para o cliente.\n");

            // NOVO: Envia as métricas detalhadas para o log (stdout) em formato JSON
            enviar_metricas_para_log(todos_os_resultados, n_simulacoes, inet_ntoa(endereco_cliente.sin_addr));
            printf("Métricas de telemetria enviadas para o sistema de logs.\n");

            close(socket_cliente);
            printf("Conexão com o cliente fechada. Aguardando próximo.\n\n");
            
            free(relatorio_string);
            free(todos_os_resultados);
        }
    }
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

void enviar_metricas_para_log(ResultadoSimulacao* resultados, int n_resultados, const char* client_ip) {
    const char* hostname = getenv("HOSTNAME");
    if (hostname == NULL) {
        hostname = "unknown";
    }

    char timestamp[30];
    time_t now = time(NULL);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));

    printf("\n--- Enviando %d métricas para o log (usando cJSON) ---\n", n_resultados);
    for (int i = 0; i < n_resultados; i++) {
        // 1. Cria o objeto JSON principal (a raiz)
        cJSON *root = cJSON_CreateObject();

        // 2. Adiciona os campos ao objeto raiz
        cJSON_AddStringToObject(root, "timestamp", timestamp);
        cJSON_AddStringToObject(root, "engine_type", "MPI/OpenMP");
        cJSON_AddNumberToObject(root, "board_size", resultados[i].tam);
        cJSON_AddNumberToObject(root, "mpi_processes", resultados[i].nprocs);
        cJSON_AddNumberToObject(root, "omp_threads", resultados[i].nthreads);
        cJSON_AddBoolToObject(root, "result_correct", resultados[i].correto);
        cJSON_AddStringToObject(root, "pod_hostname", hostname);

        // 3. Cria objetos aninhados para tempos e info do cliente
        cJSON *execution_times = cJSON_CreateObject();
        cJSON_AddNumberToObject(execution_times, "initialization", resultados[i].t_init);
        cJSON_AddNumberToObject(execution_times, "computation", resultados[i].t_comp);
        cJSON_AddNumberToObject(execution_times, "finalization", resultados[i].t_fim);
        cJSON_AddNumberToObject(execution_times, "total", resultados[i].t_total);

        cJSON *client_info = cJSON_CreateObject();
        cJSON_AddStringToObject(client_info, "ip", client_ip);
        cJSON_AddStringToObject(client_info, "connection_type", "TCP");

        // 4. Adiciona os objetos aninhados ao objeto raiz
        cJSON_AddItemToObject(root, "execution_times", execution_times);
        cJSON_AddItemToObject(root, "client_info", client_info);

        // 5. Converte a estrutura cJSON em uma string formatada
        char *json_string = cJSON_Print(root);
        
        // 6. Imprime a string JSON na saída padrão (stdout)
        printf("%s\n", json_string);

        // 7. Libera a memória alocada pela cJSON (MUITO IMPORTANTE!)
        cJSON_Delete(root);  // Libera a estrutura inteira
        free(json_string); // Libera a string gerada
    }
}
