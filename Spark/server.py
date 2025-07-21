import time
import socket
import threading
import struct
from pyspark import SparkConf, SparkContext


def wall_time():
    return time.time()

def ind2d(i, j, tam):
    return i * (tam + 2) + j

def InitTabul(tam):
    """Inicializa o tabuleiro e retorna como lista."""
    tabul = [0] * ((tam + 2) * (tam + 2))
    tabul[ind2d(1, 2, tam)] = 1
    tabul[ind2d(2, 3, tam)] = 1
    tabul[ind2d(3, 1, tam)] = 1
    tabul[ind2d(3, 2, tam)] = 1
    tabul[ind2d(3, 3, tam)] = 1
    return tabul

def Correto(tabul, tam):
    cnt = sum(tabul)
    return (
        cnt == 5 and
        tabul[ind2d(tam-2, tam-1, tam)] and
        tabul[ind2d(tam-1, tam, tam)] and
        tabul[ind2d(tam, tam-2, tam)] and
        tabul[ind2d(tam, tam-1, tam)] and
        tabul[ind2d(tam, tam, tam)]
    )

def get_neighbors(i, j, tam):
    return [
        (i-1, j-1), (i-1, j), (i-1, j+1),
        (i, j-1),           (i, j+1),
        (i+1, j-1), (i+1, j), (i+1, j+1)
    ]

def UmaVidaSpark(sc, tabul, tam):
    """Realiza uma geração do jogo da vida com Spark"""
    def process(cell):
        i, j = cell
        idx = ind2d(i, j, tam)
        cell_value = tabul[idx]

        neighbors = get_neighbors(i, j, tam)
        vizviv = sum(tabul[ind2d(x, y, tam)] for x, y in neighbors)

        if cell_value and vizviv < 2:
            return (idx, 0)
        elif cell_value and vizviv > 3:
            return (idx, 0)
        elif not cell_value and vizviv == 3:
            return (idx, 1)
        else:
            return (idx, cell_value)

    coords = [(i, j) for i in range(1, tam + 1) for j in range(1, tam + 1)]
    rdd = sc.parallelize(coords)
    updates = rdd.map(process).collect()

    new_tab = tabul.copy()
    for idx, val in updates:
        new_tab[idx] = val
    return new_tab

def main(powmin, powmax):
    conf = SparkConf().setAppName("GameOfLife").setMaster("spark://spark-master:7077").set("spark.executor.memory", "3g").set("spark.executor.cores", "2").set("spark.cores.max", "4")
    sc = SparkContext(conf=conf)

    output = ""

    for pow_ in range(powmin, powmax + 1):
        tam = 1 << pow_
        t0 = wall_time()
        tabul = InitTabul(tam)
        t1 = wall_time()

        for _ in range(2 * (tam - 3)):
            tabul = UmaVidaSpark(sc, tabul, tam)
            tabul = UmaVidaSpark(sc, tabul, tam)

        t2 = wall_time()

        if Correto(tabul, tam):
            result = "**Ok, RESULTADO CORRETO**"
        else:
            result = "**Nok, RESULTADO ERRADO**"

        t3 = wall_time()

        msg = (f"{result}\ntam={tam}; tempos: "
               f"init={t1 - t0:.7f}, comp={t2 - t1:.7f}, fim={t3 - t2:.7f}, tot={t3 - t0:.7f}")
        print(msg)
        output += msg + "\n"

    sc.stop()
    return output

def handle_client(conn, addr):
    print(f"[+] Cliente conectado: {addr}")
    try:
        data = conn.recv(8)
        if len(data) < 8:
            print("Erro: dados incompletos.")
            return

        powmin, powmax = struct.unpack('ii', data)
        print(f"Recebido de {addr}: POWMIN={powmin}, POWMAX={powmax}")

        result = main(powmin, powmax)
        conn.sendall(result.encode())

    finally:
        conn.close()

def start_server(host='0.0.0.0', port=6000):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((host, port))
        server_socket.listen()
        print(f"Servidor Spark iniciado em {host}:{port}")

        while True:
            conn, addr = server_socket.accept()
            thread = threading.Thread(target=handle_client, args=(conn, addr))
            thread.start()

if __name__ == "__main__":
    start_server()