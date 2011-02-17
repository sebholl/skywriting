#pragma once

#include "swref.h"
#include "cldvalue.h"

/* cldthread struct defn */

typedef struct cldthread {
    char *task_id;
    swref *output_ref;
    cldvalue *result;
} cldthread;


/* Initialization */

int cldthread_init( void );


/* Managing cloud threads */
#define cldthread_create( fptr, arg0 ) cldthread_smart_create( fptr, arg0, NULL )
cldthread *cldthread_smart_create( void *(*fptr)(void *), void *arg0, char *group_id );

#define cldthread_join( thread ) cldthread_joins( &thread, 1 )
int cldthread_joins( cldthread *thread[], size_t thread_count );


/* Returning values from cloud threads */

void *cldthread_exit( cldvalue *exit_value );
#define cldapp_exit( exit_value ) (int)cldthread_exit( exit_value )


/* Free any memory associated with cldthread
   (including cldthread instance itself).     */

void cldthread_free( cldthread *thread );


