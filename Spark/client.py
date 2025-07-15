import socket
import struct

HOST = 'localhost'
PORT = 6000

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
    sock.connect((HOST, PORT))
    powmin, powmax = 3, 10
    sock.sendall(struct.pack('ii', powmin, powmax))

    data = b""
    while True:
        part = sock.recv(4096)
        if not part:
            break
        data += part
        print(f"{data.decode()}", end='')

    print("Resposta do servidor:")
    print(data.decode())
