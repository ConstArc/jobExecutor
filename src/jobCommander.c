#include "common.h"
#include "processUtils.h"
#include "parsingUtils.h"

// Commander helper struct
typedef struct cmdr_args {
    char* serverName;
    int   portNum;
    cmd_hdr header;
    char* args_str;
} cmdr_args;

// Helper functions

static int init_args(int argc, char** argv, cmdr_args* commander_args_ptr);
static bool init_header(int argc, char** argv, cmd_hdr* header, char** args_str);
static void print_data_from_socket(int sockfd);
static void connect_to_server(int sock, struct hostent* rem, cmdr_args* commander_args_ptr);

int main(int argc, char* argv[]) {

    cmdr_args* commander_args_ptr;
    HANDLE_ERROR("malloc", commander_args_ptr = malloc(sizeof(cmdr_args)), NULL);
    HANDLE_ERROR("init_args", init_args(argc, argv, commander_args_ptr), -1);

    int sock;
    HANDLE_ERROR("socket", sock = socket(AF_INET, SOCK_STREAM, 0), -1);

    struct hostent* rem;
    HANDLE_ERROR("gethostbyname", rem = gethostbyname(commander_args_ptr->serverName), NULL);

    connect_to_server(sock, rem, commander_args_ptr);

    Write(sock, &(commander_args_ptr->header), sizeof(cmd_hdr));

    if (commander_args_ptr->header.cmd_id == ISSUE_JOB)
        Write(sock, commander_args_ptr->args_str, commander_args_ptr->header.args_str_size);

    // No more writing on the socket
    HANDLE_ERROR("shutdown", shutdown(sock, SHUT_WR), -1);

    // Print response from server
    print_data_from_socket(sock);

    // Close socket
    HANDLE_ERROR("close", close(sock), -1);
    exit(EXIT_SUCCESS);
}

int init_args(int argc, char** argv, cmdr_args* commander_args_ptr) {
    if (argc < 4)
        return -1;
    
    if(!isPositiveIntegerNumber(argv[2]))
        return -1;

    commander_args_ptr->serverName = strdup(argv[1]);
    commander_args_ptr->portNum = atoi(argv[2]);

    if(!init_header(argc-3, argv+3, &(commander_args_ptr->header), &(commander_args_ptr->args_str)))
        return -1;

    return 0;
}

// Performs an initial parameter check of the expected commander's
// arguments formats and initializes the command header buffer.
// Upon success, true is returned, else false is returned.
bool init_header(int argc, char** argv, cmd_hdr* header, char** args_str) {
    if (argc < 1 || argv == NULL)
        return false;
    for (int i = 0; i < argc; i++) {
        int len = strlen(argv[i]);
        if (len >= MAX_STR_SIZE)
            return false;
    }

    if (strcmp(argv[0], "issueJob") == 0) {
        if (argc-1 == 0)
            return false;
        
        header->cmd_id = ISSUE_JOB;
        header->n_args = argc-1;

        header->args_str_size = concatenated_args_size(argv+1, header->n_args);

        HANDLE_ERROR("malloc", *args_str = malloc(header->args_str_size), NULL);
        concatenate_args(*args_str, argv+1, header->n_args);
    
    }
    else if (strcmp(argv[0], "setConcurrency") == 0) {
        if (!validNumberOfArgs(argc-1, 1) || !isPositiveIntegerNumber(argv[1]))
            return false;

        header->cmd_id = SET_CONCURRENCY;
        header->n_args = 1;
        
        concatenate_args(header->small_args_str, argv+1, 1);
        header->args_str_size = 0;
    }
    else if (strcmp(argv[0], "stop") == 0) {
        if(!validNumberOfArgs(argc-1, 1))
            return false;
    
        header->cmd_id = STOP;
        header->n_args = 1;
        
        concatenate_args(header->small_args_str, argv+1, 1);
        header->args_str_size = 0;
    }
    else if (strcmp(argv[0], "poll") == 0) {
        if(!validNumberOfArgs(argc-1, 0))
            return false;
    
        header->cmd_id = POLL;
        header->n_args = 1;
        
        concatenate_args(header->small_args_str, argv, 1);
        header->args_str_size = 0;
    }
    else if (strcmp(argv[0], "exit") == 0) {
        if(!validNumberOfArgs(argc-1, 0))
            return false;

        header->cmd_id = EXIT;
        header->n_args = 0;
        header->args_str_size = 0;
    }
    else 
        return false;

    return true;
}

// Connects to server
void connect_to_server(int sock, struct hostent* rem, cmdr_args* commander_args_ptr) {
    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(commander_args_ptr->portNum);

    struct sockaddr* serverptr = (struct sockaddr*)&server;

    for (int i = 0; rem->h_addr_list[i] != NULL; i++) {
        memcpy(&server.sin_addr, rem->h_addr_list[i], rem->h_length);

        if (connect(sock, serverptr, sizeof(server)) < 0) {
            if (errno == ECONNREFUSED)
                 // Try the next address
                continue;
            else {
                close(sock);
                free(commander_args_ptr);
                HANDLE_ERROR("connect", -1, -1);
            }
        }
        else
            // Successfully connected
            return;
    }

    printf("Failed to connect to any address.\n");
    close(sock);
    free(commander_args_ptr);
    HANDLE_ERROR("connect", -1, -1);
}

// Prints data from [sockfd] until EOF is read,
// before shutting down RD from [sockfd] and returns.
void print_data_from_socket(int sockfd) {
    ReadWrite(STDOUT_FILENO, sockfd);

    shutdown(sockfd, SHUT_RD);
}