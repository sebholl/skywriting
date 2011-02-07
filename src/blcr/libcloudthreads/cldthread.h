#include "cldthread_obj.h"

typedef struct cldthread {
    char *task_id;
    char *output_id;
    cldthread_obj *result;
} cldthread;

int cldthread_init( void );
void cldthread_submit_output( void *output );

cldthread *cldthread_create( void *(*fptr)(void *), void *arg0 );

int cldthread_joins( cldthread *thread[], size_t thread_count );
int cldthread_join( cldthread *thread );
