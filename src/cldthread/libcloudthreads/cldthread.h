#pragma once

#include "cielID.h"
#include "cldvalue.h"

/* cldthread struct defn */

typedef cielID cldthread;


/* Initialization */

int cldthread_init( void );
const cldthread *cldthread_current_thread( void );

/* Managing cloud threads */

#define cldthread_create( fptr, arg0 ) cldthread_smart_create( fptr, arg0, NULL )
cldthread *cldthread_smart_create( cldvalue *(*fptr)(void *), void *arg0, char *group_id );

#define cldthread_join( thread ) cldthread_joins( &thread, 1 )
size_t cldthread_joins( cielID *id[], size_t count );

int cldthread_open_result_as_stream( void );
int cldthread_close_result_stream( void );

swref *cldthread_result_as_ref( cldthread *thread );
cldvalue *cldthread_result_as_cldvalue( cldthread *thread );
#define cldthread_result_as_fd( thread ) cielID_read_stream( thread )

/* Returning values from cloud apps */

int cldapp_exit( cldvalue *exit_value );

/* Free any memory associated with cldthread
   (including cldthread instance itself).     */

#define cldthread_free( thread ) cielID_free( thread )

