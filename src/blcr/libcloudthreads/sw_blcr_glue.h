#include "sw_blcr_results.h"

typedef struct cldthread {
    char *task_id;
    char *output_id;
    cldthread_result *result;
} cldthread;

int sw_blcr_init( void );
void sw_blcr_submit_output( void *output );

cldthread *sw_blcr_spawnthread( void *(*fptr)(void *), void *arg0 );

int sw_blcr_wait_threads( cldthread *thread[], size_t thread_count );
int sw_blcr_wait_thread( cldthread *thread );
