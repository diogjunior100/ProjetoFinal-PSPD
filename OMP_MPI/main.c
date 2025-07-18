#include <mpi/mpi.h>
#include "servidor.h"

int main(int argc, char *argv[]) {
    int rank, nprocs;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    iniciar_servidor(rank, nprocs);

    MPI_Finalize();
    
    return 0;
}