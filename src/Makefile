# Compilador a ser utilizado
CXX = g++

# Flags de compilação
CXXFLAGS = -Wall -g

# Alvo para compilar todos os binários (servidor e cliente)
all: server client

# Compila o servidor
server: server.o
	$(CXX) $(CXXFLAGS) -o server server.o -lpthread

# Compila o cliente e faz a linkagem com as bibliotecas GTK
client: client.o
	$(CXX) $(CXXFLAGS) client.o -o client `pkg-config --libs gtk+-3.0`

# Gera o arquivo objeto do servidor
server.o: server.cpp
	$(CXX) $(CXXFLAGS) -c server.cpp

# Gera o arquivo objeto do cliente
client.o: client.cpp
	$(CXX) $(CXXFLAGS) `pkg-config --cflags gtk+-3.0` -c client.cpp

# Regra para executar o servidor e dois clientes em terminais diferentes
run: server client
	gnome-terminal -- bash -c "./server; exec bash" & \
	sleep 2; \
	gnome-terminal -- bash -c "./client; exec bash" & \
	sleep 1; \
	gnome-terminal -- bash -c "./client; exec bash"

# Limpa arquivos compilados (binários e objetos)
clean:
	rm -f *.o server client
