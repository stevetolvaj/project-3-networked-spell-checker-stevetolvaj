#include "spell_checker.h"

/**
 * @author Steve Tolvaj
 * CIS 3207 001
 * Project 3: Networked Spell Checker
 * 11/12/2021
 * 
 * The spell_checker.c file contains the server, dictionary, and spell checking functionality. 
 * Includes spell_checker.h that contains structs for buffers, defined variables for arrays, and all 
 * other nessesary header files.
**/

// Declare config variables.
int max_conn_buff_size = 5; 
int max_conn_threads = 10;
int priority_mode = 0; // 0 for fifo, 1 for random priority mode.
int socket_priority = 0;

// Declare global dictionary loaded in main.
char *dictionary[MAX_DICTIONARY];

// Global buffer variables.
socket_buffer _socket_buffer;
log_buffer _log_buffer;
int server_socket;
int priority = 0;

void put_socket(int socket_id, int priority);
client_socket get_socket(); 
void put_log(char *log);
char* get_log();
void* worker(void* args);
void* logger(void* args);
void init_socket_buffer();
void destroy_socket_buffer();
void init_log_buffer();
void destroy_log_buffer();
int loadDictionary(const char filename[]);
int check_dictionary(char *word);
int is_numeric(const char arg[]);
void prompt();
void sort();
int compare(const void *p, const void *q);

int main(int argc, char const *argv[])
{     
    srand(time(NULL));

    int portNumber = 8889;

    const char *dictionary_path;
    
    // ./spell_checker 5
    // arg0: ./spell_checker arg1 5 number of args 2

    /* 
    Check for the following arguments
    Port = 8888 Conn_Buffer_Size = 12 Worker_Threads = 5 Random Priority mode = 1
    | Condition: |
    ______________________________________________________________
    |     1.     |   ./mySpellChecker 8888 12 5 1
    |     2.     |   ./mySpellChecker 8888 myDictionary.txt 12 5 1 
    |     3.     |   ./mySpellChecker myDictionary.txt 12 5 1
    |     4.     |   ./mySpellChecker 12 5 1  
    */


    if(argc == 4) {                         // Condition 4.
        max_conn_buff_size = atoi(argv[1]);
        max_conn_threads = atoi(argv[2]);
       if(atoi(argv[3]) == 0 || atoi(argv[3]) == 1) {
           priority_mode = atoi(argv[3]);
        } else {
            prompt();
            exit(1);
        }
    } else if(argc == 5) {
        if(is_numeric(argv[1])) {           // Condition 1.
            portNumber = atoi(argv[1]);
        } else {
            dictionary_path = argv[1];      // Condition 3.
        }
        max_conn_buff_size = atoi(argv[2]);
        max_conn_threads = atoi(argv[3]);
       if(atoi(argv[4]) == 0 || atoi(argv[4]) == 1) {
           priority_mode = atoi(argv[4]);
        } else {
            prompt();
            exit(1);
        }
    } else if(argc == 6) {
        if(is_numeric(argv[1])) {           // Condition 2.
            portNumber = atoi(argv[1]);
            dictionary_path = argv[2];
        } else {
            dictionary_path = argv[1];      // Condition 2.
            portNumber = atoi(argv[2]);
        }
        max_conn_buff_size = atoi(argv[3]);
        max_conn_threads = atoi(argv[4]);
        
        if(atoi(argv[5]) == 0 || atoi(argv[5]) == 1) {
            priority_mode = atoi(argv[5]);
        } else {
            prompt();
            exit(1);
        }
    } else {    // Print instructional error message if not correct arguments..
        prompt();
        exit(1);
    }

    puts("Server Started");

    // Load the dictionary into memory.
    loadDictionary(dictionary_path);

    // Initialize server variables and bind.
    char server_message[] = "You are now connected to the server! Enter a word to spell check:\n";

   
    // Create socket.
    
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        puts("Could not create socket");
        return 1;
    }

    // Define address struct.
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(portNumber);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Initialize socket and log buffer
    init_socket_buffer();
    init_log_buffer();
   
    pthread_t worker_threads[max_conn_threads];
    pthread_t logger_thread;

     if (pthread_create(&logger_thread, NULL, &logger, NULL) != 0) {
            perror("Error creating logger threads.");
        }

    // Create worker threads.
    for(int i = 0; i < max_conn_threads; i++) {
        if (pthread_create(&worker_threads[i], NULL, &worker, NULL) != 0) {
            perror("Error creating worker threads.");
        }
    }
    
    
    // Bind to port and ip.
    bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));
    listen(server_socket, MAX_SOCKET_BACKLOG);

    // Accept new client socket connections and add to socket buffer.
    int temp_client_socket;
    
   // signal(SIGINT, exit_and_clear);
   int rand_num;
    while(1) {
            temp_client_socket = accept(server_socket, NULL, NULL);
            
            if (temp_client_socket < 0) {
                puts("Could not connect to client.");
            }
       
            int send_result;
            send_result = send(temp_client_socket, server_message, sizeof(server_message), 0);

            if (send_result == -1) {
                puts("Error sending result to client.");
            }
           if(priority_mode == 0) {
               put_socket(temp_client_socket, ++priority);
           } else {
               rand_num = (rand() % 10) + 1;
               put_socket(temp_client_socket, rand_num);
            //    pthread_mutex_lock(&_socket_buffer.lock);
            
            //    pthread_mutex_unlock(&_socket_buffer.lock);
           }
            
            
    }

    printf("Server shutting down...");
    // Join worker threads before returning.
    for (int i = 0; i < max_conn_threads; i++) {
        if(pthread_join(worker_threads[i], NULL) != 0) {
            perror("Error joining threads.");
        }
    }
    // Close file before joinint logger thread by sending "EOF" to put log.
    // Send signal to close log file.
  
    if (pthread_join(logger_thread, NULL) != 0) {
        perror("Error joining threads.");
    }

    // Clear all mutex structs and buffers before returning.
    destroy_socket_buffer();
    destroy_log_buffer();
    close(server_socket);
    printf("exit");
    return 0;
}



