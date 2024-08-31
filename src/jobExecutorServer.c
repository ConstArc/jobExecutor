#include "common.h"
#include "parsingUtils.h"
#include "processUtils.h"
#include "controller.h"
#include "worker.h"
#include "jobQueue.h"

#define BACKLOG 32

// Error codes for accept system call
#define BREAK -1
#define EXIT_ERROR -2
#define CONTINUE -3

// Basic system synchro for workers, controllers
volatile int concurrency = 1;
volatile int active_workers = 0;
volatile int id = 1;
JobQueue bufferQueue;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty_or_max_concur = PTHREAD_COND_INITIALIZER;

// Synchro for exiting routine
pthread_t main_thread;
volatile bool exit_initiated = false;
volatile int active_controllers = 0;
pthread_mutex_t controllers_mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t last_controller_done = PTHREAD_COND_INITIALIZER;

// Server arguments' helper struct
typedef struct srvr_args {
    int portnum;
    int bufferSize;
    int threadPoolSize;
} srvr_args;

// Helper functions

static int init_args(int argc, char** argv, srvr_args* server_args_ptr);
static pthread_t* create_workers(int threadPoolSize);
static void create_controller(int client_sockfd);
static void server_cleanup(srvr_args* server_args_ptr, pthread_t* workers_arr);
static int handle_accept(int main_sock);

// Main socket file descriptor, where server accepts clients 
static int server_socketfd = -1;

// SIGUSR1 handler (exiting routine)
void sigusr1_handler() { 
    exit_initiated = true; 
    HANDLE_ERROR_ZERO("close", close(server_socketfd));
}

///////////////////////////////////////////////////////////////
//                Server's (main thread) Main                //
///////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) {

    // Setup signal handler for exiting routine
    Sigaction(SIGUSR1, sigusr1_handler);

    main_thread = pthread_self();

    // Initialize server arguments
    srvr_args* server_args;
    HANDLE_ERROR("malloc", server_args = malloc(sizeof(srvr_args)), NULL);
    HANDLE_ERROR("init_args", init_args(argc, argv, server_args), -1);

    // Create buffer-queue with max-size = bufferSize
    bufferQueue = jobQueue_create(server_args->bufferSize);

    HANDLE_ERROR("socket", server_socketfd = socket(AF_INET, SOCK_STREAM, 0), -1);

    // Bind server_socketfd on portNum
    Bind(server_socketfd, server_args->portnum);

    HANDLE_ERROR("listen",  listen(server_socketfd, BACKLOG), -1);

    pthread_t* workers_arr = create_workers(server_args->threadPoolSize);

    // Server's loop for accepting clients
    while(!exit_initiated) {

        int newclient_sockfd = handle_accept(server_socketfd);

        // Exit loop if socket is closed or invalid
        if (newclient_sockfd == BREAK)
            break;

        // Continue the loop on other accept errors
        else if (newclient_sockfd == CONTINUE)
            continue;

        // Critical error, violently terminates the server
        else if (newclient_sockfd == EXIT_ERROR)
            HANDLE_ERROR("accept", -1, -1);

        // All ok, create controller with the client's socket
        create_controller(newclient_sockfd);
    }

    // Wait for all remaining active controllers to terminate
    LOCK(&controllers_mtx);
        while(active_controllers > 0)
            WAIT(&last_controller_done, &controllers_mtx);
    UNLOCK(&controllers_mtx);

    // Wait for all remaining active workers to terminate
    for(int i = 0; i < server_args->threadPoolSize; i++)
        HANDLE_ERROR_ZERO("pthread_join", pthread_join(workers_arr[i], NULL));

    // Perform cleanup routine
    server_cleanup(server_args, workers_arr);

    // Exiting...
    fprintf(stderr, "Server exiting...\n");
    exit(EXIT_SUCCESS);
}

// Performs a small yet crucial sanitization check of the command line
// arguments passed from the user and initializes the helper struct of
// the server's arguments [server_args_ptr].
// On success 0 is returned.
// On error -1 is returned.
int init_args(int argc, char** argv, srvr_args* server_args_ptr) {
    if (argc < 4)
        return -1;
    
    if(!isPositiveIntegerNumber(argv[1]) || !isPositiveIntegerNumber(argv[2]) || !isPositiveIntegerNumber(argv[3]))
        return -1;

    server_args_ptr->portnum        = atoi(argv[1]);
    server_args_ptr->bufferSize     = atoi(argv[2]);
    server_args_ptr->threadPoolSize = atoi(argv[3]);

    return 0;
}

