#include "swref.h"
#include "cldvalue.h"

typedef struct cldthread {
    char *task_id;
    const swref *output_ref;
    const cldvalue *result;
} cldthread;

int cldthread_init( void );
void cldthread_submit_output( void *output );

cldthread *cldthread_create( void *(*fptr)(void *), void *arg0 );
void cldthread_free( cldthread *thread );

int cldthread_joins( cldthread *thread[], size_t thread_count );
int cldthread_join( cldthread *thread );
