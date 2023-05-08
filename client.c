// Proyecto Chat
// Sistemas Operativos
// Oscar Fernando Lopez Barrios
// Pedro Pablo Arriola Jimenez

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

#define BUFFER_SIZE 1024
#define MAX_INPUT_SIZE 256

int sock = 0;

void menuMessage(){
    printf("\nMENU\n");
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
    char* response = malloc(sizeof(char) * 40);
    switch(status_value){
        case 1:{
            strcpy(response, "ACTIVE");
            break;
        }
        case 2:{
            strcpy(response, "INACTIVE");
            break;
        }
        case 3:{
            strcpy(response, "BUSY");
            break;
        }
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
            perror("RESPONSE ERROR");
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
                    printf("\n Message from: %s to: %s: %s", message_received -> message_sender,  message_received -> message_destination, message_received -> message_content);
                }
                break;
            case 3:
                break;
            case 4:
                printf("\n Users Connected \n");

                ChatSistOS__UsersOnline *users_conected = server_response -> users_online;

                for (int i = 0; i < users_conected -> n_users; i++){
                    ChatSistOS__User *user = users_conected -> users[i];
                    char status[40];
                    strcpy(status, userStatus(user -> user_state));
                    printf("\n User: %s - Status: %s \n", user -> user_name, status);
                }
                break;

            case 5:{

                if (server_response -> response_status_code == 200){
                    ChatSistOS__UsersOnline *users_conected = server_response -> users_online;
                    for (int i = 0; i < users_conected -> n_users; i++){
                        ChatSistOS__User *user = users_conected -> users[i];
                        if (strcmp("Empty", user -> user_name) != 0)
                        {
                            printf("\n User Information: %s <<\n", user -> user_name);
                            char status[40];
                            strcpy(status, userStatus(user->user_state));
                            printf("\n User: %s - Status: %s \n", user -> user_name, status);
                        }
                        
                        
                    }
                }
                else{
                    printf("\n\n >> No se ha encontrado al usuario\n\n");
                }
                break;
            }
            default:
                break;
        }
    }
}

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
        switch (user_option){
            case 1:{
                char message_content[BUFFER_SIZE];

                printf("Write the message: ");
                scanf(" %[^\n]", message_content);
                printf("\n Message from: %s to: %s: %s", username, "ALL", message_content);

                ChatSistOS__Message user_message = CHAT_SIST_OS__MESSAGE__INIT;
                user_message.message_content = message_content;
                user_message.message_private = '0';
                user_message.message_destination = "";
                user_message.message_sender = username;

                ChatSistOS__UserOption user_option_new = CHAT_SIST_OS__USER_OPTION__INIT;
                user_option_new.op = user_option;
                user_option_new.message = &user_message;

                size_t serialized_size_option = chat_sist_os__user_option__get_packed_size(&user_option_new);
                uint8_t *buffer_option = malloc(serialized_size_option);
                chat_sist_os__user_option__pack(&user_option_new, buffer_option);
                
                if (send(sock, buffer_option, serialized_size_option, 0) < 0) {
                    perror("MESSAGE ERROR");
                    exit(1);
                }

                free(buffer_option);
                printf("\n");
                break;
            }
            case 2:{
                char message_content[BUFFER_SIZE];
                char user_destination[BUFFER_SIZE];

                printf("Write the message: ");
                scanf(" %[^\n]", message_content);
                printf("Write the user destination: ");
                scanf(" %[^\n]", user_destination);
                printf("\n Message from: %s to: %s: %s", username, user_destination, message_content);

                ChatSistOS__Message user_message = CHAT_SIST_OS__MESSAGE__INIT;
                user_message.message_content = message_content;
                user_message.message_private = '1';
                user_message.message_destination = user_destination;
                user_message.message_sender = username;

                ChatSistOS__UserOption user_option_new = CHAT_SIST_OS__USER_OPTION__INIT;
                user_option_new.op = user_option;
                user_option_new.message = &user_message;

                size_t serialized_size_option = chat_sist_os__user_option__get_packed_size(&user_option_new);
                uint8_t *buffer_option = malloc(serialized_size_option);
                chat_sist_os__user_option__pack(&user_option_new, buffer_option);
                
                if (send(sock, buffer_option, serialized_size_option, 0) < 0) {
                    perror("MESSAGE ERROR");
                    exit(1);
                }

                free(buffer_option);
                printf("\n");
                break;
            }
            case 3:{

                int option;

                printf("Choose an option:\n");
                printf("1. Active\n");
                printf("2. Inactive\n");
                printf("3. Busy\n");
                scanf(" %d", &option);

                printf("%d", option);

                ChatSistOS__Status user_status              = CHAT_SIST_OS__STATUS__INIT;
                user_status.user_name                   = username;
                user_status.user_state                  = option;
                
                ChatSistOS__UserOption user_option_new = CHAT_SIST_OS__USER_OPTION__INIT;
                user_option_new.op = user_option;
                user_option_new.status = &user_status;


                size_t serialized_size_option = chat_sist_os__user_option__get_packed_size(&user_option_new);
                uint8_t *buffer_option = malloc(serialized_size_option);
                chat_sist_os__user_option__pack(&user_option_new, buffer_option);


                if (send(sock, buffer_option, serialized_size_option, 0) < 0) {
                    perror("MESSAGE ERROR");
                    exit(1);
                }

                free(buffer_option);
                printf("\n");
                break;
            }
            case 4:{

                ChatSistOS__UserList users_list   = CHAT_SIST_OS__USER_LIST__INIT;
                users_list.list =   '1';

                ChatSistOS__UserOption user_option_new    = CHAT_SIST_OS__USER_OPTION__INIT;
                user_option_new.op                  = user_option;
                user_option_new.userlist            = &users_list;

                size_t serialized_size_option = chat_sist_os__user_option__get_packed_size(&user_option_new);
                uint8_t *buffer_option = malloc(serialized_size_option);
                chat_sist_os__user_option__pack(&user_option_new, buffer_option);


                if (send(sock, buffer_option, serialized_size_option, 0) < 0) {
                    perror("MESSAGE ERROR");
                    exit(1);
                }

                free(buffer_option);
                printf("\n");
                break;
            }
            case 5:{
                char username[BUFFER_SIZE];

                printf("Write the username: ");
                scanf("%[^\n]", username);

                printf("Information about the user: %s", username);

                ChatSistOS__UserList user_list = CHAT_SIST_OS__USER_LIST__INIT;
                user_list.list = '0';
                user_list.user_name = username;

                ChatSistOS__UserOption user_option_new = CHAT_SIST_OS__USER_OPTION__INIT;
                user_option_new.op = user_option;
                user_option_new.userlist = &user_list;

                size_t serialized_size_option = chat_sist_os__user_option__get_packed_size(&user_option_new);
                uint8_t *buffer_option = malloc(serialized_size_option);
                chat_sist_os__user_option__pack(&user_option_new, buffer_option);

                if (send(sock, buffer_option, serialized_size_option, 0) < 0) {
                    perror("MESSAGE ERROR");
                    exit(1);
                }

                free(buffer_option);
                printf("\n");
                break;

            }
            default:
                break;
        }
    }
    close(sock);
    return 0;
}
