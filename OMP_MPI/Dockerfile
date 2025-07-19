FROM debian:bookworm-slim AS builder

WORKDIR /app

RUN apt-get update && apt-get install -y \
    build-essential openmpi-bin libopenmpi-dev libcurl4-openssl-dev && \
    rm -rf /var/lib/apt/lists/*

# Copia TODOS os arquivos .c e .h para o contêiner
COPY . .

# Compila TODOS os arquivos .c necessários juntos
RUN mpicc -o servidor_hibrido main.c servidor.c resultadosimulacao.c jogodavida.c cJSON/cJSON.c -fopenmp -lm -lcurl

RUN chmod +x entrypoint.sh

EXPOSE 6666

ENV NPROCS=4

ENTRYPOINT ["/app/entrypoint.sh"]