#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mpi/mpi.h>
#include <time.h>
#include <curl/curl.h>

#include "cJSON/cJSON.h"
#include "servidor.h"

#define PORTA 6666
#define TAM_BUFFER 1024
#define MASTER 0


void send_request_elastic(const char *url, const char *api_key) {
    CURL *curl;
    CURLcode res;

    // Cria o primeiro JSON: { "index" : { "_index" : "pspd" } }
    cJSON *index_cmd = cJSON_CreateObject();
    cJSON *index_inner = cJSON_CreateObject();
    cJSON_AddStringToObject(index_inner, "_index", "pspd");
    cJSON_AddItemToObject(index_cmd, "index", index_inner);
    char *index_cmd_str = cJSON_PrintUnformatted(index_cmd);

    // Cria o segundo JSON: { "duration": "60", "strategy": "example_strategy" }
    cJSON *doc = cJSON_CreateObject();
    cJSON_AddStringToObject(doc, "duration", "60");
    cJSON_AddStringToObject(doc, "strategy", "example_strategy");
    char *doc_str = cJSON_PrintUnformatted(doc);

    // Monta o corpo NDJSON
    size_t payload_size = strlen(index_cmd_str) + strlen(doc_str) + 3;
    char *payload = malloc(payload_size);
    snprintf(payload, payload_size, "%s\n%s\n", index_cmd_str, doc_str);

    // Prepara libcurl
    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Authorization: ApiKey %s", api_key);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, auth_header);
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, stdout);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "\nErro na requisição: %s\n", curl_easy_strerror(res));
        }

        // Libera recursos
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    // Libera memória alocada
    cJSON_Delete(index_cmd);
    cJSON_Delete(doc);
    free(index_cmd_str);
    free(doc_str);
    free(payload);
}

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

            // Envia as métricas detalhadas para o log (stdout) em formato JSON
            enviar_metricas_para_log(todos_os_resultados, n_simulacoes, inet_ntoa(endereco_cliente.sin_addr));
            printf("Métricas de telemetria enviadas para o sistema de logs.\n");

            const char *url = "https://localhost:9200/pspd";
            const char *api_key = "elastic";
             
            send_request_elastic(url, api_key);

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