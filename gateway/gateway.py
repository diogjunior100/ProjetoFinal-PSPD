import socket
import threading
import itertools
import struct

ENGINE_ADDRESSES = [
    ('engine-spark', 6000),   # Nome do serviço da engine Spark
    ('engine-mpi', 6666)     # Nome do serviço da engine MPI
]

GATEWAY_HOST = '0.0.0.0'
GATEWAY_PORT = 9999


engine_selector = itertools.cycle(ENGINE_ADDRESSES)

def handle_client(client_socket):
    """Lida com a conexão de um cliente, traduzindo o protocolo se necessário."""
    try:
        # 1. Recebe os dados BINÁRIOS do cliente final
        request_bytes = client_socket.recv(8)
        if not request_bytes:
            return

        # --- NOVA LÓGICA DE TRADUÇÃO ---

        # 2. Seleciona a próxima engine (Round-Robin)
        target_engine_addr = next(engine_selector)
        print(f"[Gateway] Encaminhando requisição para a engine em {target_engine_addr}")

        # 3. Decide o formato do payload baseado no nome da engine
        if target_engine_addr[0] == 'engine-mpi':
            # Se for a engine MPI, desempacota os bytes e re-empacota como TEXTO
            powmin, powmax = struct.unpack('ii', request_bytes)
            payload = f"{powmin},{powmax}".encode('utf-8')
            print(f"[Gateway] Traduzindo para formato TEXTO para Engine MPI: '{payload.decode()}'")
        else:
            # Se for a engine Spark (ou qualquer outra), mantém o formato BINÁRIO
            payload = request_bytes
            print("[Gateway] Mantendo formato BINÁRIO para Engine Spark.")

        # 4. Conecta na engine e envia o payload no formato correto
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as engine_socket:
            engine_socket.connect(target_engine_addr)
            engine_socket.sendall(payload) # Envia o payload traduzido ou original

            # 5. Recebe a resposta e envia de volta (sem alterações)
            response_from_engine = b""
            while True:
                part = engine_socket.recv(4096)
                if not part:
                    break
                response_from_engine += part

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