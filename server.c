#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.pb-c.h"

#define PORT 9001
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int sockfd;
    char username[64];
    int status;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

void broadcast_message(Chat__Message *message) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].status != 3 && strcmp(clients[i].username, message->message_sender) != 0) {
            Chat__Answer answer = CHAT__ANSWER__INIT;
            answer.op = 5;
            answer.response_status_code = 200;
            answer.message = message;

            uint8_t buffer[BUFFER_SIZE];
            size_t message_len = chat__answer__get_packed_size(&answer);
            chat__answer__pack(&answer, buffer);

            send(clients[i].sockfd, buffer, message_len, 0);
        }
    }
}

void *handle_client(void *arg) {
    Client *client = (Client *)arg;
    int sockfd = client->sockfd;
    uint8_t buffer[BUFFER_SIZE];

    while (1) {
        ssize_t len = recv(sockfd, buffer, BUFFER_SIZE, 0);

        if (len <= 0) {
            // Client disconnected
            client->status = 3;
            break;
        }

        Chat__UserOption *user_option = chat__user_option__unpack(NULL, len, buffer);
        if (user_option == NULL) {
            continue;
        }

        if (user_option->op == 5 && user_option->message != NULL) {
            // Forward the message to other clients
            broadcast_message(user_option->message);
        }

        chat__user_option__free_unpacked(user_option, NULL);
    }

    close(sockfd);
    return NULL;
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if (client_count < MAX_CLIENTS) {
            Client *client = &clients[client_count++];
            client->sockfd = new_socket;
            client->status = 1;

            pthread_t client_thread;
            pthread_create(&client_thread, NULL, handle_client, (void *)client);
            pthread_detach(client_thread);

            printf("New client connected (fd: %d)\n", new_socket);
        } else {
            close(new_socket);
            printf("Client limit reached. Connection refused.\n");
        }
    }

    return 0;
}
