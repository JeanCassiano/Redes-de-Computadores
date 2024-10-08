# Projeto de Redes de Computadores: Sistema de Combate Multiplayer

## Integrantes:

    - Jean Carlos Pereira Cassiano - NUSP: 13864008
    - Lucas Corlete Alves de Melo - NUSP: 13676461
    - Giovanna de Freitas Velasco - NUSP: 13676346
    - João Victor de Almeida - NUSP 13695424

    
Este projeto implementa um sistema de combate multiplayer entre dois jogadores, utilizando sockets TCP para comunicação em rede. O objetivo é explorar a comunicação cliente-servidor, sincronização de turnos e troca de mensagens em tempo real para criar uma experiência interativa de combate.

## Estrutura do Projeto

O projeto é dividido em duas partes principais:

- **Servidor (server.cpp)**: Gerencia a conexão entre os jogadores, processa os comandos de combate e mantém o estado do jogo sincronizado.
- **Cliente (client.cpp)**: Permite que os jogadores se conectem ao servidor, enviem comandos de combate e recebam atualizações sobre o estado do jogo.

## Funcionalidades Principais

- **Conexão via Sockets**: Implementação de uma arquitetura cliente-servidor usando sockets TCP para comunicação em rede local.
- **Sistema de Combate**: Os jogadores podem escolher suas ações de combate (diferentes ataques e habilidades) e o servidor processa essas ações em turnos.
- **Sincronização de Estados**: O servidor mantém o estado do jogo sincronizado e envia atualizações após cada turno, garantindo uma experiência de combate justa e sincronizada.
- **Reconhecimento de Desconexão**: O servidor detecta quando um jogador se desconecta e declara o oponente como vencedor automaticamente.

## Tecnologias Utilizadas

- **Linguagem**: C++
- **Biblioteca de Redes**: Sockets TCP
- **Interface Gráfica**: GTK 

## Pré-requisitos

- **Compilador C++** (g++ recomendado)
- **Biblioteca GtK** Para instalar no Ubuntu: `sudo apt-get install libgtk-3-dev`
- **Sistema Operacional**: Linux, macOS ou Windows com suporte a sockets.

## Como Compilar e Executar

### Compilando o Servidor e o Cliente

1. No diretório do projeto, compile o servidor e o cliente usando:
   ```bash
   make
    ```

2. Para executar o programa (servidor e cliente), use o comando:

    ```bash
    make run
    ```

3. Alternativamente, você pode compilar e executar apenas o servidor ou o cliente:

    ```bash
    make server    # Compila o servidor
    make client    # Compila o cliente
    ./server       # Executa o servidor manualmente
    ./client       # Executa o cliente manualmente
    ```


### Uso

1. Servidor: Inicie o servidor primeiro. Ele aguardará a conexão de dois jogadores.

2. Clientes: Em duas instâncias diferentes (ou em duas máquinas), execute o cliente e conecte-se ao servidor. Ambos os jogadores devem se conectar ao servidor para que o combate possa começar.

3. Escolha de Classe: Cada jogador escolhe sua classe (Guerreiro, Ladino, Mago, Arqueiro ou Paladino) e aguarda o outro jogador fazer sua escolha.

4. Combate: Os jogadores se alternam em turnos. A cada turno, cada jogador escolhe um ataque ou habilidade. O servidor processa os comandos e envia atualizações sobre o estado de HP (vida) dos jogadores após cada ação.

5. Fim de Jogo: Quando o HP de um dos jogadores chega a zero, o servidor envia uma mensagem de vitória para o vencedor e uma mensagem de derrota para o jogador derrotado. O jogo termina após a confirmação de ambos os jogadores. E o servidor volta para o estado inicial, esperando conexões de mais jogadores.


### Estrutura do Código


O código está dividido nas seguintes seções principais:

* Funções de Configuração e Limpeza: Para inicializar e fechar o servidor de forma adequada.

* Classes de Jogadores: Classes derivadas (Guerreiro, Ladino, Mago, Arqueiro, Paladino) que representam os diferentes tipos de jogadores, cada um com habilidades únicas.

* Gerenciamento de Threads: Para gerenciar as conexões simultâneas de dois jogadores, cada um rodando em sua própria thread.

