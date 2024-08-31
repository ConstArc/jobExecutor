#include "common.h"
#include "jobQueue.h"
#include "processUtils.h"
#include "controller.h"

// Basic system synchro for workers, controllers
extern int concurrency;
extern int active_workers;
extern int id;
extern bool exit_initiated;
extern JobQueue bufferQueue;
extern pthread_mutex_t mtx;
extern pthread_cond_t full;
extern pthread_cond_t empty_or_max_concur;

// Synchro for exiting routine
extern pthread_t main_thread;
extern int active_controllers;              // Number of active controllers
extern pthread_mutex_t controllers_mtx;     // Mutex to protect above counter 
extern pthread_cond_t last_controller_done; // Condition variable on the above counter

// Helper functions
static void execute_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr);
static void send_termination_msg(int client_sockfd);
static void controller_enter(void);
static void controller_exit(void);

// IssueJob
static void issueJob_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr);
static void issueJob_response(int client_sockfd, Job* job);

// SetConcurrency
static void setConcurrency_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr);
static void setConcurrency_response(int client_sockfd, int new_concurrency);

// Stop
static void stop_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr);
static void stop_response(int client_sockfd, int job_id, bool found);

// Poll
static void poll_cmd(int client_sockfd);
static void poll_response(int client_sockfd, Job* job_array, int size);

// Exit
static void exit_cmd(int client_sockfd);
static void exit_response(int client_sockfd);

//////////////////////////////////////////////////////////////////////
//                Controller's Thread Function                      //
//////////////////////////////////////////////////////////////////////
void* controller_func(void* arg) {
    controller_enter();

    controller_arg_t* controller_data = (controller_arg_t*)arg;
    int client_sockfd = controller_data->socket_client;
    free(controller_data);

    cmd_hdr cmd_header;
    Read(client_sockfd, &cmd_header, sizeof(cmd_hdr));

    execute_cmd(client_sockfd, &cmd_header);

    controller_exit();

    return NULL;
}

void execute_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr) {

    switch (cmd_header_ptr->cmd_id) {
        case ISSUE_JOB:
            issueJob_cmd(client_sockfd, cmd_header_ptr);
            break;

        case SET_CONCURRENCY:
            setConcurrency_cmd(client_sockfd, cmd_header_ptr);
            break;

        case STOP:
            stop_cmd(client_sockfd, cmd_header_ptr);
            break;

        case POLL:
            poll_cmd(client_sockfd);
            break;

        case EXIT:
            exit_cmd(client_sockfd);
            break;

        // Invalid command id, simply return
        default:
            return;
    }
}

void issueJob_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr) {

    // Read the arguments' string
    char* args_str = calloc(cmd_header_ptr->args_str_size, 1);
    HANDLE_ERROR("calloc", args_str, NULL);
    Read(client_sockfd, args_str, cmd_header_ptr->args_str_size);

    // We won't be reading anything else from this particular client
    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_RD), -1);

    LOCK(&mtx);

        // Create new job and increase id
        Job* new_job = create_job(id++, cmd_header_ptr, client_sockfd);
        new_job->args_str = args_str;

        // While buffer-queue is full, wait. If exit was initiated
        // though, don't wait.
        while(jobQueue_is_full(bufferQueue) && !exit_initiated)
            WAIT(&full, &mtx);

        // If exit was initiated, exit without enqueueing new job
        if (exit_initiated) {
            UNLOCK(&mtx);

            send_termination_msg(client_sockfd);
            destroy_job(new_job);

            controller_exit();
        }

        // Enqueue the new job
        jobQueue_enqueue(bufferQueue, new_job);

        // Signal a sleeping worker since buffer-queue 
        // is definitely no more empty
        SIGNAL(&empty_or_max_concur);

        // Send response back to jobCommander that issued the new job
        issueJob_response(client_sockfd, new_job);

    UNLOCK(&mtx);
}

