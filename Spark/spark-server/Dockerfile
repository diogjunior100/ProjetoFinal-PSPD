FROM bitnami/spark:3.5.0

USER root

# Instala Python e pip (já vem com Spark do Bitnami)
RUN apt-get update && apt-get install -y python3 python3-pip

# Copia o código
WORKDIR /app
COPY server.py .

# Instala o pyspark
RUN pip3 install pyspark

CMD ["python3", "server.py"]