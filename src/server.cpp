// SERVER.CPP

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdexcept>

#define PORT 8080
#define MAX_PLAYERS 2

class Player {
public:
    int hp;
    int attack;
    int speed;
    int chosenAttack;

    Player() : chosenAttack(0) {}

    virtual void attack1(Player &opponent) = 0;
    virtual void attack2(Player &opponent) = 0;
    virtual void attack3(Player &opponent) = 0;

    virtual ~Player() {}
};

class Warrior : public Player {
public:
    Warrior() {
        hp = 150;
        attack = 10;
        speed = 5;
    }

    void attack1(Player &opponent) override {
        std::cout << "Warrior uses Sword Attack!" << std::endl;
        opponent.hp -= 10;
    }

    void attack2(Player &opponent) override {
        std::cout << "Warrior uses Shield Bash!" << std::endl;
        opponent.hp -= 5;
    }

    void attack3(Player &opponent) override {
        std::cout << "Warrior uses Rally!" << std::endl;
        hp += 20;
        if (hp > 150) hp = 150;
    }
};

class Rogue : public Player {
public:
    Rogue() {
        hp = 100;
        attack = 10;
        speed = 10;
    }

    void attack1(Player &opponent) override {
        std::cout << "Rogue uses Quick Attack!" << std::endl;
        opponent.hp -= 10;
    }

    void attack2(Player &opponent) override {
        std::cout << "Rogue uses Stealth!" << std::endl;
    }

    void attack3(Player &opponent) override {
        std::cout << "Rogue uses Backstab!" << std::endl;
        opponent.hp -= 50;
        hp -= 20;
    }
};

struct GameData {
    int client_socket1;
    int client_socket2;
    Player* player1;
    Player* player2;
};

void sendToBothClients(int socket1, int socket2, const std::string &message) {
    send(socket1, message.c_str(), message.length(), 0);
    send(socket2, message.c_str(), message.length(), 0);
}

void executeAttack(Player* attacker, Player* opponent) {
    switch (attacker->chosenAttack) {
        case 1:
            attacker->attack1(*opponent);
            break;
        case 2:
            attacker->attack2(*opponent);
            break;
        case 3:
            attacker->attack3(*opponent);
            break;
        default:
            std::cerr << "Invalid attack choice." << std::endl;
            break;
    }
}

void *handleGame(void *data) {
    GameData *game_data = (GameData *)data;
    char buffer[1024] = {0};

    while (game_data->player1->hp > 0 && game_data->player2->hp > 0) {
        std::cout << "Starting new round." << std::endl;

        // Notificar ambos os jogadores para escolherem seus ataques
        sendToBothClients(game_data->client_socket1, game_data->client_socket2, "Choose your attack (1, 2, 3):\n");

        // Receber ataque do primeiro jogador
        memset(buffer, 0, sizeof(buffer));
        if (read(game_data->client_socket1, buffer, 1024) <= 0) {
            std::cerr << "Error reading from client 1." << std::endl;
            break;
        }
        game_data->player1->chosenAttack = std::stoi(buffer);
        send(game_data->client_socket1, "Wait for your opponent...\n", 26, 0);

        // Receber ataque do segundo jogador
        memset(buffer, 0, sizeof(buffer));
        if (read(game_data->client_socket2, buffer, 1024) <= 0) {
            std::cerr << "Error reading from client 2." << std::endl;
            break;
        }
        game_data->player2->chosenAttack = std::stoi(buffer);
        send(game_data->client_socket2, "Wait for your opponent...\n", 26, 0);

        // Executar os ataques
        executeAttack(game_data->player1, game_data->player2);
        if (game_data->player2->hp > 0) {
            executeAttack(game_data->player2, game_data->player1);
        }

        // Enviar status do jogo para ambos os jogadores
        std::string statusMsg = "End of round. Player 1 HP: " + std::to_string(game_data->player1->hp) + 
                                ", Player 2 HP: " + std::to_string(game_data->player2->hp) + "\n";
        sendToBothClients(game_data->client_socket1, game_data->client_socket2, statusMsg);

        if (game_data->player1->hp <= 0 || game_data->player2->hp <= 0) {
            std::string endMsg1 = (game_data->player1->hp <= 0 ? "You lose! You were defeated.\n" : "You win! You defeated your opponent.\n");
            std::string endMsg2 = (game_data->player2->hp <= 0 ? "You lose! You were defeated.\n" : "You win! You defeated your opponent.\n");

            send(game_data->client_socket1, endMsg1.c_str(), endMsg1.length(), 0);
            send(game_data->client_socket2, endMsg2.c_str(), endMsg2.length(), 0);
            break;
        }
    }

    pthread_exit(NULL);
}

int main() {
    int server_fd, client_socket[MAX_PLAYERS];
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t game_thread;
    GameData game_data;

    // Criar o socket do servidor
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind do socket à porta especificada
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        return -1;
    }

    // Ouvir por conexões
    if (listen(server_fd, MAX_PLAYERS) < 0) {
        std::cerr << "Listen failed" << std::endl;
        return -1;
    }

    // Aceitar as conexões dos jogadores
    for (int i = 0; i < MAX_PLAYERS; i++) {
        std::cout << "Waiting for player " << i + 1 << " to connect..." << std::endl;
        client_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (client_socket[i] < 0) {
            std::cerr << "Accept failed" << std::endl;
            return -1;
        }
        std::cout << "Player " << i + 1 << " connected!" << std::endl;
    }

    // Jogadores escolhem classes
    for (int i = 0; i < MAX_PLAYERS; i++) {
        std::string classMsg = "Choose your class: 1 for Warrior, 2 for Rogue\n";
        send(client_socket[i], classMsg.c_str(), classMsg.length(), 0);
        char buffer[1024] = {0};
        if (read(client_socket[i], buffer, 1024) <= 0) {
            std::cerr << "Error reading class choice from client." << std::endl;
            return -1;
        }
        int choice = std::stoi(buffer);

        if (i == 0) {  // Primeiro jogador
            game_data.player1 = (choice == 1) ? static_cast<Player*>(new Warrior()) : static_cast<Player*>(new Rogue());
        } else {  // Segundo jogador
            game_data.player2 = (choice == 1) ? static_cast<Player*>(new Warrior()) : static_cast<Player*>(new Rogue());
        }

        std::string response = "Player chosen with HP: " + std::to_string((i == 0 ? game_data.player1 : game_data.player2)->hp) +
                               ", Attack: " + std::to_string((i == 0 ? game_data.player1 : game_data.player2)->attack) +
                               ", Speed: " + std::to_string((i == 0 ? game_data.player1 : game_data.player2)->speed) + "\n";
        send(client_socket[i], response.c_str(), response.length(), 0);
    }

    // Configuração dos dados do jogo
    game_data.client_socket1 = client_socket[0];
    game_data.client_socket2 = client_socket[1];

    pthread_create(&game_thread, NULL, handleGame, (void *)&game_data);
    pthread_join(game_thread, NULL);

    delete game_data.player1;
    delete game_data.player2;

    close(server_fd);
    return 0;
}
