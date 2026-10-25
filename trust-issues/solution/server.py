
import socket
import ssl


context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
context.load_cert_chain("cert.pem", "key.pem")

server = context.wrap_socket(socket.socket(), True)
server.bind(("0.0.0.0", 8443))
server.listen(1)

client, addr = server.accept()
print(client.recv(4096))
