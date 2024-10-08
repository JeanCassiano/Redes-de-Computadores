#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <csignal>
#include <chrono>
#include <thread>
#include <stdexcept>

#define PORT 8080
#define MAX_PLAYERS 2
#define RECONNECT_TIMEOUT 30
pthread_t game_thread;
int server_fd;
int client_socket[MAX_PLAYERS];
pthread_t client_threads[MAX_PLAYERS]; // Cada cliente terá sua própria thread

// Função que realiza a limpeza dos recursos e encerra o servidor
// Fecha os sockets de clientes, o socket do servidor, e cancela a thread do jogo
void cleanup()
{
    std::cout << "\nServer is shutting down. Cleaning up resources..." << std::endl;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        if (client_socket[i] > 0)
        {
            close(client_socket[i]);
            std::cout << "Closed client socket " << i + 1 << std::endl;
        }
    }
    if (server_fd > 0)
    {
        close(server_fd);
        std::cout << "Closed server socket." << std::endl;
    }
    exit(0);
}

// Função de tratamento de sinal (SIGINT ou SIGTERM)
// Quando uma interrupção é recebida (Ctrl+C), a função cleanup() é chamada
void signalHandler(int signum)
{
    std::cout << "Interrupt signal (" << signum << ") received." << std::endl;
    cleanup(); // Limpa recursos quando capturado um sinal de kill no terminal
}

// Classe base para jogadores
// Define as propriedades e funções básicas para todos os jogadores
class Player
{
public:
    int hp;
    int attack;
    int speed;
    int chosenAttack;
    std::string attack1Name, attack2Name, attack3Name;

    Player() : chosenAttack(0) {}

    virtual void attack1(Player &opponent) = 0;
    virtual void attack2(Player &opponent) = 0;
    virtual void attack3(Player &opponent) = 0;

    virtual void receiveDamage(int damage)
    {
        hp -= damage;
    }

    virtual ~Player() {}
};

// Classe Guerreira
class Warrior : public Player
{
public:
    Warrior()
    {
        hp = 150;
        attack = 10;
        speed = 5;
        attack1Name = "Sword Attack";
        attack2Name = "Shield Bash";
        attack3Name = "Rally";
    }

    void attack1(Player &opponent) override
    {
        opponent.hp -= 10;
    }

    void attack2(Player &opponent) override
    {
        opponent.hp -= 5;
    }

    void attack3(Player &opponent) override
    {
        hp += 20;
        if (hp > 150)
            hp = 150;
    }
};

// Classe Ladina
class Rogue : public Player
{
public:
    bool stealth;
    Rogue()
    {
        hp = 100;
        attack = 10;
        speed = 10;
        attack1Name = "Quick Attack";
        attack2Name = "Stealth";
        attack3Name = "Backstab";
        stealth = false;
    }

    void attack1(Player &opponent) override
    {
        opponent.hp -= 10;
    }

    void attack2(Player &opponent) override
    {
        stealth = true;
    }

    void attack3(Player &opponent) override
    {
        opponent.hp -= 50;
        hp -= 20;
    }

    void receiveDamage(int damage) override
    {
        if (stealth)
        {
            std::cout << "O ladino esquivou do ataque!!\n";
            stealth = false; // O escudo é quebrado após o primeiro ataque
        }
        else
        {
            hp -= damage;
        }
    }
};

// Classe Mago
class Mage : public Player
{
public:
    bool shieldActive;
    Mage()
    {
        hp = 80;
        attack = 10;
        speed = 7;
        shieldActive = false;
        attack1Name = "Fireball";
        attack2Name = "Frost Nova";
        attack3Name = "Arcane Shield";
    }

    void attack1(Player &opponent) override
    {
        opponent.hp -= 30; // Fireball
    }

    void attack2(Player &opponent) override
    {
        opponent.speed = 0;
    }

    void attack3(Player &opponent) override
    {
        shieldActive = true; // Arcane Shield: metade do dano é absorvido no próximo turno
    }

    void receiveDamage(int damage) override
    {
        if (shieldActive)
        {
            damage /= 2;
            shieldActive = false; // O escudo só dura um turno
        }
        hp -= damage;
    }
};

// Classe Arqueiro
class Archer : public Player
{
public:
    bool eagleEyeActive;

    Archer()
    {
        hp = 100;
        attack = 15;
        speed = 9;
        eagleEyeActive = false;
        attack1Name = "Arrow Shot";
        attack2Name = "Multi-Shot";
        attack3Name = "Eagle Eye";
    }

