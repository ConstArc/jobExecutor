#include "common.h"
#include "jobQueue.h"
#include "parsingUtils.h"

//////////////////////////////////////////////////////////////////////

typedef struct QueueNode {
    Job*    data;
    struct QueueNode* next;
} QueueNode;

typedef struct queue {
    QueueNode* head;            // Head node
    QueueNode* tail;            // Tail node
    int cur_num_elements;       // Current number of elements in the queue
    int max_num_elements;       // Maximum number of elements in the queue
} queue;

//////////////////////////////////////////////////////////////////////

static QueueNode* create_node(void);
static void destroy_node(QueueNode* node);
static QueueNode* find_prev_node_by_id(const JobQueue JQ, int id);

//////////////////////////////////////////////////////////////////////

// Allocates space dynamically for a new node. Returns a pointer
// to this space.
QueueNode* create_node() {
    QueueNode* new_node = malloc(sizeof(QueueNode));
    HANDLE_ERROR("malloc", new_node, NULL);

    new_node->next = NULL;

    return new_node;
}

//////////////////////////////////////////////////////////////////////

Job* create_job(int id, cmd_hdr* command, int sockfd) {
    assert(command != NULL);

    Job* new_job;
    HANDLE_ERROR("malloc", new_job = malloc(sizeof(Job)), NULL);

    new_job->sockfd = sockfd;

    new_job->id = id;
    new_job->n_args = command->n_args;

    new_job->args_str_size = command->args_str_size;

    return new_job;
}

void destroy_job(Job* job_to_destroy) {
    assert(job_to_destroy != NULL);

    free(job_to_destroy->args_str);
    free(job_to_destroy);
}

// Allocates space and returns a new empty JobQueue
JobQueue jobQueue_create(int max_size) {
    assert(max_size > 0);

    JobQueue JQ = malloc(sizeof(queue));
    HANDLE_ERROR("malloc", JQ, NULL);

    JQ->head = NULL;
    JQ->tail = NULL;
    JQ->cur_num_elements = 0;
    JQ->max_num_elements = max_size;

    return JQ;
}

// Returns the size (current number of elements)
// of JobQueue [JQ].
int jobQueue_size(const JobQueue JQ) {
    assert(JQ != NULL);
    assert(JQ->cur_num_elements >= 0);

    return JQ->cur_num_elements;
}

// Returns the maximum size of JobQueue [JQ]
int jobQueue_max_size(const JobQueue JQ) {
    assert(JQ != NULL);

    return JQ->max_num_elements;
}

// Returns true if current number of elements
// is equal to the maximum size of JobQueue [JQ],
// false otherwise.
bool jobQueue_is_full(const JobQueue JQ) {
    assert(JQ != NULL);
    assert(JQ->cur_num_elements >= 0);

    return ((JQ->cur_num_elements) == (JQ->max_num_elements));
}

// Returns true if the JobQueue [JQ] is empty,
// false otherwise.
bool jobQueue_is_empty(const JobQueue JQ) {
    assert(JQ != NULL);
    assert(JQ->cur_num_elements >= 0);

    return (JQ->cur_num_elements == 0);
}

// Frees allocated space of a QueueNode [node]
void destroy_node(QueueNode* node) {
    assert(node != NULL);

    destroy_job(node->data);
    free(node);
}

// Enqueues 'new_job' in JobQueue 'JQ'
void jobQueue_enqueue(const JobQueue JQ, Job* new_job) {
    assert(JQ != NULL);
    assert(new_job != NULL);

    if (jobQueue_size(JQ) == JQ->max_num_elements)
        return;

    QueueNode* new_node = create_node();
    new_node->data = new_job; 

    // Queue was not empty previously
    if (!jobQueue_is_empty(JQ))
        JQ->tail->next = new_node;

    JQ->tail = new_node;

    // Queue was empty previously
    if (jobQueue_is_empty(JQ))
        JQ->head = new_node;

    // Pushing at the end was successful, increase the number of elements by 1
    JQ->cur_num_elements++;

}

// Finds and returns the previous node of a node that has id = 'id'.
// Edge cases:
//  A) If the node is not found within the 'JQ', NULL is returned.
//  B) If the 'JQ' is empty or it has only got 1 node, NULL is returned.
//  C) If the node has more than 1 nodes and NULL is returned, it
//     could potentially mean that the node is at the head. Further
//     processing is needed to verify this scenario from caller.
QueueNode* find_prev_node_by_id(const JobQueue JQ, int id) {
    assert(JQ != NULL);

    if (jobQueue_is_empty(JQ) || (jobQueue_size(JQ) == 1) )
        return NULL;

    QueueNode* tmp = JQ->head;

    while(tmp->next != NULL) {
        if (tmp->next->data->id == id)
            return tmp;

        tmp = tmp->next;
    }

    return NULL;
}

