#pragma once
#include "common.h"

// A debug macro to check how many controllers entered
// the system and how many exited the system.
#ifdef DEBUG
#define DEBUG_LOG(msg)  \
    fprintf(stderr, "Controller: [%lu] %s\n", (unsigned long) pthread_self(), msg)
#else
#define DEBUG_LOG(msg)
#endif

typedef struct {
    int socket_client;
} controller_arg_t;

void* controller_func(void* arg);
