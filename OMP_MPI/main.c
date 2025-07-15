#include <mpi/mpi.h>
#include "servidor.h" // Inclui a nossa lógica de servidor

int main(int argc, char *argv[]) {
    int rank, nprocs;

    // Inicializa o ambiente MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    // Chama a função principal que inicia o servidor
    iniciar_servidor(rank, nprocs);

    // Finaliza o ambiente MPI
    MPI_Finalize();
    
    return 0;
}