// If found within the JobQueue 'JQ', deletes and frees allocated space
// of node with id = 'id', initializes 'pid_removed' with the pid of deleted
// node and finally returns true. If the node is not found within the 'JQ',
// false is returned.
bool jobQueue_remove_by_id(const JobQueue JQ, int id) {
    assert(JQ != NULL);

    if (jobQueue_is_empty(JQ))
        return false;

    QueueNode* prev_node = find_prev_node_by_id(JQ, id);

    // The node wasn't found in the queue at all, or the node is
    // at the head of the queue and the queue either has 1 node
    // or more nodes
    if (prev_node == NULL) {
        // Node wasn't found at all
        if (JQ->head->data->id != id)   
            return false;

        // Queue only has one node
        if(jobQueue_size(JQ) == 1) {
            destroy_node(JQ->head);
            JQ->head = NULL;
            JQ->tail = NULL;

            // Set the number of elements to 0
            JQ->cur_num_elements = 0;
            return true;
        }

        // Queue has more nodes
        if(jobQueue_size(JQ) > 1) {
            QueueNode* node_to_delete = JQ->head;
            JQ->head = JQ->head->next;
            destroy_node(node_to_delete);

            // Decrease the number of elements by 1
            JQ->cur_num_elements--;

            return true;
        }
    }

    // Node is at tail
    if (JQ->tail->data->id == id) {
        prev_node->next = NULL;
        destroy_node(JQ->tail);
        JQ->tail = prev_node;
        JQ->cur_num_elements--;
        return true;
    }

    // General case
    QueueNode* node_to_delete = prev_node->next;
    prev_node->next = prev_node->next->next;
    destroy_node(node_to_delete);

    // Decrease the number of elements by 1
    JQ->cur_num_elements--;

    return true;
}


// Dequeues the head Job of the JobQueue [JQ]
Job* jobQueue_dequeue(const JobQueue JQ) {
    assert(JQ != NULL);

    if (jobQueue_is_empty(JQ))
        return NULL;
    
    QueueNode* tmp_head_ptr = JQ->head;
    Job* data = tmp_head_ptr->data;

    JQ->head = JQ->head->next;
    free(tmp_head_ptr);

    // Decrease the number of elements by 1
    JQ->cur_num_elements--;

    // Queue is empty, reset is needed
    if (jobQueue_is_empty(JQ))
        JQ->tail = NULL;

    // Return the dequeued element
    return data;
}

// Deletes and frees allocated space of the whole JobQueue [JQ]
void jobQueue_destroy(const JobQueue JQ) {
    assert(JQ != NULL);

    if (jobQueue_is_empty(JQ)) {
        free(JQ);
        return;
    }

    QueueNode* tmp = JQ->head;
    QueueNode* to_delete;

    while (tmp->next != NULL) {
        to_delete = tmp;
        tmp = tmp->next;
        destroy_node(to_delete);
    }
    destroy_node(tmp);

    free(JQ);
}

// Creates and returns an array of Job copies to all Job items
// within the JobQueue [JQ]. Also, it initializes the [arr_size]
// variable. 
Job* jobQueue_job_array_create(const JobQueue JQ, int* arr_size) {
    assert(JQ != NULL);

    if (jobQueue_is_empty(JQ)) {
        *arr_size = 0;
        return NULL;
    }

    int size = jobQueue_size(JQ);
    Job* job_array;
    HANDLE_ERROR("malloc", job_array = malloc(size * sizeof(Job)), NULL);

    QueueNode* tmp = JQ->head;
    *arr_size = size;

    for (int i = 0; i < size; i++) {
        memcpy(&(job_array[i]), tmp->data, sizeof(Job));
        tmp = tmp->next;
    }

    return job_array;
}

// Creates and returns an array of Job pointers to all Job items
// within the JobQueue [JQ]. Also, it initializes the [arr_size]
// variable. 
Job** jobQueue_job_ptr_array_create(const JobQueue JQ, int* arr_size) {
    assert(JQ != NULL);

    if (jobQueue_is_empty(JQ)) {
        *arr_size = 0;
        return NULL;
    }
    
    int size = jobQueue_size(JQ);
    Job** job_ptr_array;
    HANDLE_ERROR("malloc", job_ptr_array = malloc(size * sizeof(Job*)), NULL);

    QueueNode* tmp = JQ->head;
    *arr_size = size;

    for(int i = 0; i < size; i++) {
        job_ptr_array[i] = tmp->data;

        tmp = tmp->next;
    }

    return job_ptr_array;
}

void jobQueue_empty_it(const JobQueue JQ) {
    assert(JQ != NULL);

    while(!jobQueue_is_empty(JQ)) {
        Job* job_to_destroy = jobQueue_dequeue(JQ);
        destroy_job(job_to_destroy);
    }

    assert(JQ->cur_num_elements == 0);
}