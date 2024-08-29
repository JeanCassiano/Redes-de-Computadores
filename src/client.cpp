// CLIENT.CPP

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char command[100];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address and connect to server
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Receive class choice message
    if (read(sock, buffer, 1024) <= 0) {
        std::cerr << "Error receiving class choice message from server." << std::endl;
        return -1;
    }
    std::cout << "Server: " << buffer;

    // Send class choice
    std::cin.getline(command, sizeof(command));
    send(sock, command, strlen(command), 0);

    // Receive class confirmation
    if (read(sock, buffer, 1024) <= 0) {
        std::cerr << "Error receiving class confirmation from server." << std::endl;
        return -1;
    }
    std::cout << "Server: " << buffer;

    // Main game loop
    while (true) {
        // Receive game status or attack prompt
        memset(buffer, 0, sizeof(buffer));
        if (read(sock, buffer, 1024) <= 0) {
            std::cerr << "Error receiving message from server." << std::endl;
            break;
        }
        std::cout << "Server: " << buffer;

        // Check if game is over
        if (strstr(buffer, "win") != NULL || strstr(buffer, "lose") != NULL) {
            break;
        }

        // Prompt for attack if instructed
        if (strstr(buffer, "Choose your attack") != NULL) {
            std::cin.getline(command, sizeof(command));
            send(sock, command, strlen(command), 0);
        }
    }

    close(sock);
    return 0;
}
