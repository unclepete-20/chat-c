#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include "chat.pb-c.h"


#define BACKLOG 10
#define BUFFER_SIZE 1024

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    char username[100];
    char ip[100];
    int socketFD;
    int status;
    time_t last_active;
} User;

// Constantes de usuario
#define MAX_USERS 100
User userList[MAX_USERS];
int numUsers = 0;


// Permite ingresar un nuevo usuario al servicio
void addUser(char *username, char *ip, int socketFD, int status)
{
    if (numUsers >= MAX_USERS)
    {
        printf("SERVER AT FULL CAPACITY. UNABLE TO ADD NEW USERS.\n");
        return;
    }
    User newUser;
    strcpy(newUser.username, username);
    strcpy(newUser.ip, ip);
    newUser.socketFD = socketFD;
    newUser.status = status;
    newUser.last_active = time(NULL);
    userList[numUsers] = newUser;
    numUsers++;
}

// Permite eliminar un usuario del servicio
void removeUser(char *username, char *ip, int socketFD, int status)
{
    int i, j;

    pthread_mutex_lock(&lock);

    for (i = 0; i < numUsers; i++)
    {
        User user = userList[i];
        if (strcmp(user.username, username) == 0 && strcmp(user.ip, ip) == 0 && user.socketFD == socketFD)
        {
            // Encontró el usuario, lo elimina
            for (j = i; j < numUsers - 1; j++)
            {
                userList[j] = userList[j + 1];
            }
            numUsers--;
            printf("USER ELIMINATED: %s\n", username);
            return;
        }
    }

    pthread_mutex_unlock(&lock);

    printf("USER NOT REGISTERED: %s\n", username);
}

// Verifica existencia de un usuario
int userExists(char *username)
{
    int i;

    pthread_mutex_lock(&lock);

    for (i = 0; i < numUsers; i++)
    {
        if (strcmp(userList[i].username, username) == 0)
        {
            return 1; // El usuario ya existe en la lista
        }
    }

    pthread_mutex_unlock(&lock);

    return 0; 
}

// Verifica el status del usuario segun protocolo
void* check_inactive_users(void *arg) {

    pthread_mutex_lock(&lock);

    while (1) {
        time_t current_time = time(NULL);
        printf("________________TIME-ON-SERVER-(INFO)_______________________\n");
        for (int i = 0; i < numUsers; i++) {
            double elapsed_time = difftime(current_time, userList[i].last_active);
            printf("User: %s, Elapsed Time: %.0f\n", userList[i].username, elapsed_time);
            if (elapsed_time >= 60) {
                userList[i].status = 3; 
            }
        }
        sleep(15); 
    }

    pthread_mutex_unlock(&lock);
}

