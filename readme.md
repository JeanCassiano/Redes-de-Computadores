# Projeto de Redes de Computadores: Sistema de Combate Multiplayer

Este projeto implementa um sistema de combate entre dois jogadores utilizando sockets para comunicação em rede. O foco é explorar a comunicação cliente-servidor, sincronização de estados e trocas de mensagens em tempo real para criar uma experiência de combate multiplayer.

## Estrutura do Projeto

O projeto é dividido em duas partes principais:

- **Servidor (server.cpp)**: Gerencia a conexão entre os jogadores, processa os comandos de combate e mantém o estado do jogo sincronizado.
- **Cliente (client.cpp)**: Permite que os jogadores se conectem ao servidor, enviem comandos de combate e recebam atualizações sobre o estado do jogo.

## Funcionalidades Principais

- **Conexão via Sockets**: Implementação de uma arquitetura cliente-servidor usando sockets TCP para comunicação em rede local.
- **Sistema de Combate**: Jogadores podem escolher ações (ataque, defesa, habilidades) e o servidor processa essas ações em turnos.
- **Sincronização de Estados**: O servidor mantém o estado do jogo sincronizado e envia atualizações após cada turno.

## Tecnologias Utilizadas

- **Linguagem**: C++
- **Biblioteca de Redes**: Sockets TCP
- **Interface Gráfica**: SFML (opcional, para visualização gráfica do combate)

## Pré-requisitos

- **Compilador C++** (g++ recomendado)
- **Biblioteca SFML** (opcional, para interface gráfica)
- **Sistema Operacional**: Linux, macOS ou Windows com suporte a sockets.

## Como Compilar e Executar

### Compilando o Servidor e o Cliente

1. Compile o servidor usando g++:
   ```bash
   g++ -o server server.cpp
    ```

2. Execute o servidor:

    ```bash
    ./server
    ```

3. Compile o cliente usando g++:

    ```bash
    g++ -o client client.cpp -lsfml-graphics -lsfml-window -lsfml-system
    ```
4. Execute o cliente:

    ```bash
    ./server
    ```

### Uso

1. Inicie o servidor primeiro. Ele ficará aguardando conexões de dois jogadores.

2. Execute o cliente em duas máquinas ou duas instâncias no mesmo computador para conectar ao servidor.
    
3. Os jogadores poderão escolher suas ações de combate. As ações são enviadas ao servidor, que processa o resultado do turno e envia as atualizações de volta.

<br>
<br>
Desenvolvido por Jean Cassiano como parte de um projeto de Redes de Computadores para explorar a comunicação em rede usando sockets e C++.