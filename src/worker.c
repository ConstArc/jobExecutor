#include "common.h"
#include "jobQueue.h"
#include "processUtils.h"
#include "parsingUtils.h"
#include "worker.h"

// Basic system synchro for workers, controllers
extern int concurrency;
extern JobQueue bufferQueue;
extern int active_workers;
extern pthread_mutex_t mtx;
extern pthread_cond_t full;
extern pthread_cond_t empty_or_max_concur;

// Synchro for exiting routine
extern bool exit_initiated;

// Helper functions

static pid_t exec_job(Job* job);
static void send_output(int sockfd, int job_id, pid_t chld_pid);
static void send_data_from_file(int sockfd, int file_fd);

//////////////////////////////////////////////////////////////////////
//                Worker's Thread Function                          //
//////////////////////////////////////////////////////////////////////
void* worker_func(void* arg __attribute__((unused))) {              // No arguments needed
    while(true) {
        LOCK(&mtx);
            assert(active_workers >= 0);

            // While the buffer-queue is empty or we have max concurrency of active_workers, and exit routine
            // is not initiated, sleep
            while( (jobQueue_is_empty(bufferQueue) || (active_workers == concurrency) ) && !exit_initiated )
                WAIT(&empty_or_max_concur, &mtx);
            
            // If exit routine initiated, just break, aka
            // don't dequeue a waiting job for execution.
            if (exit_initiated) {
                UNLOCK(&mtx);

                break;
            }

            // Dequeue a job from our buffer-queue
            Job* job_to_execute = jobQueue_dequeue(bufferQueue);

            // Buffer-queue is definitely not full from now on
            SIGNAL(&full);

            // And now we are active
            active_workers++;
        UNLOCK(&mtx);

        // Place job for execution and get the child's
        // process pid
        pid_t chld_pid = exec_job(job_to_execute);

        // Wait for child process to finish
        int status;
        HANDLE_ERROR("wait", wait(&status), -1);

        // Send output to the client
        send_output(job_to_execute->sockfd, job_to_execute->id, chld_pid);

        // Free allocated space
        destroy_job(job_to_execute);

        // No more active
        LOCK(&mtx);
            active_workers--;

            SIGNAL(&empty_or_max_concur);
        UNLOCK(&mtx);
    }

    pthread_exit(NULL);
}

// Executes [job] calling execvp and returns the child's pid
pid_t exec_job(Job* job) {
    pid_t chld_pid;
    HANDLE_ERROR("fork", chld_pid = fork(), -1);
    if(chld_pid == 0) {
        // Creating a string vector by tokening the job's args string
        // with the separator being a single whitespace.
        char** argv = tokenize_str(job->args_str, job->n_args, ' ');

        // Appending null on the arguments vector
        append_null(&argv, job->n_args);

        // Creating output file
        char filename[MAX_STR_SIZE];
        snprintf(filename, sizeof(filename), "%s/%d.output", OUTPUT_DIR, getpid());

        int fd;
        HANDLE_ERROR("open", fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR), -1);

        // Redirecting stdout and stderr
        HANDLE_ERROR("dup2", dup2(fd, STDOUT_FILENO), -1);
        HANDLE_ERROR("dup2", dup2(fd, STDERR_FILENO), -1);

        close(fd);
        HANDLE_ERROR("execvp", execvp(argv[0], argv), -1);
    }

    return chld_pid;
}

// Sends output to the client at [client_sockfd]
void send_output(int client_sockfd, int job_id, pid_t chld_pid) {

    char header[MAX_STR_SIZE];
    snprintf(header, sizeof(header), "\n-----job_%d output start------\n", job_id);
    int header_len = strlen(header);

    char footer[MAX_STR_SIZE];
    snprintf(footer, sizeof(footer), "-----job_%d output end------\n\n", job_id);
    int footer_len = strlen(footer);

    // Get output file name and open it
    char output_file[MAX_STR_SIZE];
    snprintf(output_file, sizeof(output_file), "%s/%d.output", OUTPUT_DIR, chld_pid);
    int output_fd;
    HANDLE_ERROR("open", output_fd = open(output_file, O_RDONLY), -1);

    // Write response
    Write(client_sockfd, header, header_len);
    send_data_from_file(client_sockfd, output_fd);
    Write(client_sockfd, footer, footer_len);

    // Output file no longer needed, delete it
    HANDLE_ERROR("remove", remove(output_file), -1);

    // No more writing on [client_socketfd], shutdown(WR)
    // and close it.
    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
    HANDLE_ERROR("close", close(client_sockfd), -1);
}

// Writes data from [output_fd] to [client_sockfd] until EOF
// is read, before closing [output_fd] and returns.
void send_data_from_file(int client_sockfd, int output_fd) {

    ReadWrite(client_sockfd, output_fd);

    HANDLE_ERROR("close", close(output_fd), -1);
}