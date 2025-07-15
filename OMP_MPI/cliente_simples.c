// Salve como: cliente_simples.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define TAM_BUFFER 4096 // Buffer maior para receber o relatório

int main(int argc, char *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <IP do Servidor> <porta_servico> <pow_min> <pow_max>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 127.0.0.1 8000 3 8\n", argv[0]);
        exit(1);
    }

    char* ip_servidor = argv[1];
    int portaServico = atoi(argv[2]);
    int pow_min = atoi(argv[3]);
    int pow_max = atoi(argv[4]);

    int socket_cliente;
    struct sockaddr_in endereco_servidor;
    char buffer_envio[50];
    char buffer_recebimento[TAM_BUFFER];

    // 1. Criar Socket
    socket_cliente = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_cliente < 0) {
        perror("Erro ao criar socket");
        exit(1);
    }
    printf("Socket do cliente criado.\n");

    // 2. Conectar ao Servidor
    memset(&endereco_servidor, '\0', sizeof(endereco_servidor));
    endereco_servidor.sin_family = AF_INET;
    endereco_servidor.sin_port = htons(portaServico);
    endereco_servidor.sin_addr.s_addr = inet_addr(ip_servidor);

    if (connect(socket_cliente, (struct sockaddr*)&endereco_servidor, sizeof(endereco_servidor)) < 0) {
        perror("Erro na conexão");
        exit(1);
    }
    printf("Conectado ao servidor %s na portaServico %d.\n", ip_servidor, portaServico);

    // 3. Enviar dados para o servidor
    sprintf(buffer_envio, "%d,%d", pow_min, pow_max);
    if (write(socket_cliente, buffer_envio, strlen(buffer_envio)) < 0) {
        perror("Erro ao enviar dados");
        exit(1);
    }
    printf("Dados (%s) enviados para o servidor.\n", buffer_envio);
    printf("Aguardando resultados...\n\n");

    // 4. Receber a resposta do servidor
    memset(buffer_recebimento, '\0', TAM_BUFFER);
    int bytes_lidos = read(socket_cliente, buffer_recebimento, TAM_BUFFER - 1);
    if (bytes_lidos < 0) {
        perror("Erro ao ler resposta");
    } else if (bytes_lidos == 0) {
        printf("O servidor fechou a conexão sem enviar dados.\n");
    } else {
        // Imprime o relatório recebido
        printf("%s", buffer_recebimento);
    }

    // 5. Fechar o socket
    close(socket_cliente);
    printf("\nConexão fechada.\n");

    return 0;
}