// In contrast to other responses, no shutdown(WR) or close is performed
// on [client_sockfd], since it is expected (normal behaviour) that there
// is more response to be written to the particular client from the worker
// later on.
void issueJob_response(int client_sockfd, Job* job) {
    char header[MAX_STR_SIZE];
    snprintf(header, sizeof(header), "JOB < job_%d, ", job->id);
    int header_len = strlen(header);

    char footer[MAX_STR_SIZE];
    snprintf(footer, sizeof(footer), " > SUBMITTED\n");
    int footer_len = strlen(footer);

    Write(client_sockfd, header, header_len);
    Write(client_sockfd, job->args_str, job->args_str_size);
    Write(client_sockfd, footer, footer_len);
}

void setConcurrency_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr) {
    // We won't be reading anything else from this particular client
    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_RD), -1);

    LOCK(&mtx);
        if(exit_initiated) {
            UNLOCK(&mtx);

            send_termination_msg(client_sockfd);

            controller_exit();
        }

        // Get the current concurrency
        int old_concurrency = concurrency;

        // Update concurrency
        concurrency = atoi(cmd_header_ptr->small_args_str);

        // Store the new concurrency on a local variable
        // for further (safe) passing in response
        int new_concurrency = concurrency;

        // Concurrency got increased, signal all workers
        // awaiting due to max_concurrency cap
        if (old_concurrency < new_concurrency)
            BROADCAST(&empty_or_max_concur);
 
    UNLOCK(&mtx);

    setConcurrency_response(client_sockfd, new_concurrency);
}

void setConcurrency_response(int client_sockfd, int new_concurrency) {
    char response[MAX_STR_SIZE];
    snprintf(response, sizeof(response), "CONCURRENCY SET AT %d\n", new_concurrency);
    int response_len = strlen(response);

    Write(client_sockfd, response, response_len);

    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
    HANDLE_ERROR("close", close(client_sockfd), -1);
}

void stop_cmd(int client_sockfd, cmd_hdr* cmd_header_ptr) {
    // We won't be reading anything else from this particular client
    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_RD), -1);

    int id_to_remove;
    sscanf(cmd_header_ptr->small_args_str, "job_%d", &id_to_remove);

    LOCK(&mtx);
        if(exit_initiated) {
            UNLOCK(&mtx);

            send_termination_msg(client_sockfd);

            controller_exit();
        }

        bool found_flag = jobQueue_remove_by_id(bufferQueue, id_to_remove);
    UNLOCK(&mtx);

    stop_response(client_sockfd, id_to_remove, found_flag);
}

void stop_response(int client_sockfd, int job_id, bool found_flag) {
    char response[MAX_STR_SIZE];

    if (found_flag)
        snprintf(response, sizeof(response), "JOB <job_%d> REMOVED\n", job_id);
    else
        snprintf(response, sizeof(response), "JOB <job_%d> NOTFOUND\n", job_id);

    int response_len = strlen(response);

    Write(client_sockfd, response, response_len);

    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
    HANDLE_ERROR("close", close(client_sockfd), -1);
}

void poll_cmd(int client_sockfd) {
    // We won't be reading anything else from this particular client
    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_RD), -1);

    LOCK(&mtx);
        if(exit_initiated) {
            UNLOCK(&mtx);

            send_termination_msg(client_sockfd);

            controller_exit();
        }

        // Create a copy of jobs within the 'bufferQueue'
        int size;
        Job* job_array = jobQueue_job_array_create(bufferQueue, &size);
    
    UNLOCK(&mtx);

    poll_response(client_sockfd, job_array, size);
}

