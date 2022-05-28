#include "spell_checker.h"

/**
 * @author Steve Tolvaj
 * CIS 3207 001
 * Project 3: Networked Spell Checker
 * 11/12/2021
 * 
 * The client.c file contains a simple client program to connect to the server that defaults to ip 127.0.0.1 and port
 * 8889 if no command line arguments are used else both ip and port must be specified. 
 * i.e. either ./client or ./client <ip><port> must be used.
**/

int main(int argc, char const *argv[])
{
    int client_socket;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address;

    if (client_socket == -1) {
        puts("Could not create socket");
        return 1;
    }

    const char *ip_address;
    char default_ip[] = "127.0.0.1"; 
    int port_num = 8889;

    if(argc == 3) {
        ip_address = argv[1];
        port_num = atoi(argv[2]);
    } else {
        ip_address = default_ip;
    }

  

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port_num);  // Choose port that may not be in use by OS.
    server_address.sin_addr.s_addr = inet_addr(ip_address);           // inet_addr("127.0.0.1");   

    int connect_status = connect(client_socket, (struct sockaddr *)&server_address , sizeof(server_address));
    // Connect to server.
    if (connect_status == -1) {
        puts("connect error");
        return 1;
    }
    
    size_t buff_size = 0;
    char *buff = NULL;

    // Buffer for server responses;
    char server_response[256];
    recv(client_socket, &server_response, sizeof(server_response), 0);
    printf("%s", server_response);
    while (1)
    {
        getline(&buff, &buff_size, stdin);
        send(client_socket, buff, buff_size, 0);
        read(client_socket, &server_response, MAX_WORD_SIZE);
        printf("%s", server_response);
    }
  
    free(buff);
    close(client_socket);

    return 0;
}

