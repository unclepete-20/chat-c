#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    int socket_fd;
    struct sockaddr_in addr;
} ClientInfo;

int client_count = 0;
ClientInfo clients[MAX_CLIENTS];

void *client_handler(void *arg) {
    ClientInfo *client = (ClientInfo *)arg;
    int socket_fd = client->socket_fd;

    while (1) {
        uint8_t recv_buffer[BUFFER_SIZE];
        ssize_t bytes_received = recv(socket_fd, recv_buffer, sizeof(recv_buffer), 0);
        if (bytes_received <= 0) {
            break;
        }


        for (int i = 0; i < client_count; i++) {
            if (clients[i].socket_fd != socket_fd) {
                send(clients[i].socket_fd, recv_buffer, bytes_received, 0);
            }
        }

    }

    close(socket_fd);

    return NULL;
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while (1) {
        ClientInfo new_client;
        socklen_t addr_len = sizeof(new_client.addr);

        new_client.socket_fd = accept(server_fd, (struct sockaddr *)&new_client.addr, &addr_len);
        if (new_client.socket_fd < 0) {
            perror("accept");
            continue;
        }

        printf("New client connected: %s\n", inet_ntoa(new_client.addr.sin_addr));

        clients[client_count++] = new_client;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, client_handler, (void *)&clients[client_count - 1]);
        pthread_detach(thread_id);
    }

    close(server_fd);

    return 0;
}
