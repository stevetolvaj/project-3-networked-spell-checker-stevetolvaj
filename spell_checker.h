#ifndef SPELL_CHECKER_H
#define SPELL_CHECKER_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
// For network
#include <sys/types.h>
#include <sys/socket.h>
#include<arpa/inet.h>   // for inet function
#include <netinet/in.h>

#define MAX_LOG 2056 // For max buffer of logger.
#define MAX_DICTIONARY 200000 // Maximum words stored in dictionary.
#define MAX_SOCKET_BACKLOG 1024
#define MAX_WORD_SIZE 256
typedef struct client_socket {
    int socket_id;
    int priority;
} client_socket;
// Circular buffer variables for socket descriptors.
typedef struct socket_buffer {
    client_socket *client_socket;
    int fill_ptr;
    int use_ptr;
    int arr_count;
    pthread_mutex_t lock;
    pthread_cond_t empty_cond;
    pthread_cond_t fill_cond;
}socket_buffer;

typedef struct log_buffer {
    char **array;
    int fill_ptr;
    int use_ptr;
    int arr_count;
    pthread_mutex_t lock;
    pthread_cond_t empty_cond;
    pthread_cond_t fill_cond;
}log_buffer;

#endif