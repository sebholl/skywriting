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

/* Returning values from cloud apps */
int cldapp_exit( cldvalue *exit_value );

#define cldthread_result_as_fd( thread ) cielID_read_stream( thread )

#define cldthread_result_as_int( thread ) ((int)cldthread_result_as_intmax( thread ))
#define cldthread_result_as_long( thread ) ((long)cldthread_result_as_intmax( thread ))
intmax_t cldthread_result_as_intmax( cldthread *thread );

#define cldthread_result_as_float( thread ) ((float)cldthread_result_as_intmax( thread ))
double cldthread_result_as_double( cldthread *thread );

const char *cldthread_result_as_string( cldthread *thread );

cldptr cldthread_result_as_cldptr( cldthread *thread );

/* Free any memory associated with cldthread
   (including cldthread instance itself).     */

void cldthread_free( cldthread *thread );

