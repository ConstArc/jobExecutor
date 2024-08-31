#pragma once

// GNU extensions
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <netdb.h>
#include <errno.h>
#include <poll.h>

// A simple macro to avoid too much code repetition
// on boilerplate checks upon (mainly) system calls
// and other functions too.
// Use this macro if upon error, err_value is returned
#define HANDLE_ERROR(func_name, expr, err_value)   \
    do {                                \
        if ((expr) == (err_value)) {    \
            perror(func_name);          \
            exit(EXIT_FAILURE);         \
        }                               \
    } while (0)

// Use this macro if upon success, 0 is returned and != 0 upon error
#define HANDLE_ERROR_ZERO(func_name, expr)    \
    do {                                      \
        if ((expr) != 0) {                    \
            perror(func_name);                \
            exit(EXIT_FAILURE);               \
        }                                     \
    } while (0)


// Macros for synchro functions for better readability
#define LOCK(mutex_ptr)   \
    do {                                            \
        if (pthread_mutex_lock(mutex_ptr) != 0) {   \
            perror("pthread_mutex_lock");           \
            exit(EXIT_FAILURE);                     \
        }                                           \
    } while (0)

#define UNLOCK(mutex_ptr)   \
    do {                                             \
        if (pthread_mutex_unlock(mutex_ptr) != 0) {  \
            perror("pthread_mutex_unlock");          \
            exit(EXIT_FAILURE);                      \
        }                                            \
    } while (0)

#define WAIT(cond_ptr, mutex_ptr)   \
    do {                                                     \
        if (pthread_cond_wait(cond_ptr, mutex_ptr) != 0) {   \
            perror("pthread_cond_wait");                     \
            exit(EXIT_FAILURE);                              \
        }                                                    \
    } while (0)

#define BROADCAST(cond_ptr)   \
    do {                                                     \
        if (pthread_cond_broadcast(cond_ptr) != 0) {         \
            perror("pthread_cond_broadcast");                \
            exit(EXIT_FAILURE);                              \
        }                                                    \
    } while (0)

#define SIGNAL(cond_ptr)   \
    do {                                                  \
        if (pthread_cond_signal(cond_ptr) != 0) {         \
            perror("pthread_cond_signal");                \
            exit(EXIT_FAILURE);                           \
        }                                                 \
    } while (0)

// Each command type is given its unique ID
enum COMMAND_ID {
    ISSUE_JOB,
    SET_CONCURRENCY,
    STOP,
    POLL,
    EXIT
};

// Default max string size
#define MAX_STR_SIZE 256

// Default batch size for iterative read/write
// on a file
#define BATCH_SIZE 2048

// Commander info header
typedef struct cmd_hdr {
    int    cmd_id;                              // Coresponding command id of command
    char   small_args_str[MAX_STR_SIZE];        // A small buffer for small argument strings
    size_t args_str_size;                       // Size of arguments string passed in arguments fifo
    size_t n_args;                              // Number of arguments passed for later processing
} cmd_hdr;

// Data of Queues' nodes
typedef struct Job {
    size_t args_str_size;
    size_t n_args;               // Number of arguments of arguments' string
    char*  args_str;             // Arguments' string
    int    id;                   // Unique Job's id
    int    sockfd;
} Job;