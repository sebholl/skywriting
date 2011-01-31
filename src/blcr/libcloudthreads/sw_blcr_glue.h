typedef struct cldthread {
    char *task_id;
    char *output_id;
} cldthread;

int sw_blcr_init( void );
cldthread *sw_blcr_spawnthread( void(*fptr)(void *), void *arg0 );

int sw_blcr_wait_threads( cldthread *thread[], size_t thread_count );
int sw_blcr_wait_thread( cldthread *thread );
