#!/bin/sh

# Define o valor da variável de ambiente NPROCS (numero de processos).
# Se a variável não for definida, usa o valor padrão '4'.
NPROCS=${NPROCS:-4}

# Executa o comando mpirun usando o número de processos da variável.
exec mpirun --allow-run-as-root -np "$NPROCS" ./servidor_hibrido