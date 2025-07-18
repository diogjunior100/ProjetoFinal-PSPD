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
    tabul = [0] * ((tam + 2) * (tam + 2))
    tabul[ind2d(1, 2, tam)] = 1; tabul[ind2d(2, 3, tam)] = 1
    tabul[ind2d(3, 1, tam)] = 1; tabul[ind2d(3, 2, tam)] = 1
    tabul[ind2d(3, 3, tam)] = 1
    return tabul
def Correto(tabul, tam):
    cnt = sum(tabul)
    return (cnt == 5 and tabul[ind2d(tam-2, tam-1, tam)] and
            tabul[ind2d(tam-1, tam, tam)] and tabul[ind2d(tam, tam-2, tam)] and
            tabul[ind2d(tam, tam-1, tam)] and tabul[ind2d(tam, tam, tam)])
def get_neighbors(i, j, tam):
    return [(i-1, j-1), (i-1, j), (i-1, j+1), (i, j-1), (i, j+1),
            (i+1, j-1), (i+1, j), (i+1, j+1)]
def UmaVidaSpark(sc, tabul, tam):
    def process(cell):
        i, j = cell; idx = ind2d(i, j, tam); cell_value = tabul[idx]
        neighbors = get_neighbors(i, j, tam)
        vizviv = sum(tabul[ind2d(x, y, tam)] for x, y in neighbors)
        if cell_value and (vizviv < 2 or vizviv > 3): return (idx, 0)
        if not cell_value and vizviv == 3: return (idx, 1)
        return (idx, cell_value)
    coords = [(i, j) for i in range(1, tam + 1) for j in range(1, tam + 1)]
    rdd = sc.parallelize(coords)
    updates = rdd.map(process).collect()
    new_tab = tabul.copy()
    for idx, val in updates: new_tab[idx] = val
    return new_tab


def run_game_of_life(sc, powmin, powmax):
    output = ""
    for pow_ in range(powmin, powmax + 1):
        tam = 1 << pow_
        t0, tabul = wall_time(), InitTabul(tam)
        t1 = wall_time()
        for _ in range(2 * (tam - 3)):
            tabul = UmaVidaSpark(sc, tabul, tam)
            tabul = UmaVidaSpark(sc, tabul, tam)
        t2 = wall_time()
        result = "**Ok, RESULTADO CORRETO**" if Correto(tabul, tam) else "**Nok, RESULTADO ERRADO**"
        t3 = wall_time()
        msg = (f"{result}\ntam={tam}; tempos: init={t1-t0:.7f}, comp={t2-t1:.7f}, fim={t3-t2:.7f}, tot={t3-t0:.7f}")
        print(msg)
        output += msg + "\n"
    # MUDANÇA 2: Removemos o sc.stop() daqui
    return output

def handle_client(conn, addr, sc):
    print(f"[+] Cliente conectado: {addr}")
    try:
        data = conn.recv(8)
        if len(data) < 8:
            print("Erro: dados incompletos.")
            return

        powmin, powmax = struct.unpack('ii', data)
        print(f"Recebido de {addr}: POWMIN={powmin}, POWMAX={powmax}")

        result = run_game_of_life(sc, powmin, powmax)
        conn.sendall(result.encode())
        print(f"[*] Resposta enviada para {addr}")

    except Exception as e:
        import traceback
        traceback.print_exc()
        print(f"[!] Erro ao lidar com o cliente {addr}: {e}")
    finally:
        conn.close()
        print(f"[-] Conexão com {addr} fechada.")

def start_server(host='0.0.0.0', port=6000):
    conf = SparkConf() \
        .setAppName("GameOfLife") \
        .setMaster("spark://cluster-with-template-master-svc:7077")
    #     .set("spark.executor.memory", "4G") \
    #     .set("spark.executor.cores", "2") \
    #     .set("spark.executor.instances", "3")
    sc = SparkContext(conf=conf)
    sc.setLogLevel("WARN")
    print("[*] SparkContext criado e pronto.")

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
            server_socket.bind((host, port))
            server_socket.listen()
            print(f"[*] Servidor Spark aguardando conexões em {host}:{port}")

            while True:
                conn, addr = server_socket.accept()
                thread = threading.Thread(target=handle_client, args=(conn, addr, sc))
                thread.start()
    finally:
        print("[*] Parando o SparkContext...")
        sc.stop()

if __name__ == "__main__":
    start_server()