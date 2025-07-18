import socket
import struct
import argparse

def main():
    # Configuração do parser de argumentos
    parser = argparse.ArgumentParser(description="Enviar valores de potência para o gateway.")
    parser.add_argument("powmin", type=int, help="Valor mínimo de potência")
    parser.add_argument("powmax", type=int, help="Valor máximo de potência")
    args = parser.parse_args()

    HOST = 'localhost'
    PORT = 9999

    print(f"--- Conectando no Gateway em {HOST}:{PORT} ---")
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((HOST, PORT))

        print(f"Enviando dados: powmin={args.powmin}, powmax={args.powmax}")
        sock.sendall(struct.pack('ii', args.powmin, args.powmax))

        print("Aguardando resposta final...")
        data = b""
        while True:
            parte = sock.recv(4096)
            if not parte:
                break
            data += parte

        print("\n--- RESPOSTA RECEBIDA DO SISTEMA ---")
        print(data.decode('utf-8', errors='ignore'))
        print("------------------------------------")

if __name__ == "__main__":
    main()
