#include "common.h"
#include "processUtils.h"

// A wrapper function for setting up a sigaction struct
void Sigaction(int SIG, void (*handler_func)(int))  {

    struct sigaction sa;
    sa.sa_handler = handler_func;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    HANDLE_ERROR("sigaction", sigaction(SIG, &sa, NULL), -1);
}

// A wrapper function for read system call, it iteratively reads bytes from 
// [fd] into [dest], until all [n_bytes] are succesfully read.
void Read(int fd, void* dest, size_t n_bytes) {

    long bytes_left = (long) n_bytes;
    char* dest_ptr = dest;
    ssize_t bytes_read = 0;

    // As long as we have more bytes to read to reach [n_bytes]
    while(bytes_left > 0) {

        // Try to read 'bytes_left' bytes from [fd] and store them where the dest_ptr points.
        // 'bytes_read' stores how many bytes read() actually read in this iteration
        bytes_read = read(fd, dest_ptr, bytes_left);
        
        if (bytes_read == 0)
            break;

        if (bytes_read == -1 && errno == EINTR)
            continue;
        
        if (bytes_read == -1) 
            HANDLE_ERROR("read", bytes_read, -1);

        bytes_left -= bytes_read;     // Update 'bytes_left'
        dest_ptr += bytes_read;       // Move the dest_ptr 'bytes_read' bytes forward
    }

    // All [n_bytes] have been read successfully
}


// A wrapper function for write system call, it iteratively writes bytes from
// [src] to [fd], until all [n_bytes] are successfully written.
void Write(int fd, void* src, size_t n_bytes) {

    const char* src_ptr = src;
    ssize_t bytes_left = n_bytes;
    ssize_t bytes_written = 0;

    // As long as we have more bytes to write to reach [n_bytes]
    while (bytes_left > 0) {

        // Try to write 'bytes_left' bytes to fd from src_ptr. 'bytes_written'
        // stores how many bytes write() actually wrote in this iteration
        bytes_written = write(fd, src_ptr, bytes_left);

        // Interrupt, try again
        if (bytes_written == -1 && errno == EINTR)
            continue;

        // Other errors
        if (bytes_written == -1)
            HANDLE_ERROR("write", bytes_written, -1);

        bytes_written += bytes_written;    // Update total bytes written
        bytes_left -= bytes_written;       // Update 'bytes_left'
        src_ptr += bytes_written;          // Move the src_ptr 'bytes_written' bytes forward
    }

    // All [n_bytes] have been written successfully
}

// Binds [sock] on [port]
void Bind(int sock, short port) {

    struct sockaddr_in server;
    server.sin_family = AF_INET ;
    server.sin_addr.s_addr = htonl(INADDR_ANY) ;
    server.sin_port = htons(port) ;

    struct sockaddr* serverptr = (struct sockaddr*) &server;
    HANDLE_ERROR("bind", bind(sock, serverptr, sizeof(server)), -1);
}


// Writes data from [source_fd] to [dest_fd]
// until EOF is read, and returns.
void ReadWrite(int dest_fd, int source_fd) {
    ssize_t bytes_read = 0;
    char temp_buf[BATCH_SIZE];

    while(true) {
        bytes_read = read(source_fd, temp_buf, BATCH_SIZE);
        
        // EOF
        if (bytes_read == 0)
            break;

        // Interrupt, try again
        if (bytes_read == -1 && errno == EINTR)
            continue;

        // Other errors
        if (bytes_read == -1)
            HANDLE_ERROR("read", bytes_read, -1);

        Write(dest_fd, temp_buf, bytes_read);
    }
}