/**
 * The is_numeric function will check if all characters within a character
 * array are numeric.
 * 
 * @param arg The character array to inspect.
 * @return int 0 if not numeric or 1 if all numeric.
 */
int is_numeric(const char arg[]) {
    for (size_t i = 0; i < strlen(arg); i++)
    {
       if(!isdigit(arg[i])) {
           return 0;
       }
    }
    return 1;
}

void prompt() {
        printf("Check for the following arguments\n");
        printf("Port = 8888 Conn_Buffer_Size = 12 Worker_Threads = 5 Random Priority mode = 1\n");
        printf("| Condition: |\n");    
        printf("______________________________________________________________\n");
        printf("|     1.     |   ./mySpellChecker 8888 12 5 1\n");
        printf("|     2.     |   ./mySpellChecker 8888 myDictionary.txt 12 5 1\n");
        printf("|     3.     |   ./mySpellChecker myDictionary.txt 12 5 1\n");
        printf("|     4.     |   ./mySpellChecker 12 5 1  \n");
}

/**
 * The init_socket_buffer function will initialize all lock, conditional, and
 * buffer variables when called.
 */
void init_socket_buffer() {
    _socket_buffer.client_socket = malloc(sizeof(int) * max_conn_buff_size);
    if((_socket_buffer.client_socket) == NULL) {
        perror("Error: Unable to allocate memory for socket buffer!\n");
    }
    _socket_buffer.fill_ptr = 0;
    _socket_buffer.use_ptr = 0;
    _socket_buffer.arr_count = 0;
    pthread_mutex_init(&(_socket_buffer.lock), NULL);
    pthread_cond_init(&(_socket_buffer.empty_cond), NULL);
    pthread_cond_init(&(_socket_buffer.fill_cond), NULL);
}

/**
 * The init_socket_buffer function will destroy all lock, conditional, and
 * buffer variables when called.
 */
void destroy_socket_buffer() {
    pthread_mutex_destroy(&(_socket_buffer.lock));
    pthread_cond_destroy(&(_socket_buffer.empty_cond));
    pthread_cond_destroy(&(_socket_buffer.fill_cond));
    free(_socket_buffer.client_socket);
}

/**
 * The init_log_buffer function will initialize all lock, conditional, and
 * buffer variables when called.
 */
void init_log_buffer() {
    _log_buffer.array = malloc(sizeof(char) * MAX_LOG);
    if((_log_buffer.array) == NULL) {
        perror("Error: Unable to allocate memory for socket buffer!\n");
    }
    _log_buffer.fill_ptr = 0;
    _log_buffer.use_ptr = 0;
    _log_buffer.arr_count = 0;
    pthread_mutex_init(&(_log_buffer.lock), NULL);
    pthread_cond_init(&(_log_buffer.empty_cond), NULL);
    pthread_cond_init(&(_log_buffer.fill_cond), NULL);
}

/**
 * The init_log_buffer function will destroy all lock, conditional, and
 * buffer variables when called.
 */
void destroy_log_buffer() {
    pthread_mutex_destroy(&(_log_buffer.lock));
    pthread_cond_destroy(&(_log_buffer.empty_cond));
    pthread_cond_destroy(&(_log_buffer.fill_cond));
    free(_log_buffer.array);
}

/**
 * The put_socket function will place the latest client socket descriptor
 * into the circular buffer while maintaining access to the buffer and
 * waiting if it is full.
 * 
 * @param socket The client socket to place into the buffer.
 */