    void attack1(Player &opponent) override
    {
        opponent.hp -= 20; // Arrow Shot
    }

    void attack2(Player &opponent) override
    {
        if (eagleEyeActive || (rand() % 2 == 0))
        {                      // 50% de chance de acertar se o Eagle Eye não estiver ativo
            opponent.hp -= 15; // Multi-Shot
        }
        else
        {
            std::cout << "Multi-Shot falhou!\n";
        }
    }

    void attack3(Player &opponent) override
    {
        eagleEyeActive = true; // Garante que o próximo ataque não falhe
    }

    void resetEagleEye()
    {
        eagleEyeActive = false;
    }
};

// Classe Paladino
class Paladin : public Player
{
public:
    bool shieldActive;

    Paladin()
    {
        hp = 120;
        attack = 20;
        speed = 5;
        shieldActive = false;
        attack1Name = "Holy Strike";
        attack2Name = "Divine Shield";
        attack3Name = "Lay on Hands";
    }

    void attack1(Player &opponent) override
    {
        opponent.hp -= 25; // Holy Strike
    }

    void attack2(Player &opponent) override
    {
        shieldActive = true; // Absorve completamente o próximo ataque
    }

    void attack3(Player &opponent) override
    {
        hp += 30; // Cura HP
        if (hp > 120)
            hp = 120; // Limita o HP máximo
        speed -= 2;   // Reduz a velocidade permanentemente
    }

    void receiveDamage(int damage) override
    {
        if (shieldActive)
        {
            std::cout << "Divine Shield absorveu o ataque!\n";
            shieldActive = false; // O escudo é quebrado após o primeiro ataque
        }
        else
        {
            hp -= damage;
        }
    }
};

// Estrtura com os elementos necessários para o funcionamento do jogo
struct GameData
{
    int client_socket1;
    int client_socket2;
    Player *player1;
    Player *player2;
    std::string ip_player1;
    std::string ip_player2;
    bool player1_reconnecting = false;
    bool player2_reconnecting = false;
};

// Função para enviar uma mensagem a ambos os clientes conectados
void sendToBothClients(int socket1, int socket2, const std::string &message)
{
    if (send(socket1, message.c_str(), message.length(), 0) < 0)
    {
        std::cerr << "Error sending message to client 1: " << strerror(errno) << std::endl;
    }
    if (send(socket2, message.c_str(), message.length(), 0) < 0)
    {
        std::cerr << "Error sending message to client 2: " << strerror(errno) << std::endl;
    }
}

