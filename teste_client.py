import socket
import struct

HOST = 'localhost'
PORT = 9999 

print(f"--- Conectando no Gateway em {HOST}:{PORT} ---")
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect((HOST, PORT))
    
    powmin, powmax = 3, 4
    print(f"Enviando dados: powmin={powmin}, powmax={powmax}")
    sock.sendall(struct.pack('ii', powmin, powmax))

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