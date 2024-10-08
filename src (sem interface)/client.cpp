#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char command[100];
    
    // Tentativa de criar o socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error: " << strerror(errno) << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converter o endereço IP para binário e conectar ao servidor
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported: " << strerror(errno) << std::endl;
        close(sock);  // Fecha o socket antes de sair
        return -1;
    }

    // Tentativa de conexão ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed: " << strerror(errno) << std::endl;
        close(sock);  // Fecha o socket antes de sair
        return -1;
    }

    // Receber a mensagem de escolha de classe
    if (read(sock, buffer, 1024) <= 0) {
        std::cerr << "Error receiving class choice message from server: " << strerror(errno) << std::endl;
        close(sock);  // Fecha o socket antes de sair
        return -1;
    }
    std::cout << "Server: " << buffer;

    // Solicita a escolha da classe ao jogador
    int classChoice;
    while (true) {
        std::cin >> classChoice;
        if (classChoice >= 1 && classChoice <= 5) {
            break;
        } else {
            std::cout << "Escolha inválida, tente novamente: ";
        }
    }

    // Converte o valor int para string e envia para o servidor
    snprintf(command, sizeof(command), "%d", classChoice);
    if (send(sock, command, strlen(command), 0) <= 0) {
        std::cerr << "Error sending class choice to server: " << strerror(errno) << std::endl;
        close(sock);  // Fecha o socket antes de sair
        return -1;
    }

    // Receber a confirmação da classe
    if (read(sock, buffer, 1024) <= 0) {
        std::cerr << "Error receiving class confirmation from server: " << strerror(errno) << std::endl;
        close(sock);  // Fecha o socket antes de sair
        return -1;
    }
    std::cout << "Server: " << buffer;

    // Loop principal do jogo
    while (true) {
        // Receber o status do jogo ou prompt de ataque
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(sock, buffer, 1024);
        
        if (bytesRead < 0) {
            std::cerr << "Error receiving message from server: " << strerror(errno) << std::endl;
            break;
        } else if (bytesRead == 0) {
            std::cerr << "Server disconnected unexpectedly." << std::endl;
            break;
        }

        std::cout << "Server: " << buffer;

        // Verificar se o jogo acabou
        if (strstr(buffer, "win") != NULL || strstr(buffer, "lose") != NULL) {
            break;
        }

        // Prompt para o ataque se instruído
        if (strstr(buffer, "Choose your attack") != NULL) {
            int attackChoice;
            while (true) {
                std::cin >> attackChoice;
                if (attackChoice >= 1 && attackChoice <= 3) {
                    break;
                } else {
                    std::cout << "Escolha inválida, tente novamente: ";
                }
            }
            
            snprintf(command, sizeof(command), "%d", attackChoice);
            if (send(sock, command, strlen(command), 0) <= 0) {
                std::cerr << "Error sending attack choice to server: " << strerror(errno) << std::endl;
                break;
            }
        }
    }

    // Fechar o socket antes de sair
    close(sock);
    return 0;
}