void poll_response(int client_sockfd, Job* job_array, int size) {
    char poll_header[MAX_STR_SIZE];
    snprintf(poll_header, sizeof(poll_header), "-----poll output start------\n");
    int poll_header_len = strlen(poll_header);

    char poll_footer[MAX_STR_SIZE];
    snprintf(poll_footer, sizeof(poll_footer), "-----poll output end------\n\n");
    int poll_footer_len = strlen(poll_footer);

    // Write poll header
    Write(client_sockfd, poll_header, poll_header_len);

    // buffer-queue is empty
    if (size == 0) {
        char response[MAX_STR_SIZE];
        snprintf(response, sizeof(response), "Job Buffer is Empty!\n");
        int response_len = strlen(response);

        Write(client_sockfd, response, response_len);

        // Write poll footer
        Write(client_sockfd, poll_footer, poll_footer_len);

        HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
        HANDLE_ERROR("close", close(client_sockfd), -1);

        return;
    }

    char job_footer[MAX_STR_SIZE];
    snprintf(job_footer, sizeof(job_footer), " >\n");
    int job_footer_len = strlen(job_footer);

    for (int i = 0; i < size; i++) {

        char job_header[MAX_STR_SIZE];
        snprintf(job_header, sizeof(job_header), "JOB < job_%d, ", (job_array[i]).id);
        int job_header_len = strlen(job_header);

        Write(client_sockfd, job_header, job_header_len);
        Write(client_sockfd, (job_array[i]).args_str, (job_array[i]).args_str_size);
        Write(client_sockfd, job_footer, job_footer_len);
    }

    // Free allocated space
    free(job_array);

    // Write poll footer
    Write(client_sockfd, poll_footer, poll_footer_len);

    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
    HANDLE_ERROR("close", close(client_sockfd), -1);
}

void exit_cmd(int client_sockfd){
    // We won't be reading anything else from this particular client
    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_RD), -1);

    LOCK(&mtx);
        if(exit_initiated) {
            UNLOCK(&mtx);

            send_termination_msg(client_sockfd);

            controller_exit();
        }

        HANDLE_ERROR_ZERO("pthread_kill", pthread_kill(main_thread, SIGUSR1));

        int size;
        Job** job_ptr_array = jobQueue_job_ptr_array_create(bufferQueue, &size);

        // Send to all awaiting commanders the informative message
        Job* job_i;
        if (size != 0) {
            for (int i = 0; i < size; i++) {
                job_i = job_ptr_array[i];
                send_termination_msg(job_i->sockfd);
            }

            free(job_ptr_array);
        }

        // Prevent worker threads from taking queued jobs
        jobQueue_empty_it(bufferQueue);

        // Signal all awaiting workers so that they can
        // terminate.
        BROADCAST(&empty_or_max_concur);

        // Signal all awaiting controllers so that they can
        // terminate.
        BROADCAST(&full);

        // Send response back to commander who sent exit
        exit_response(client_sockfd);
    UNLOCK(&mtx);
}

void exit_response(int client_sockfd) {

    char response[MAX_STR_SIZE];
    snprintf(response, sizeof(response), "SERVER TERMINATED\n");
    int response_len = strlen(response);

    Write(client_sockfd, response, response_len);

    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
    HANDLE_ERROR("close", close(client_sockfd), -1);
}

void send_termination_msg(int client_sockfd) {

    char response[MAX_STR_SIZE];
    snprintf(response, sizeof(response), "SERVER TERMINATED BEFORE EXECUTION\n");
    int response_len = strlen(response);

    Write(client_sockfd, response, response_len);

    HANDLE_ERROR("shutdown", shutdown(client_sockfd, SHUT_WR), -1);
    HANDLE_ERROR("close", close(client_sockfd), -1);
}

// Locks on 'controllers_mtx' to increase 
// the 'active_controllers' counter by one.
// Unlocks the 'controllers_mtx' and returns.
void controller_enter() {
    LOCK(&controllers_mtx);
        active_controllers++;

        DEBUG_LOG("entered.");
    UNLOCK(&controllers_mtx);
}

// Locks on 'controllers_mtx' to decrease 
// the 'active_controllers' counter by one.
// In case 'active_controllers' counter reaches 0
// perform signal on 'last_controller_done' condition
// variable.
// Unlocks the 'controllers_mtx' and calls pthread_exit().
void controller_exit() {
    LOCK(&controllers_mtx);
        active_controllers--;

        DEBUG_LOG("exiting...");

        // Signal server
        if(active_controllers == 0)
            SIGNAL(&last_controller_done);
    UNLOCK(&controllers_mtx);

    pthread_exit(NULL);
}