//Funcion que maneja las respuestas al cliente
void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    int client_in_session = 1;

    // Recibir el registro del cliente
    uint8_t recv_buffer[BUFFER_SIZE];
    ssize_t recv_size = recv(client_socket, recv_buffer, sizeof(recv_buffer), 0);
    if (recv_size < 0)
    {
        perror("UNABLE TO RECEIVE CLIENTS MESSAGE...");
        exit(1);
    }

    // Deserializar el registro de NewUser
    Chat__UserOption *usero_registration = chat_sist_os__user_option__unpack(NULL, recv_size, recv_buffer);
    if (usero_registration == NULL)
    {
        fprintf(stderr, "UNABLE TO UNPACK SENDER'S MESSAGE\n");
        exit(1);
    }

    Chat__NewUser *chat_registration = usero_registration->createuser;

    printf("\n >> |NEW USER CONNECTED| >>> NAME: %s  >>> IP: %s\n", chat_registration->username, chat_registration->ip);

    // Informacion del Cliente asociada al thread
    User MyInfo;
    strcpy(MyInfo.username, chat_registration->username);
    strcpy(MyInfo.ip, chat_registration->ip);
    MyInfo.socketFD = client_socket;


    // Answer server
    Chat__Answer server_response_registro = CHAT_SIST_OS__ANSWER__INIT;
    if (userExists(MyInfo.username) == 0)
    {
        // Agregar usuario conectado a la lista de usuarios
        addUser(chat_registration->username, chat_registration->ip, client_socket, 1);

        server_response_registro.op = 0;
        server_response_registro.response_status_code = 400;
        server_response_registro.response_message = "SUCCESSFULLY REGISTERED";
        // server_response.message = &response;

        // Serializar la respuesta en un buffer
        size_t serialized_size_servidor_registro = chat_sist_os__answer__get_packed_size(&server_response_registro);
        uint8_t *server_buffer_registro = malloc(serialized_size_servidor_registro);
        chat_sist_os__answer__pack(&server_response_registro, server_buffer_registro);

        // Enviar el buffer de respuesta a través del socket
        if (send(MyInfo.socketFD, server_buffer_registro, serialized_size_servidor_registro, 0) < 0)
        {
            perror("UNABLE TO SEND RESPONSE");
            exit(1);
        }

        // Liberar los buffers y el mensaje
        free(server_buffer_registro);
    }
    else
    {

        server_response_registro.op = 0;
        server_response_registro.response_status_code = 200;
        server_response_registro.response_message = "USER ALREADY REGISTERED";

        // Serializar la respuesta en un buffer
        size_t serialized_size_servidor_registro = chat_sist_os__answer__get_packed_size(&server_response_registro);
        uint8_t *server_buffer_registro = malloc(serialized_size_servidor_registro);
        chat_sist_os__answer__pack(&server_response_registro, server_buffer_registro);

        // Enviar el buffer de respuesta a través del socket
        if (send(MyInfo.socketFD, server_buffer_registro, serialized_size_servidor_registro, 0) < 0)
        {
            perror("UNABLE TO SEND RESPONSE");
            exit(1);
        }

        // Liberar los buffers y el mensaje
        free(server_buffer_registro);
    }

    chat_sist_os__user_option__free_unpacked(usero_registration, NULL);

    //Escuchando useroptions recibidas
    printf("\n\n OPTION'S MENU ([%s])\n", MyInfo.username);
    while (1)
    {
        printf("\n");

        uint8_t recv_buffer_opcion[BUFFER_SIZE];
        ssize_t recv_size_opcion = recv(client_socket, recv_buffer_opcion, sizeof(recv_buffer_opcion), 0);
        if (recv_size_opcion < 0)
        {
            perror("UNABLE TO RECEIVE CLIENT'S MESSAGE");
            exit(1);
        }
        if (recv_size_opcion == 0)
        {
            perror("CLIENT DISCONNECTED");
            goto exit_chat;
        }
        // Deserializar la opcion elegida del cliente
        Chat__UserOption *client_option = chat_sist_os__user_option__unpack(NULL, recv_size_opcion, recv_buffer_opcion);
        if (client_option == NULL)
        {
            fprintf(stderr, "UNABLE TO UNPACK CONTENT\n");
            exit(1);
        }

        int selected_option = client_option->op;

        printf("[%s] CHOSE ==> [%d]", MyInfo.username, selected_option);

        switch (selected_option)
        {
        case 1:
            printf("\n\n");
            Chat__Message *received_message = client_option->message;
            
            pthread_mutex_lock(&lock);

            // Se recorre la lista de usuarios
            for (int i = 0; i < numUsers; i++)
            {
                if (strcmp(userList[i].username, MyInfo.username) == 0)
                {
                    // revisar si el usuario esta inactivo en este caso activarlo como activo
                    if (userList[i].status == 3){
                        userList[i].status = 1;
                    }
                    // Si el usuario se llama Gabriel, omitirlo y continuar con el siguiente
                    userList[i].last_active = time(NULL);
                    continue;
                }

            pthread_mutex_unlock(&lock);

                Chat__Answer server_response = CHAT_SIST_OS__ANSWER__INIT;
                server_response.op = 1;
                server_response.response_status_code = 400;
                server_response.message = received_message;

                // Serializar la respuesta en un buffer
                size_t serialized_size_servidor = chat_sist_os__answer__get_packed_size(&server_response);
                uint8_t *server_buffer = malloc(serialized_size_servidor);
                chat_sist_os__answer__pack(&server_response, server_buffer);

                // Enviar el buffer de respuesta a través del socket
                if (send(userList[i].socketFD, server_buffer, serialized_size_servidor, 0) < 0)
                {
                    perror("UNABLE TO SEND RESPONSE");
                    exit(1);
                }

                free(server_buffer);
            }

            break;
        case 2:

            printf("\n\n");
            Chat__Message *received_message_directo = client_option->message;

            // Recorrer la lista de usuarios
            int enviar_mensaje = 0;
            int indice_usuario = 0;

            pthread_mutex_lock(&lock);

            for (int i = 0; i < numUsers; i++)
            {
                if (strcmp(userList[i].username, received_message_directo->message_destination) == 0)
                {
                    // revisar si el usuario esta inactivo en este caso activarlo como activo
                    if (userList[i].status == 3){
                        userList[i].status = 1;
                    }
                    userList[i].last_active = time(NULL);
                    enviar_mensaje = 1;
                    indice_usuario = i;
                }
            }

            pthread_mutex_unlock(&lock);

            if (enviar_mensaje == 1)
            {
                // Si el usuario se encuentra
                Chat__Answer server_response = CHAT_SIST_OS__ANSWER__INIT;
                server_response.op = 2;
                server_response.response_status_code = 400;
                server_response.message = received_message_directo;

                // Serializar la respuesta en un buffer
                size_t serialized_size_servidor = chat_sist_os__answer__get_packed_size(&server_response);
                uint8_t *server_buffer = malloc(serialized_size_servidor);
                chat_sist_os__answer__pack(&server_response, server_buffer);

                // Enviar el buffer de respuesta a través del socket
                if (send(userList[indice_usuario].socketFD, server_buffer, serialized_size_servidor, 0) < 0)
                {
                    perror("UNABLE TO SEND RESPONSE");
                    exit(1);
                }

                // Liberar los buffers y el mensaje
                free(server_buffer);
            }
            else
            {

                // Si el usuario no se encuentra
                Chat__Answer server_response = CHAT_SIST_OS__ANSWER__INIT;
                server_response.op = 2;
                server_response.response_status_code = 200;
                server_response.response_message = "USER NOT FOUND...";
                server_response.message = received_message_directo;

                // Serializar la respuesta en un buffer
                size_t serialized_size_servidor = chat_sist_os__answer__get_packed_size(&server_response);
                uint8_t *server_buffer = malloc(serialized_size_servidor);
                chat_sist_os__answer__pack(&server_response, server_buffer);

                // Enviar el buffer de respuesta a través del socket
                if (send(MyInfo.socketFD, server_buffer, serialized_size_servidor, 0) < 0)
                {
                    perror("UNABLE TO SEND RESPONSE");
                    exit(1);
                }

                free(server_buffer);
            }

            break;
        case 3:
                printf("\n\n");
                Chat__Status *status_verify = client_option->status;
                // Se recorre la lista de usuarios

                pthread_mutex_lock(&lock);

                for (int i = 0; i < numUsers; i++) {
                    if (strcmp(userList[i].username, MyInfo.username) == 0) {
                        // revisar si el usuario esta inactivo en este caso activarlo como activo
                        if (userList[i].status == 3){
                            userList[i].status = 1;
                        }
                        // Si el usuario se llama Gabriel, cambiar su estado
                        userList[i].last_active = time(NULL);
                        userList[i].status = status_verify->user_state;

                        Chat__Answer server_response          = CHAT_SIST_OS__ANSWER__INIT;
                        server_response.op   =   3 ;
                        server_response.response_status_code = 400;
                        server_response.response_message = "\nSTATUS CHANGED ON-PREMISE";
                    }
                }
                
                pthread_mutex_unlock(&lock);

            break;
        case 4:
            printf("\n\n");

            //Usuarios online
            Chat__UsersOnline connected_clients = CHAT_SIST_OS__USERS_ONLINE__INIT;
            connected_clients.n_users = numUsers;
            connected_clients.users   = malloc(sizeof(Chat__User *) * numUsers);

            pthread_mutex_lock(&lock);

            for (int i = 0; i < numUsers; i++)
            {
                if (strcmp(userList[i].username, MyInfo.username) == 0) {
                    // revisar si el usuario esta inactivo en este caso activarlo como activo
                    if (userList[i].status == 3){
                        userList[i].status = 1;
                    }
                    userList[i].last_active = time(NULL);
                }
                Chat__User *new_user = malloc(sizeof(Chat__User));
                chat_sist_os__user__init(new_user);
                new_user->user_name = userList[i].username;
                new_user->user_state = userList[i].status;
                new_user->user_ip = userList[i].ip;

                connected_clients.users[i] = new_user;
            }
            
            pthread_mutex_unlock(&lock);

            // Answer del servidor
            Chat__Answer server_response = CHAT_SIST_OS__ANSWER__INIT;
            server_response.op = 4;
            server_response.response_status_code = 400;
            server_response.response_message = "USERS CURRENTLY CONNECTED:";
            server_response.users_online = &connected_clients;

            // Serializar la respuesta en un buffer
            size_t serialized_size_servidor = chat_sist_os__answer__get_packed_size(&server_response);
            uint8_t *server_buffer = malloc(serialized_size_servidor);
            chat_sist_os__answer__pack(&server_response, server_buffer);


            if (send(MyInfo.socketFD, server_buffer, serialized_size_servidor, 0) < 0)
            {
                perror("UNABLE TO SEND RESPONSE");
                exit(1);
            }
            free(server_buffer);
            break;
        case 5:
            {
                printf("\n\n");
                int user_found = 0;
                // Se muestran los usuarios disponibles en linea
                Chat__UsersOnline connected_clients = CHAT_SIST_OS__USERS_ONLINE__INIT;
                connected_clients.n_users = numUsers;
                connected_clients.users   = malloc(sizeof(Chat__User *) * numUsers);
                
                pthread_mutex_lock(&lock);

                for (int i = 0; i < numUsers; i++)
                {
                    if (strcmp(userList[i].username, MyInfo.username) == 0) {
                        if (userList[i].status == 3){
                            userList[i].status = 1;
                        }
                        userList[i].last_active = time(NULL);
                    }
                    Chat__User *new_user = malloc(sizeof(Chat__User));
                    chat_sist_os__user__init(new_user);
                    Chat__User *empty = malloc(sizeof(Chat__User));
                    chat_sist_os__user__init(empty);
                    empty->user_name = "NULL";
                    new_user->user_name = userList[i].username;
                    new_user->user_state = userList[i].status;
                    new_user->user_ip = userList[i].ip;
                    if(strcmp(userList[i].username, client_option->userlist->user_name) == 0){
                        connected_clients.users[i] = new_user;
                        user_found = 1;
                    }else{
                        connected_clients.users[i] = empty;
                    }
                    
                }

                pthread_mutex_unlock(&lock);

                // Respuesta recibida en el servidor
                Chat__Answer server_response = CHAT_SIST_OS__ANSWER__INIT;
                server_response.op = 5;
                if (user_found == 1)
                {
                    server_response.response_status_code = 400;
                }else{
                    server_response.response_status_code = 200;
                }
                

                server_response.response_message = "USERS CURRENTLY CONNECTED";
                server_response.users_online = &connected_clients;

                // Serializar la respuesta en un buffer
                size_t serialized_size_servidor = chat_sist_os__answer__get_packed_size(&server_response);
                uint8_t *server_buffer = malloc(serialized_size_servidor);
                chat_sist_os__answer__pack(&server_response, server_buffer);

                // Enviar el buffer de respuesta a través del socket
                if (send(MyInfo.socketFD, server_buffer, serialized_size_servidor, 0) < 0)
                {
                    perror("UNABLE TO SEND RESPONSE");
                    exit(1);
                }
                free(server_buffer);
            break;
            }
        case 6:

            pthread_mutex_lock(&lock);
            for (int i = 0; i < numUsers; i++){
                if (strcmp(userList[i].username, MyInfo.username) == 0) {
                    if (userList[i].status == 3){
                        userList[i].status = 1;
                    }
                    userList[i].last_active = time(NULL);
                }
            }
            pthread_mutex_unlock(&lock);

            break;
        case 7:
            chat_sist_os__user_option__free_unpacked(client_option, NULL);
            goto exit_chat;
        default:
            fprintf(stderr, "INVALID SELECTION...: %d\n", selected_option);
            break;
        }

        // Libera el desempaquetamiento
        chat_sist_os__user_option__free_unpacked(client_option, NULL);
    }

    exit_chat:
    removeUser(MyInfo.username, MyInfo.ip, MyInfo.socketFD, MyInfo.status);

    printf("\n\n ********** CONNECTED USERS **********\n");

    for (int i = 0; i < numUsers; i++)
    {
        printf("USER INFO #%d:\n", i + 1);
        printf("USERNAME: %s\n", userList[i].username);
        printf("STATUS: %d\n", userList[i].status);
        printf("IP: %s\n", userList[i].ip);
        printf("SOCKET ID: %d\n", userList[i].socketFD);
        printf("\n");
    }
    close(client_socket);
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("USE: %s <PORT>\n", argv[0]);
        exit(1);
    }

    int server_port = atoi(argv[1]);

    // Crear el socket del servidor

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("SOCKE UNABLE TO LISTEN");
        exit(1);
    }

    // Permitir la reutilización de la dirección y puerto del servidor

    int option = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0)
    {
        perror("SOCKET INVALID CONFIG DETECTED");
        exit(1);
    }

    // Configurar la dirección y puerto del servidor

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(server_port);

    // Enlazar el socket del servidor a la dirección y puerto especificados
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("SOCKET UNABLE TO LISTEN TO SERVER");
        exit(1);
    }

    // Escuchar conexiones entrantes
    if (listen(server_socket, BACKLOG) < 0)
    {
        perror("SOCKET UNABLE TO MANAGE INCOMING CONNECTIONS");
        exit(1);
    }

    printf("SERVER LISTENING ON PORT: %d\n", server_port);


    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        perror("Error initializing mutex");
        exit(1);
    }

    while (1)
    {
        // Esperar a que llegue una conexión
        struct sockaddr_in client_address;
        socklen_t client_address_length = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);
        if (client_socket < 0)
        {
            perror("UNABLE TO PROCESS CLIENT REQUEST");
            exit(1);
        }

        printf("\nCLIENT CONNECTED FROM %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Se crean hilos para manejar simulataneamente la conexión de clientes en el servidor
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client, (void *)&client_socket) < 0)
        {
            perror("UNABLE TO INITIALIZE THREAD FOR THE USER");
            exit(1);
        }

        pthread_t inactive_users_thread;
        if (pthread_create(&inactive_users_thread, NULL, check_inactive_users, NULL)) {
            perror("TIME THREAD NOT CREATED");
            exit(1);
        }
    }

    pthread_mutex_destroy(&lock);

    return 0;
}