// Creates [threadPoolSize] number of worker threads. Their pthread_t structs
// are stored in a [threadPoolSize] size array, which is returned for further
// processing.
pthread_t* create_workers(int threadPoolSize) {

    pthread_t* workers_arr;
    HANDLE_ERROR("calloc", workers_arr = calloc(threadPoolSize, sizeof(pthread_t)), NULL);

    for (int i = 0; i < threadPoolSize; i++)
        HANDLE_ERROR_ZERO("pthread_create", pthread_create(&workers_arr[i], NULL, worker_func, NULL));

    return workers_arr;
}

// Handles the accept call of the server.
// On success, the new client's socket fd is returned.
// On error, the return values are:
//   - BREAK: the server's main loop of accepting new clients
//            should break, in case the exit routine has been
//            initiated or the main socket has been closed for
//            any reason.
//
//   - EXIT_ERROR: the server should immediatelly and violently
//                 terminate itself due to an unexpected
//                 critical error.
//
//   - CONTINUE: the accept failed but it was a non-critical
//               error therefore the server's main loop should
//               continue.
int handle_accept(int main_sock) {

    struct sockaddr_in client;
    socklen_t clientlen = sizeof(client);
    struct sockaddr* clientptr = (struct sockaddr*) &client;
    memset(&client, 0, sizeof(struct sockaddr_in));

    // Use of accept4 (GNU extension) with the SOCK_CLOEXEC flag enabled
    // so that children processes (issueJobs) won't be able to tamper
    // with the server's open fds.
    int newclient_sockfd = accept4(main_sock, clientptr, &clientlen, SOCK_CLOEXEC);
    
    // Not guaranteed to prevent from accepting a new connection
    // but it is worth a shot.
    if (exit_initiated)
        return BREAK;

    // Upon error
    if (newclient_sockfd < 0) {
        // Server should break from main loop (if the main_sock was closed)
        if (errno == EBADF || errno == EINVAL)
            return BREAK;

        // Errors that must NOT be ignored (violent exit on critical errors)
        else if (errno == EFAULT || errno == ENOTSOCK || errno == EOPNOTSUPP || errno == ENOMEM)
            return EXIT_ERROR;

        // Non-critical errors, server should continue
        else 
            return CONTINUE;  
    }

    // Upon success
    return newclient_sockfd;
}

// Creates a controller thread with the client's socket fd. After creation
// the thread is detached.
void create_controller(int client_sockfd) {
    pthread_t controller;
    controller_arg_t* controller_data;
    HANDLE_ERROR("malloc", controller_data = malloc(sizeof(controller_arg_t)), NULL);
    controller_data->socket_client = client_sockfd;

    // Not guaranteed to prevent from creating a new controller thread
    // but it is worth a shot.
    if (exit_initiated) {
        free(controller_data);
        return;
    }

    // Create the controller
    HANDLE_ERROR_ZERO("pthread_create", pthread_create(&controller, NULL, controller_func, (void*)controller_data));

    // Detach the controller
    HANDLE_ERROR_ZERO("pthread_detach", pthread_detach(controller));
}

// Server's cleanup routine.
// Frees [server_args], [workers_arr]. Destroys buffer-queue
// and destroys all mutexes and condition variables.
void server_cleanup(srvr_args* server_args_ptr, pthread_t* workers_arr) {

    // Free allocated space for helper structs
    free(server_args_ptr);
    free(workers_arr);

    // Destroy buffer-queue
    jobQueue_destroy(bufferQueue);

    // Destroy mutexes and condition variables
    HANDLE_ERROR_ZERO("pthread_mutex_destroy", pthread_mutex_destroy(&mtx));
    HANDLE_ERROR_ZERO("pthread_mutex_destroy", pthread_mutex_destroy(&controllers_mtx));
    HANDLE_ERROR_ZERO("pthread_cond_destroy", pthread_cond_destroy(&full));
    HANDLE_ERROR_ZERO("pthread_cond_destroy", pthread_cond_destroy(&empty_or_max_concur));
    HANDLE_ERROR_ZERO("pthread_cond_destroy", pthread_cond_destroy(&last_controller_done));
}