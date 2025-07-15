# gateway/mock_engine.py
import socket
import sys

def start_engine(port):
    """Inicia uma engine de teste em uma porta específica."""
    host = '0.0.0.0'  # localhost

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((host, port))
        s.listen()
        print(f"[Engine na Porta {port}] Aguardando conexões...")

        while True:
            conn, addr = s.accept()
            with conn:
                print(f"[Engine na Porta {port}] Conexão recebida de {addr}")
                data = conn.recv(1024)
                if not data:
                    break
                
                print(f"[Engine na Porta {port}] Recebeu dados: {data.decode('utf-8')}")
                
                # Responde para o Gateway
                response_message = f"Processado pela engine na porta {port}".encode('utf-8')
                conn.sendall(response_message)

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Erro: Forneça o número da porta como argumento.")
        print("Uso: python mock_engine.py <porta>")
        sys.exit(1)
        
    try:
        engine_port = int(sys.argv[1])
        start_engine(engine_port)
    except ValueError:
        print("Erro: A porta deve ser um número.")