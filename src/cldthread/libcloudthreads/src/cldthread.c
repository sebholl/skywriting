#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "../lib/cJSON/cJSON.h"

#include "swref.h"
#include "ciel_checkpoint.h"

#include "cldthread.h"

#include "_cldthread.c"

/* API */

int cldthread_init( void ){

    if( sw_init() && blcr_init_framework() ){

        _ciel_set_task_id( cielID_create( sw_get_current_task_id() ) );

        return 1;

    }

    #ifdef DEBUG
    printf( "cldthread_init(): initialisation failed [could perhaps resort to POSIX]\n" );
    #endif

    return 0;

}


cldthread *cldthread_smart_create( char *const group_id, cldvalue *(*const fptr)(void *), void *const arg ){

    cielID *thread_task_id;
    cielID *thread_output_id;

    static int _spawn_count = 0;

    /* Create values for the new task */

    thread_task_id = cielID_create2( (group_id != NULL)
                                     ? sw_generate_task_id( "cldthread", group_id, arg )
                                     : sw_generate_task_id( "cldthread", sw_get_current_task_id(), (void *)++_spawn_count ) );

    thread_output_id = cielID_create2( sw_generate_output_id( thread_task_id->id_str ) );

    int result = _ciel_spawn_chkpt_task( thread_task_id, thread_output_id, NULL, 0, 0 );

    if( result < 0 ){ /* resumed process */

        _cldthread_submit_output( fptr( arg ) );
        exit( EXIT_SUCCESS );

    } else if ( result > 0 ) { /* checkpoint succeeded */

        /* continue on our merry way */

    } else { /* error when attempting to checkpoint */

        fprintf( stderr, "cldthread_smart_create: error while attempting to spawn cloud thread\n" );
        exit( EXIT_FAILURE );

    }

    return thread_output_id;

}

static void *(*_posixfptr)(void *);

static cldvalue *_posixstub( void *arg ){

    void *result = _posixfptr( arg );
    return cldvalue_integer( (intmax_t)((int)result) );

}

cldthread *cldthread_posix_create( void *(*fptr)(void *), void *arg ){

    _posixfptr = fptr;
    return cldthread_create( _posixstub, arg );

}


int cldthread_stream_result( void ){

    if( _outputstream == NULL ) {

        _outputstream = cielID_create( sw_get_current_output_id() );

    }

    return cielID_publish_stream( _outputstream );

}


swref *cldthread_result_as_ref( cldthread *const thread ){

    swref *ref = swref_at_id( thread );
    cielID_close_fd( thread );

    return ref;

}


cldvalue *cldthread_result_as_cldvalue( cldthread *const thread ){

    swref *ref = cldthread_result_as_ref( thread );

    cldvalue *result = (cldvalue *)swref_to_cldvalue( ref );
    swref_free_ex( ref, 1 );

    return result;

}


int cldapp_exit( cldvalue *const result ){
    _cldthread_submit_output( result );
    return 0;
}