void put_socket(int socket, int _priority) {
    pthread_mutex_lock(&(_socket_buffer.lock));
    // Lock thread if too many items in buffer.
    while(_socket_buffer.arr_count >= max_conn_buff_size) {
        pthread_cond_wait(&(_socket_buffer.empty_cond), &(_socket_buffer.lock));
    }
    _socket_buffer.client_socket[_socket_buffer.fill_ptr].socket_id = socket;
    _socket_buffer.client_socket[_socket_buffer.fill_ptr].priority = _priority;
    sort();
    //printf("%d\n", array[fill_ptr]);
    _socket_buffer.fill_ptr = (_socket_buffer.fill_ptr + 1) % max_conn_buff_size;
    _socket_buffer.arr_count++;
    // Send signal of adding one to buffer.
    pthread_cond_signal(&_socket_buffer.fill_cond);
    pthread_mutex_unlock(&(_socket_buffer.lock));
}

/**
 * The get_socket function will remove a client socket descriptor from the 
 * circular buffer while maintaining access to the buffer and
 * waiting if it is empty.
 * 
 * @return int The client socket descriptor.
 */
client_socket get_socket() {
    pthread_mutex_lock(&(_socket_buffer.lock));
    while(_socket_buffer.arr_count < 1) {
        pthread_cond_wait(&(_socket_buffer.fill_cond), &(_socket_buffer.lock));
    }
    client_socket temp = _socket_buffer.client_socket[_socket_buffer.use_ptr];
    _socket_buffer.use_ptr = (_socket_buffer.use_ptr + 1) % max_conn_buff_size;
    _socket_buffer.arr_count--;
    pthread_cond_signal(&(_socket_buffer.empty_cond));
    pthread_mutex_unlock(&(_socket_buffer.lock));
    return temp;
}

/**
 * The function for worker threads to use. Removes first file descriptor from
 * buffer, reads words one by one, and compares them to the dictionary.
 * Responds to client by sending word back with ok or misspelled concatenated to
 * the end. Also adds a formated log string to the log buffer.
 * 
 * @param args Not used.
 * @return void* Not used.
 */
void* worker(void* args) {
    client_socket temp_client_socket;
    char word[MAX_WORD_SIZE];
    int result = 0;
    char s_log[MAX_LOG];
    
   // printf("SOCKET = %d THREAD = %ld BUFFER_COUNT = %d\n", get_socket(), pthread_self(), _socket_buffer.arr_count);

    char ok[] = " : OK";
    char miss[] = " : MISPELLED";
    struct timeval start_time, finish_time;
   
    while(1) {
      //sleep(1); //**************************** Uncomment here to test upper bound of pthread_wait.
      
      //"SOCKET = %d THREAD = %ld BUFFER_COUNT = %d\n", get_socket(), pthread_self(), _socket_buffer.arr_count
        temp_client_socket = get_socket();
        while(read(temp_client_socket.socket_id, word, MAX_WORD_SIZE) > 0) {
            gettimeofday(&start_time, NULL);
            // Shutdown server if response is below.
            // if(strcmp(word, "shutdown server\n") == 0) {
            //     write(temp_client_socket, word, MAX_WORD_SIZE);
            //     exit_and_clear();
            // }

            result = check_dictionary(word);

            if(result == 1) {
                strcat(word, ok);
                strcat(word, "\n");
                write(temp_client_socket.socket_id, word, MAX_WORD_SIZE);

            }   else {
                strcat(word, miss);
                strcat(word, "\n");
                write(temp_client_socket.socket_id, word, MAX_WORD_SIZE);       
            }   
            gettimeofday(&finish_time, NULL);
            // TODO: add priority to log
            // Format and print to log file.
            sprintf(s_log, "REQUEST_TIME_SECONDS = %ld REQUEST_TIME_MICRO_SECONDS = %ld\nCOMPLETED_TIME_SECONDS = %ld COMPLETED_TIME_MICRO_SECONDS = %ld\nPRIORITY = %d SPELLING_RESULT = %s ",
            start_time.tv_sec, start_time.tv_usec, finish_time.tv_sec, finish_time.tv_usec, temp_client_socket.priority, word);
            put_log(s_log);

            memset(word, 0, MAX_WORD_SIZE);
            result = 0;
            
        }

        close(temp_client_socket.socket_id);
    }
    return 0;
}

// void exit_and_clear() {
//     destroy_socket_buffer();
//     destroy_log_buffer();
//     close(server_socket);
//     printf("exit");
//     exit(0);
// }

/**
 * The put_log function will place the latest log string
 * into the circular buffer while maintaining access to the buffer and
 * waiting if it is full.
 * 
 * @param log The log string to place into the buffer.
 */
