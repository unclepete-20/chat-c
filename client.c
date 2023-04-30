#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.pb-c.h"

#define SERVER_IP "127.0.0.1"
#define PORT 9001
#define BUFFER_SIZE 1024
#define MAX_INPUT_SIZE 256

int sock = 0;

void *receive_messages(void *arg) {
    uint8_t buffer[BUFFER_SIZE];

    while (1) {
        ssize_t len = recv(sock, buffer, BUFFER_SIZE, 0);
        if (len > 0) {
            Chat__Answer *answer = chat__answer__unpack(NULL, len, buffer);
            if (answer != NULL && answer->message != NULL) {
                printf("Received from %s: %s\n", answer->message->message_sender, answer->message->message_content);
                chat__answer__free_unpacked(answer, NULL);
            }
        }
    }

    return NULL;
}

int main(int argc, char const *argv[]) {
    struct sockaddr_in server_address;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, '0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    pthread_t receive_thread;
    pthread_create(&receive_thread, NULL, receive_messages, NULL);

    while (1) {
        char input[MAX_INPUT_SIZE];
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;
        }

        char recipient[64];
        printf("Enter recipient: ");
        fgets(recipient, 64, stdin);
        recipient[strcspn(recipient, "\n")] = 0;

        Chat__Message message = CHAT__MESSAGE__INIT;
        message.message_private = 1;
        message.message_destination = recipient;
        message.message_content = input;
        message.message_sender = "your_username"; // Replace this with the actual sender's username

        Chat__UserOption user_option = CHAT__USER_OPTION__INIT;
        user_option.op = 5;
        user_option.message = &message;

        uint8_t buffer[BUFFER_SIZE];
        size_t message_len = chat__user_option__get_packed_size(&user_option);
        chat__user_option__pack(&user_option, buffer);

        send(sock, buffer, message_len, 0);
    }

    close(sock);

    return 0;
}
