#include<stdio.h>
#include<stdlib.h>
#include<time.h>



typedef struct client_socket{
    int socket_id;
    int priority;
}client_socket;

// Circular buffer variables for socket descriptors.
typedef struct socket_buffer {
    client_socket *client;
    int fill_ptr;
    int use_ptr;
    int arr_count;

}socket_buffer;

socket_buffer _socket_buffer;
int max_conn_buffer_size = 10;

void put_socket(int socket, int priority) {

    _socket_buffer.client[_socket_buffer.fill_ptr].socket_id = socket;
    _socket_buffer.client[_socket_buffer.fill_ptr].priority = priority;

    //printf("%d\n", array[fill_ptr]);
    _socket_buffer.fill_ptr = (_socket_buffer.fill_ptr + 1) % max_conn_buffer_size;
    _socket_buffer.arr_count++;

}

client_socket get_socket() {
    client_socket temp = _socket_buffer.client[_socket_buffer.use_ptr];
    _socket_buffer.use_ptr = (_socket_buffer.use_ptr + 1) % max_conn_buffer_size;
    _socket_buffer.arr_count--;
    return temp;
}

int compare(const void *p, const void *q) 
   { 
       int l = ((client_socket*)p)->priority;
       int r = ((client_socket*)q)->priority;
      return (l-r);
} 

client_socket sort() {
    socket_buffer temp;
    for (size_t i = 0; i < max_conn_buffer_size; i++)
    {
        temp.client[i] = get_socket();
    }

    qsort(temp.client, max_conn_buffer_size, sizeof(client_socket), compare);

    for (size_t i = 0; i < max_conn_buffer_size; i++)
    {
        put_socket(temp.client[i].socket_id, temp.client[i].priority);
    }
    
    
}

int main(int argc, char const *argv[])
{
    // Start with different seed each time program starts.
    srand(time(NULL));

    _socket_buffer.fill_ptr = 0;
    _socket_buffer.use_ptr = 0;
    _socket_buffer.arr_count = 0;
    _socket_buffer.client = malloc(sizeof(client_socket) * max_conn_buffer_size);
    if((_socket_buffer.client) == NULL) {
        perror("Error: Unable to allocate memory for socket buffer!\n");
    }

    int random_num;

    for (size_t i = 0; i < max_conn_buffer_size; i++)
    {
        random_num = (rand() % 10) + 1;
        put_socket(i, random_num);
    }
    get_socket();
    get_socket();

    put_socket(4,3);
    put_socket(4,3);

    // qsort((void*)_socket_buffer.client, max_conn_buffer_size, sizeof(client_socket), compare);
    //printf("Lowest is: %d\n", get_lowest_priority().socket_id);
    sort();
    client_socket temp;
    for (size_t i = max_conn_buffer_size; i > 0; i--)
    {
        temp = get_socket();
       printf("Socket %d Priority %d\n", temp.socket_id, temp.priority);
    }
    
    free(_socket_buffer.client);

    return 0;
}
