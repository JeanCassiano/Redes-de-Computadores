#include <gtk/gtk.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <pthread.h>

#define PORT 8080

int sock = 0;
GtkWidget *message_entry;
GtkWidget *chat_display;
pthread_t receive_thread;

// Estrutura para passar dados à função de atualização de GUI
struct MessageData {
    std::string message;
};

// Função para limpar e fechar corretamente os recursos
void clean_exit() {
    if (sock > 0) {
        close(sock);  // Fechar o socket antes de sair
        std::cout << "Socket closed.\n";
    }

    if (pthread_self() != receive_thread) {
        pthread_cancel(receive_thread);  // Cancela a thread de recebimento de mensagens
        pthread_join(receive_thread, NULL);  // Aguarda a thread de recebimento
    }

    gtk_main_quit();  // Encerra o loop do GTK
}

// Tratamento de sinal para interrupção
void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received.\n";
    clean_exit();
    exit(0);  // Sair do programa
}

// Função chamada na thread principal para atualizar o chat_display
gboolean update_chat_display(gpointer data) {
    MessageData *msg_data = (MessageData *)data;
    
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_display));
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(buffer, &end);

    gtk_text_buffer_insert(buffer, &end, msg_data->message.c_str(), -1);
    gtk_text_buffer_insert(buffer, &end, "\n", -1);

    delete msg_data;  // Liberar a memória alocada para a mensagem
    return FALSE;  // Retornar FALSE para garantir que o callback seja chamado apenas uma vez
}

// Função para enviar a mensagem ao servidor
void send_message(GtkWidget *widget, gpointer data) {
    const char *message = gtk_entry_get_text(GTK_ENTRY(message_entry));

    if (send(sock, message, strlen(message), 0) <= 0) {
        std::cerr << "Error sending message to server: " << strerror(errno) << std::endl;
        clean_exit();
        exit(1);
    }

    // Adicionar a mensagem enviada no display de chat (usando a função na thread principal)
    MessageData *msg_data = new MessageData();
    msg_data->message = "You: " + std::string(message);
    g_idle_add(update_chat_display, msg_data);

    gtk_entry_set_text(GTK_ENTRY(message_entry), "");  // Limpar a caixa de entrada
}

// Função para inicializar o socket e conectar ao servidor
void initialize_connection() {
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error: " << strerror(errno) << std::endl;
        exit(1);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported: " << strerror(errno) << std::endl;
        clean_exit();
        exit(1);
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed: " << strerror(errno) << std::endl;
        clean_exit();
        exit(1);
    }
}

// Função para receber mensagens do servidor (em thread separada)
void *receive_messages(void *) {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = read(sock, buffer, sizeof(buffer) - 1);
        
        if (bytesRead <= 0) {
            std::cerr << "Server disconnected or error reading: " << strerror(errno) << std::endl;
            clean_exit();
            exit(1);
        }

        buffer[bytesRead] = '\0';  // Certificar-se de que o buffer termina com '\0' para uma string válida

        // Usar g_idle_add para garantir que a interface gráfica seja atualizada na thread principal
        MessageData *msg_data = new MessageData();
        msg_data->message = "Server: " + std::string(buffer);
        g_idle_add(update_chat_display, msg_data);
    }
}

int main(int argc, char *argv[]) {
    // Captura interrupções (Ctrl+C)
    signal(SIGINT, signalHandler);

    gtk_init(&argc, &argv);  // Inicializa o GTK

    // Criando a janela principal
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Chat Client");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    // Caixa principal vertical
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Área de exibição de chat
    chat_display = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(chat_display), FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), chat_display, TRUE, TRUE, 0);

    // Entrada de mensagem
    message_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(vbox), message_entry, FALSE, FALSE, 0);

    // Botão de enviar mensagem
    GtkWidget *send_button = gtk_button_new_with_label("Send");
    gtk_box_pack_start(GTK_BOX(vbox), send_button, FALSE, FALSE, 0);

    // Conectar ao servidor
    initialize_connection();

    // Iniciar uma thread para receber mensagens
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    // Conectar o botão de enviar à função send_message
    g_signal_connect(send_button, "clicked", G_CALLBACK(send_message), NULL);

    // Conectar o fechamento da janela à função clean_exit (para garantir encerramento correto)
    g_signal_connect(window, "destroy", G_CALLBACK(clean_exit), NULL);

    gtk_widget_show_all(window);  // Mostra todos os componentes da interface

    gtk_main();  // Inicia o loop principal da GUI

    return 0;
}
