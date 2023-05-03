#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "chat.pb-c.h"
#include <ifaddrs.h>

//#define SERVER_IP "127.0.0.1"
//#define PORT 9001
#define BUFFER_SIZE 1024
#define MAX_INPUT_SIZE 256

int sock = 0;

void menuMessage(){
    printf("MENU");
    printf("Tiene las siguientes opciones:");
    printf("1. Enviar mensaje de todos los usuarios");
    printf("2. Enviar mensaje directo");
    printf("3. Cambiar Status");
    printf("4. Listar usuarios conectados");
    printf("5. Desplegar info de un usuario");
    printf("6. Ayuda");
    printf("7. Salir del Chat");
}

void helpMessage(){
    printf("Para enviar mensajes puedes utilizar las opciones 1 y 2");
    printf("Para las dos opciones debes escribir el mensaje que desees enviar");
    printf("Para la opcion 2 debes de indicar el nombre de usuario al que deseas\nenviar el mensaje.");
    printf("Puedes cambiar tu status dentro del chat con la opcion 3");
    printf("Puedes ver la lista de los usuarios con las opciones 4 y 5");
    printf("Para salir del chat puedes utilizar la opcion 7");
}

char* userStatus(int status_value){
    char* response;
    switch(status_value){
        case 1:
            strcpy(response, "ACTIVE");
            break;
        case 2:
            strcpy(response, "INACTIVE");
            break;
        case 3:
            strcpy(response, "BUSY");
            break;
        default:
            break;
    }

    return response;
}

// char* getUserIP(){
//     struct ifaddrs *ifaddr, *ifa;
//     char* user_ip = NULL;

//     if(getifaddrs(&ifaddr) == -1);
// }

void* serverResponse(void* args){
    int socket = *(int*) args;

    while(1){

        uint8_t buffer_recv[BUFFER_SIZE];
        ssize_t size_recv = recv(socket, buffer_recv, sizeof(buffer_recv), 0);

        if(size_recv< 0){
            perror("Response ERROR");
            exit(1);
        }

        if(size_recv == 0){
            perror("Server Not Available");
            exit(1);
        }

        ChatSistOS__Answer *server_response = chat_sist_os__answer__unpack(NULL, size_recv, buffer_recv);

        int option = server_response -> op;

        switch (option){
            case 1:
                if(server_response -> response_status_code == 400){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            case 2:
                if(server_response -> response_status_code == 400){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            case 3:
                if(server_response -> response_status_code == 400){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            case 4:
                if(server_response -> response_status_code == 400){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            default:
                break;
        }
    }
}

// void *receive_messages(void *arg) {
//     uint8_t buffer[BUFFER_SIZE];

//     while (1) {
//         ssize_t len = recv(sock, buffer, BUFFER_SIZE, 0);
//         if (len > 0) {
//             ChatSistOS__Answer *answer = chat_sist_os__answer__unpack(NULL, len, buffer);
//             if (answer != NULL && answer->message != NULL) {
//                 printf("Received from %s: %s\n", answer->message->message_sender, answer->message->message_content);
//                 chat_sist_os__answer__unpack(answer, NULL);
//             }
//         }
//     }

//     return NULL;
// }

int main(int argc, char *argv[]) {
    struct sockaddr_in server_address;

    if(argc != 4){
        printf("Ingrese los argumentos necesarios para conectarse");
        printf("<user_name> <ip_servidor> <puerto_servidor>");
        exit(1);
    }

    char* username = argv[1];
    char* ip_server = argv[2];
    int* port_server = atoi(argv[3]);
    int user_option = 0;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, '0', sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_server);

    if (inet_pton(AF_INET, ip_server, &server_address.sin_addr) <= 0) {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    int user_socket = socket(AF_INET, SOCK_STREAM, 0);

    ChatSistOS__NewUser registration_user = CHAT_SIST_OS__NEW_USER__INIT;
    registration_user.username = username;
    registration_user.ip = "HOLA";

    ChatSistOS__UserOption user_registered_option = CHAT_SIST_OS__USER_OPTION__INIT;
    user_registered_option.op = user_option;
    user_registered_option.createuser = &registration_user;

    size_t serialized_registatrion = chat_sist_os__user_option__get_packed_size(&user_registered_option);
    uint8_t *registration_buffer = malloc(serialized_registatrion);
    chat_sist_os__user_option__pack(&user_registered_option, registration_buffer);

    if(send(user_socket, registration_buffer, serialized_registatrion, 0) < 0){
        perror("MESSAGE ERROR");
        exit(1);
    }

    free(registration_buffer);

    uint8_t buffer_recv[BUFFER_SIZE];
    ssize_t size_recv= recv(user_socket, buffer_recv, sizeof(buffer_recv), 0);
    if(size_recv < 0){
        perror("NO RESPONSE RECEIVED");
        exit(1);
    }

    ChatSistOS__Answer *answer = chat_sist_os__answer__unpack(NULL, size_recv, buffer_recv);

    if(answer -> response_status_code == 400){
        printf("\n Message from: %s to: %s: %s", "Server", username, answer -> response_message);
    }
    else{
        printf("SE CAGO EN TODO");
    }

    // pthread_t receive_thread;
    // pthread_create(&receive_thread, NULL, receive_messages, NULL);

    // while (1) {
    //     char input[MAX_INPUT_SIZE];
    //     if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
    //         break;
    //     }

    //     char recipient[64];
    //     printf("Enter recipient: ");
    //     fgets(recipient, 64, stdin);
    //     recipient[strcspn(recipient, "\n")] = 0;

    //     ChatSistOS__Message message = CHAT__MESSAGE__INIT;
    //     message.message_private = 1;
    //     message.message_destination = recipient;
    //     message.message_content = input;
    //     message.message_sender = username; // Replace this with the actual sender's username

    //     ChatSistOS__UserOption user_option = CHAT__USER_OPTION__INIT;
    //     user_option.op = 5;
    //     user_option.message = &message;

    //     uint8_t buffer[BUFFER_SIZE];
    //     size_t message_len = chat__user_option__get_packed_size(&user_option);
    //     chat__user_option__pack(&user_option, buffer);

    //     send(sock, buffer, message_len, 0);
    // }

    // close(sock);

    return 0;
}
