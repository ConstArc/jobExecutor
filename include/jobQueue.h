#pragma once
#include "common.h"

typedef struct queue* JobQueue;

Job* create_job(int id, cmd_hdr* command, int sockfd);
void destroy_job(Job* job_to_destroy);

JobQueue jobQueue_create(int max_size);

bool jobQueue_is_empty(const JobQueue JQ);
int jobQueue_size(const JobQueue JQ);
int jobQueue_max_size(const JobQueue JQ);
bool jobQueue_is_full(const JobQueue JQ);

void jobQueue_enqueue(const JobQueue JQ, Job* new_job);
Job* jobQueue_dequeue(const JobQueue JQ);
bool jobQueue_remove_by_id(const JobQueue JQ, int id);

Job* jobQueue_job_array_create(const JobQueue JQ, int* arr_size);
Job** jobQueue_job_ptr_array_create(const JobQueue JQ, int* arr_size);

void jobQueue_destroy(const JobQueue JQ);
void jobQueue_empty_it(const JobQueue JQ);