void put_log(char *log) {
    pthread_mutex_lock(&(_log_buffer.lock));
    // Lock thread if too many items in buffer.
    while(_log_buffer.arr_count >= max_conn_buff_size) {
        pthread_cond_wait(&(_log_buffer.empty_cond), &(_socket_buffer.lock));
    }
    _log_buffer.array[_log_buffer.fill_ptr] = log;
    //printf("%d\n", array[fill_ptr]);
    _log_buffer.fill_ptr = (_log_buffer.fill_ptr + 1) % MAX_LOG;
    _log_buffer.arr_count++;
    // Send signal of adding one to buffer.
    pthread_cond_signal(&_log_buffer.fill_cond);
    pthread_mutex_unlock(&(_log_buffer.lock));
}

/**
 * The get_log function will remove a log string from the 
 * circular buffer while maintaining access to the buffer and
 * waiting if it is empty.
 * 
 * @return int The string to print to the log file.
 */
char* get_log() {
    pthread_mutex_lock(&(_log_buffer.lock));
    while(_log_buffer.arr_count < 1) {
        pthread_cond_wait(&(_log_buffer.fill_cond), &(_log_buffer.lock));
    }
    char *temp = _log_buffer.array[_log_buffer.use_ptr];
    _log_buffer.use_ptr = (_log_buffer.use_ptr + 1) % MAX_LOG;
    _log_buffer.arr_count--;
    pthread_cond_signal(&(_log_buffer.empty_cond));
    pthread_mutex_unlock(&(_log_buffer.lock));
    return temp;
}

/**
 * The logger function removes log strings from the circular buffer and 
 * prints them to the log file one by one.
 * 
 * @param args 
 * @return void* 
 */
void* logger(void* args) {
    FILE *log_file;
    
    char *temp;
    while(1) {
        log_file = fopen("log.txt", "a");
        if(log_file == NULL) {
        perror("Error: Log file cannot be opened!");
        }
        temp = get_log();
        fprintf(log_file,"%s\n", temp);
        fclose(log_file);
    }
    return 0;
}

/**
 * The check_dictionary function will compare the word to the array or words
 * in the global dictionary variable. 
 * 
 * @param word The word to check against the dictionary.
 * @return int 0 if the word does not match or 1 if it does match any word in
 * the dictionary.
 */
int check_dictionary(char word[]) {
    int correct = 0;
    word[strcspn(word, "\n")] = 0;
    word[strcspn(word, "\t")] = 0;
    word[strcspn(word, " ")] = 0;


    int i = 0; 
   // Compare word to all words in dictionary and break with 1 returned if found.
    while(dictionary[i] != NULL) {
        
        if(strcmp(word, dictionary[i]) == 0) {
            correct = 1;
            break;
        }
        i++;
    }   
    return correct;
}

/**
 * The loadDictionary function will load the default dictionary if
 * the systems dictionary cannot be found.
 * 
 * @param dictionary The pointer to the memory location the dictionary is stored in.
 * Use an empty string if default dictionary is needed.
 * @return The file path of the dictionary to use.
 */
int loadDictionary(const char filename[]) {
    FILE *input_file = fopen(filename, "r");
    size_t line_size = 64;
    
    // Check if file path is valid or not. Use default if needed.
    if (input_file == NULL) {
        input_file = fopen("dictionary.txt", "r");
        puts("No specified dictionary or incorrect path. Using default dictionary!");
        if (input_file == NULL) {
            puts("Error opening all dictionary files!");
            return -1;
        }
    }

    // Store words from file in dictionary array.
  int i = 0;
  while(getline(&dictionary[i], &line_size, input_file) > 0)  { 
      // Remove newline chars.
      dictionary[i][strcspn(dictionary[i], "\n")] = 0;
      i++;   
  }
  dictionary[i] = NULL;

  fclose(input_file);

  return 0;
}

void sort() {
    socket_buffer temp;
    temp.client_socket = malloc(sizeof(int) * max_conn_buff_size);
    if((temp.client_socket) == NULL) {
        perror("Error: Unable to allocate memory for socket buffer!\n");
    }
    temp.fill_ptr = 0;
    temp.use_ptr = 0;
    temp.arr_count = 0;

    for (size_t i = 0; i < max_conn_buff_size; i++)
    {
        temp.client_socket[i] = get_socket();
    }

    qsort(temp.client_socket, max_conn_buff_size, sizeof(client_socket), compare);

    for (size_t i = 0; i < max_conn_buff_size; i++)
    {
        put_socket(temp.client_socket[i].socket_id, temp.client_socket[i].priority);
    }
    
    
}

int compare(const void *p, const void *q) 
   { 
       int l = ((client_socket*)p)->priority;
       int r = ((client_socket*)q)->priority;
      return (l-r);
} 