// Função para executar o ataque escolhido pelo jogador
void executeAttack(Player *attacker, Player *opponent)
{
    switch (attacker->chosenAttack)
    {
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

// Função que envia os ataques de cada classe para o devido jogador
void sendAttackOptions(int client_socket, Player *player)
{
    std::string attackOptions = "Choose your attack: 1 - " + player->attack1Name + ", 2 - " + player->attack2Name + ", 3 - " + player->attack3Name + "\n";
    if (send(client_socket, attackOptions.c_str(), attackOptions.length(), 0) < 0)
    {
        std::cerr << "Error sending attack options: " << strerror(errno) << std::endl;
    }
}

pthread_mutex_t gameStateMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t turnCond = PTHREAD_COND_INITIALIZER; // Condição para sincronizar turnos
int turnCounter = 0;                                // Contador de turnos para sincronizar os dois jogadores

// Estrtura com os dados de cada jogador
struct PlayerData
{
    int client_socket;
    Player *player;
    int opponent_socket; // Socket do oponente
    Player *opponent;    // Aponta para o oponente
    int chosenAttack;
};

// Função que executa a lógica de combate para cada cliente
void *handleClientGame(void *data)
{
    PlayerData *player_data = (PlayerData *)data; // Dados do jogador
    char buffer[1024] = {0};

    while (true)
    {
        // Enviar opções de ataque
        sendAttackOptions(player_data->client_socket, player_data->player);

        // Receber o ataque do jogador
        bool validAttack = false;
        while (!validAttack)
        {
            memset(buffer, 0, sizeof(buffer));
            if (read(player_data->client_socket, buffer, 1024) <= 0)
            {
                std::cerr << "Player disconnected during attack phase. Game over.\n";
                close(player_data->client_socket);

                // Oponente vence automaticamente
                std::string winMsg = "Your opponent has disconnected. You win!\n";
                send(player_data->opponent_socket, winMsg.c_str(), winMsg.length(), 0);

                // Enviar mensagem de confirmação e fechar o jogo
                std::string confirmMsg = "Type 'Exit' to confirm and exit.\n";
                send(player_data->opponent_socket, confirmMsg.c_str(), confirmMsg.length(), 0);

                // Loop de confirmação
                memset(buffer, 0, sizeof(buffer));
                while (true)
                {
                    if (read(player_data->opponent_socket, buffer, 1024) <= 0)
                    {
                        std::cerr << "Opponent disconnected before confirmation.\n";
                        close(player_data->opponent_socket);
                        pthread_exit(NULL);
                    }

                    if (std::string(buffer) == "Exit")
                    {
                        std::cout << "Opponent confirmed game end.\n";
                        break;
                    }
                }

                pthread_exit(NULL); // Finalizar a thread do jogador desconectado
            }

            try {
                player_data->chosenAttack = std::stoi(buffer);
                if (player_data->chosenAttack >= 1 && player_data->chosenAttack <= 3) {
                    validAttack = true; // Entrada válida
                } else {
                    std::string invalidMsg = "Invalid attack choice. Please choose 1, 2, or 3.\n";
                    send(player_data->client_socket, invalidMsg.c_str(), invalidMsg.length(), 0);
                }
            } catch (std::invalid_argument &e) {
                std::string invalidMsg = "Invalid choice. Please enter a number between 1 and 3.\n";
                send(player_data->client_socket, invalidMsg.c_str(), invalidMsg.length(), 0);
            }
        }

        // Bloquear o acesso ao estado do jogo (mutex) para processar o ataque
        pthread_mutex_lock(&gameStateMutex);

        // Executar o ataque baseado na escolha
        switch (player_data->chosenAttack)
        {
        case 1:
            player_data->player->attack1(*player_data->opponent);
            break;
        case 2:
            player_data->player->attack2(*player_data->opponent);
            break;
        case 3:
            player_data->player->attack3(*player_data->opponent);
            break;
        default:
            std::cerr << "Invalid attack choice." << std::endl;
            break;
        }

        // Atualizar o contador de turnos
        turnCounter++;
        if (turnCounter < 2)
        {
            // Se apenas um jogador atacou, esperar o outro
            pthread_cond_wait(&turnCond, &gameStateMutex);
        }
        else
        {
            // Ambos os jogadores atacaram, reiniciar o contador e avisar o outro jogador
            turnCounter = 0;
            pthread_cond_broadcast(&turnCond); // Acorda o outro jogador
        }

        // Enviar resultados para o jogador
        std::string resultMsg = "You used attack " + std::to_string(player_data->chosenAttack) + ". Your HP: " + std::to_string(player_data->player->hp) + ". Opponent HP: " + std::to_string(player_data->opponent->hp) + "\n";
        send(player_data->client_socket, resultMsg.c_str(), resultMsg.length(), 0);

        if (player_data->opponent->hp <= 0 || player_data->player->hp <= 0)
        {
            // Mensagens de vitória ou derrota
            std::string winMsg = "You win! Your opponent was defeated.\n";
            std::string loseMsg = "You lose! You were defeated by your opponent.\n";

            if (player_data->player->hp > 0 && player_data->opponent->hp <= 0)
            {
                // Jogador atual venceu
                send(player_data->client_socket, winMsg.c_str(), winMsg.length(), 0);     // Enviar mensagem de vitória para o jogador
                send(player_data->opponent_socket, loseMsg.c_str(), loseMsg.length(), 0); // Enviar mensagem de derrota para o oponente
            }
            else if (player_data->player->hp <= 0)
            {
                // Jogador atual perdeu
                send(player_data->client_socket, loseMsg.c_str(), loseMsg.length(), 0); // Enviar mensagem de derrota para o jogador
                send(player_data->opponent_socket, winMsg.c_str(), winMsg.length(), 0); // Enviar mensagem de vitória para o oponente
            }

            // Espera o jogador digitar "Exit" para confirmar o término do jogo
            std::string confirmMsg = "Type 'Exit' to confirm and exit.\n";
            send(player_data->client_socket, confirmMsg.c_str(), confirmMsg.length(), 0);
            send(player_data->opponent_socket, confirmMsg.c_str(), confirmMsg.length(), 0);

            // Loop de confirmação
            memset(buffer, 0, sizeof(buffer));
            while (true)
            {
                if (read(player_data->client_socket, buffer, 1024) <= 0)
                {
                    std::cerr << "Player disconnected before confirmation.\n";
                    close(player_data->client_socket);
                    pthread_exit(NULL);
                }

                std::cout << "Player confirmed game end.\n";
                break; // Saída do loop quando o jogador confirmar
            }
            break; // Saída do loop principal do jogo
        }

        pthread_mutex_unlock(&gameStateMutex); // Liberar o mutex após o ataque
    }

    // Fechar o socket do cliente e terminar a thread
    close(player_data->client_socket);
    pthread_exit(NULL);
}


// Função Main
int main()
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Interpreta sinais de kill no terminal
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Cria o servidor Socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        std::cerr << "setsockopt failed" << std::endl;
        close(server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }

    // Habilita o servidor para aceitar conexões
    if (listen(server_fd, MAX_PLAYERS) < 0)
    {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        close(server_fd);
        return -1;
    }

    // Espera até que dois jogadores estejam conectados, após o termino de uma partida, continua esperando mais jogadores conectarem
    while (true)
    {
        std::cout << "Waiting for players to connect..." << std::endl;

        PlayerData players[MAX_PLAYERS];

        // Conecta dois jogadores
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            if ((client_socket[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                std::cerr << "Accept failed: " << strerror(errno) << std::endl;
                cleanup();
                return -1;
            }
            std::cout << "Player " << i + 1 << " connected!" << std::endl;

            // Configurações iniciais de dados do jogador
            players[i].client_socket = client_socket[i];
        }

        // Definir oponente para cada jogador
        players[0].opponent = players[1].player;
        players[1].opponent = players[0].player;

        // Enviar a mensagem de escolha de classe para ambos os jogadores
        std::string chooseClassMsg = "Choose your class: 1 - Warrior, 2 - Rogue, 3 - Mage, 4 - Archer, 5 - Paladin\n";
        sendToBothClients(players[0].client_socket, players[1].client_socket, chooseClassMsg);

        // Receber a escolha de classe de cada jogador
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            int classChoice = 0;
            bool validChoice = false;
            while (!validChoice)
            {
                char buffer[1024] = {0};
                if (read(client_socket[i], buffer, 1024) <= 0)
                {
                    std::cerr << "Player disconnected during class selection." << std::endl;
                    close(client_socket[i]);
                    cleanup();
                    return -1;
                }

                try
                {
                    classChoice = std::stoi(buffer);
                    if (classChoice >= 1 && classChoice <= 5)
                    {
                        validChoice = true; // Entrada válida
                    }
                    else
                    {
                        std::string invalidMsg = "Invalid choice. Please choose a valid class: 1 - Warrior, 2 - Rogue, 3 - Mage, 4 - Archer, 5 - Paladin\n";
                        send(client_socket[i], invalidMsg.c_str(), invalidMsg.length(), 0);
                    }
                }
                catch (std::invalid_argument &e)
                {
                    std::string invalidMsg = "Invalid choice. Please enter a number between 1 and 5.\n";
                    send(client_socket[i], invalidMsg.c_str(), invalidMsg.length(), 0);
                }
            }

            // Atribuir a classe com base na escolha válida
            switch (classChoice)
            {
            case 1:
                players[i].player = new Warrior();
                break;
            case 2:
                players[i].player = new Rogue();
                break;
            case 3:
                players[i].player = new Mage();
                break;
            case 4:
                players[i].player = new Archer();
                break;
            case 5:
                players[i].player = new Paladin();
                break;
            }
            std::string classConfirmMsg = "Class chosen successfully! Waiting for opponent...\n";
            send(players[i].client_socket, classConfirmMsg.c_str(), classConfirmMsg.length(), 0);
        }

        // Configura os oponentes e seus respectivos sockets
        players[0].opponent = players[1].player;
        players[0].opponent_socket = players[1].client_socket; // Socket do oponente

        players[1].opponent = players[0].player;
        players[1].opponent_socket = players[0].client_socket; // Socket do oponente

        // Sincronizar os jogadores e iniciar o combate, tratando cada um dos clientes como uma thread
        for (int i = 0; i < MAX_PLAYERS; i++)
        {
            pthread_create(&client_threads[i], NULL, handleClientGame, (void *)&players[i]);
            pthread_detach(client_threads[i]);
        }
    }

    cleanup();
    return 0;
}