* Sincronização de Turnos: Controle de turnos com pthread_mutex e pthread_cond para garantir que ambos os jogadores joguem de forma sincronizada.

* Desconexão e Tratamento de Erros: O servidor detecta e lida com desconexões, declarando o oponente como vencedor se um jogador sair.

<br>
<br>

## Classes de Personagens

Cada classe no sistema de combate multiplayer possui habilidades e características únicas que permitem diferentes estilos de jogo. Abaixo estão as descrições detalhadas das classes disponíveis e suas habilidades:

### 1. Guerreiro (Warrior)
- **Vida (HP)**: 150
- **Ataque**: 10
- **Velocidade**: 5

#### Habilidades:
- **Sword Attack**: Causa 10 de dano ao oponente.
- **Shield Bash**: Causa 5 de dano ao oponente.
- **Rally**: Restaura 20 de HP ao Guerreiro. Não permite ultrapassar o limite de HP máximo de 150.

#### Descrição:
O Guerreiro é uma classe equilibrada, com uma boa quantidade de HP e habilidades que permitem tanto causar dano como restaurar vida. Ideal para jogadores que preferem uma abordagem de combate direto.

---

### 2. Ladino (Rogue)
- **Vida (HP)**: 100
- **Ataque**: 10
- **Velocidade**: 10
- **Estado Especial**: Stealth (Furtivo)

#### Habilidades:
- **Quick Attack**: Causa 10 de dano ao oponente.
- **Stealth**: Ativa o estado de furtividade, permitindo que o próximo ataque seja evitado.
- **Backstab**: Causa 50 de dano ao oponente, mas o Ladino perde 20 de HP no processo.

#### Descrição:
O Ladino é ágil e possui habilidades de alto risco e alta recompensa. Seu estado de furtividade permite esquivar de um ataque, tornando-o um excelente personagem para jogadores que gostam de estratégias evasivas e ofensivas.

---

### 3. Mago (Mage)
- **Vida (HP)**: 80
- **Ataque**: 10
- **Velocidade**: 7
- **Estado Especial**: Arcane Shield (Escudo Arcano)

#### Habilidades:
- **Fireball**: Causa 30 de dano ao oponente.
- **Frost Nova**: Congela o oponente, reduzindo sua velocidade em 2.
- **Arcane Shield**: Ativa um escudo que absorve metade do dano no próximo turno.

#### Descrição:
O Mago é uma classe de alto dano, porém com menos HP. Suas habilidades focam em causar dano e controle de oponentes, além de ter uma defesa temporária com o Escudo Arcano. É ideal para jogadores que gostam de atacar à distância e controlar o campo de batalha.

---

### 4. Arqueiro (Archer)
- **Vida (HP)**: 100
- **Ataque**: 15
- **Velocidade**: 9
- **Estado Especial**: Eagle Eye (Olho de Águia)

#### Habilidades:
- **Arrow Shot**: Causa 20 de dano ao oponente.
- **Multi-Shot**: Tenta acertar o oponente com múltiplas flechas, causando 15 de dano. Há 50% de chance de errar, exceto se Eagle Eye estiver ativo.
- **Eagle Eye**: Garante que o próximo Multi-Shot não errará.

#### Descrição:
O Arqueiro é uma classe versátil com dano médio e habilidades de precisão. Seu estado Eagle Eye permite garantir a eficácia de ataques em sequência. É ideal para jogadores que preferem atacar de forma consistente e rápida.

---

### 5. Paladino (Paladin)
- **Vida (HP)**: 120
- **Ataque**: 20
- **Velocidade**: 5
- **Estado Especial**: Divine Shield (Escudo Divino)

#### Habilidades:
- **Holy Strike**: Causa 25 de dano ao oponente.
- **Divine Shield**: Absorve completamente o próximo ataque recebido.
- **Lay on Hands**: Cura 30 de HP, limitando o HP máximo em 120. Reduz a velocidade em 2 permanentemente.

#### Descrição:
O Paladino é uma classe com alta capacidade de sobrevivência. Suas habilidades incluem cura e defesa, tornando-o um personagem ideal para jogadores que querem resistir por mais tempo no combate enquanto causam dano significativo.

<br>
<br>
Desenvolvido por Jean Cassiano como parte de um projeto de Redes de Computadores para explorar a comunicação em rede usando sockets e C++.
