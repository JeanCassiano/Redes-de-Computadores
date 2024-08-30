#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <ncurses.h>

#define PORT 8080

// Função para exibir o menu de ataques baseado na classe
int chooseAttack(const std::string& playerClass) {
    const char* optionsWarrior[] = {"1. Sword Attack", "2. Shield Bash", "3. Rally"};
    const char* optionsRogue[] = {"1. Quick Attack", "2. Stealth", "3. Backstab"};
    const char** options;
    int numOptions = 3;
    int choice = 0;
    int ch;

    // Definir opções de ataque com base na classe
    if (playerClass == "Warrior") {
        options = optionsWarrior;
    } else if (playerClass == "Rogue") {
        options = optionsRogue;
    } else {
        std::cerr << "Invalid class." << std::endl;
        return -1;
    }

    initscr();            // Inicia o modo ncurses
    raw();                // Linha de entrada em modo raw
    keypad(stdscr, TRUE); // Ativa captura de teclas especiais
    noecho();             // Não exibe as teclas pressionadas

    while (true) {
        clear(); // Limpa a tela
        printw("Choose your attack:\n");
        for (int i = 0; i < numOptions; ++i) {
            if (i == choice) {
                attron(A_REVERSE); // Inverte as cores para o item selecionado
            }
            printw("%s\n", options[i]);
            if (i == choice) {
                attroff(A_REVERSE);
            }
        }

        ch = getch(); // Captura a tecla pressionada

        switch (ch) {
            case KEY_UP:
                choice = (choice == 0) ? numOptions - 1 : choice - 1;
                break;
            case KEY_DOWN:
                choice = (choice == numOptions - 1) ? 0 : choice + 1;
                break;
            case 10: // Enter
                endwin();
                return choice + 1; // Retorna a escolha do jogador
        }
    }
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char command[100];
    std::string playerClass;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converter o endereço IP para binário e conectar ao servidor
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Receber a mensagem de escolha de classe
    if (read(sock, buffer, 1024) <= 0) {
        std::cerr << "Error receiving class choice message from server." << std::endl;
        return -1;
    }
    std::cout << "Server: " << buffer;

    // Solicita a escolha da classe ao jogador
    int classChoice;
    std::cin >> classChoice;

    // Converte o valor int para string e envia para o servidor
    snprintf(command, sizeof(command), "%d", classChoice);
    send(sock, command, strlen(command), 0);

    // Receber a confirmação da classe
    if (read(sock, buffer, 1024) <= 0) {
        std::cerr << "Error receiving class confirmation from server." << std::endl;
        return -1;
    }
    std::cout << "Server: " << buffer;

    // Armazenar a classe escolhida pelo jogador
    if (classChoice == 1) {
        playerClass = "Warrior";
    } else if (classChoice == 2) {
        playerClass = "Rogue";
    } else {
        std::cerr << "Invalid class received." << std::endl;
        return -1;
    }

    // Loop principal do jogo
    while (true) {
        // Receber o status do jogo ou prompt de ataque
        memset(buffer, 0, sizeof(buffer));
        if (read(sock, buffer, 1024) <= 0) {
            std::cerr << "Error receiving message from server." << std::endl;
            break;
        }
        std::cout << "Server: " << buffer;

        // Verificar se o jogo acabou
        if (strstr(buffer, "win") != NULL || strstr(buffer, "lose") != NULL) {
            break;
        }

        // Prompt para o ataque se instruído
        if (strstr(buffer, "Choose your attack") != NULL) {
            int attackChoice = chooseAttack(playerClass);
            snprintf(command, sizeof(command), "%d", attackChoice);
            send(sock, command, strlen(command), 0);
        }
    }

    close(sock);
    return 0;
}
