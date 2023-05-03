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
    printf("MENU\n");
    printf("Tiene las siguientes opciones:\n");
    printf("1. Enviar mensaje de todos los usuarios\n");
    printf("2. Enviar mensaje directo\n");
    printf("3. Cambiar Status\n");
    printf("4. Listar usuarios conectados\n");
    printf("5. Desplegar info de un usuario\n");
    printf("6. Ayuda\n");
    printf("7. Salir del Chat\n");
}

void helpMessage(){
    printf("Para enviar mensajes puedes utilizar las opciones 1 y 2\n");
    printf("Para las dos opciones debes escribir el mensaje que desees enviar\n");
    printf("Para la opcion 2 debes de indicar el nombre de usuario al que deseas\nenviar el mensaje.\n");
    printf("Puedes cambiar tu status dentro del chat con la opcion 3\n");
    printf("Puedes ver la lista de los usuarios con las opciones 4 y 5\n");
    printf("Para salir del chat puedes utilizar la opcion 7\n");
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
                if(server_response -> response_status_code == 200){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            case 2:
                if(server_response -> response_status_code == 200){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            case 3:
                if(server_response -> response_status_code == 200){
                    ChatSistOS__Message *message_received = server_response -> message;
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender, "ALL", message_received -> message_content);
                }
                break;
            case 4:
                if(server_response -> response_status_code == 200){
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
    int port_server = atoi(argv[3]);
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

    // int user_socket = socket(AF_INET, SOCK_STREAM, 0);

    ChatSistOS__NewUser registration_user = CHAT_SIST_OS__NEW_USER__INIT;
    registration_user.username = username;
    registration_user.ip = "127.0.0.1";

    ChatSistOS__UserOption user_registered_option = CHAT_SIST_OS__USER_OPTION__INIT;
    user_registered_option.op = user_option;
    user_registered_option.createuser = &registration_user;

    size_t serialized_registatrion = chat_sist_os__user_option__get_packed_size(&user_registered_option);
    uint8_t *registration_buffer = malloc(serialized_registatrion);
    chat_sist_os__user_option__pack(&user_registered_option, registration_buffer);

    if(send(sock, registration_buffer, serialized_registatrion, 0) < 0){
        perror("MESSAGE ERROR");
        exit(1);
    }

    free(registration_buffer);

    uint8_t buffer_recv[BUFFER_SIZE];
    ssize_t size_recv= recv(sock, buffer_recv, sizeof(buffer_recv), 0);
    if(size_recv < 0){
        perror("NO RESPONSE RECEIVED");
        exit(1);
    }

    ChatSistOS__Answer *answer = chat_sist_os__answer__unpack(NULL, size_recv, buffer_recv);

    if(answer -> response_status_code == 200){
        printf("\n Message from: %s to: %s: %s", "Server", username, answer -> response_message);
    }

    chat_sist_os__answer__free_unpacked(answer, NULL);

    pthread_t user_thread;
    if(pthread_create(&user_thread, NULL, serverResponse, (void*)&sock) < 0){
        perror("THREAD ERROR");
        exit(1);
    }

    while(user_option != 7){
        menuMessage();
        scanf("%d", &user_option);
    }

    return 0;
}
