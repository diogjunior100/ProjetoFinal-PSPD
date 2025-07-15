import socket
import threading
import itertools

ENGINE_ADDRESSES = [
    ('engine-spark', 8001),  # Nome do serviço da engine Spark
    ('engine-mpi', 8002)     # Nome do serviço da engine MPI
]

GATEWAY_HOST = '0.0.0.0'
GATEWAY_PORT = 9999


engine_selector = itertools.cycle(ENGINE_ADDRESSES)

def handle_client(client_socket):
    """Lida com a conexão de um cliente, redirecionando-a para uma engine."""
    try:
        request = client_socket.recv(4096)
        if not request:
            print("[Gateway] Cliente desconectou sem enviar dados.")
            return

        print(f"[Gateway] Recebeu {len(request)} bytes. Redirecionando...")
        target_engine_addr = next(engine_selector)
        print(f"[Gateway] Encaminhando requisição para a engine em {target_engine_addr}")

        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as engine_socket:
            engine_socket.connect(target_engine_addr)
            engine_socket.sendall(request)
            response_from_engine = engine_socket.recv(4096)
            client_socket.sendall(response_from_engine)
            print(f"[Gateway] Resposta da engine enviada ao cliente.")

    except Exception as e:
        print(f"[Gateway] Erro ao lidar com o cliente: {e}")
    finally:
        client_socket.close()

def start_gateway():
    """Inicia o servidor do Gateway."""
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((GATEWAY_HOST, GATEWAY_PORT))
    server_socket.listen(5)
    print(f"[Gateway] Servidor iniciado em {GATEWAY_HOST}:{GATEWAY_PORT}. Aguardando requisições.")

    while True:
        client_conn, client_addr = server_socket.accept()
        print(f"[Gateway] Nova conexão de {client_addr}")
        thread = threading.Thread(target=handle_client, args=(client_conn,))
        thread.start()

if __name__ == '__main__':
    start_gateway()