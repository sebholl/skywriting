#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include <cJSON/cJSON.h>

#include "swref.h"

#include "cldthread.h"

#include "_cldthread.c"

static cldvalue *_result = NULL;

/* API */

int cldthread_init( void ){

    if(_result==NULL) _result = cldvalue_none();
    _cldthread_set_task_id( sw_get_current_task_id() );
    return blcr_init_framework() && sw_init();

}

cldthread *cldthread_smart_create( cldvalue *(*const fptr)(void *), void *const arg0, char *const group_id ){

    char *path;

    char *thread_task_id;
    char *thread_output_id;

    static int _spawn_count = 0;

    cldthread *result;

    result = NULL;

    /* Create values for the new task */

    if(group_id != NULL){

        thread_task_id = sw_generate_task_id( "cldthread", group_id, arg0 );

    } else {

        thread_task_id = sw_generate_task_id( "cldthread", sw_get_current_task_id(), (void *)++_spawn_count );

    }

    thread_output_id = sw_generate_output_id( thread_task_id );

    asprintf( &path, "/tmp/%s.%s.checkpoint.thread", sw_get_current_task_id(), thread_task_id );

    if( _cldthread_task_checkpoint( thread_task_id, NULL, path, fptr, arg0 ) ){

        cJSON *jsonenc_dpnds;
        cJSON *jsonenc_args;
        char *tmp;

        swref *chkpt_ref;
        swref *args_ref;

        /* Post arguments as data in the block store */

        jsonenc_args = cJSON_CreateObject();

        chkpt_ref = sw_move_file_to_store( NULL, path, NULL );

        cJSON_AddItemToObject( jsonenc_args, "checkpoint", swref_serialize( chkpt_ref ) );
        swref_free( chkpt_ref );

        tmp = cJSON_PrintUnformatted( jsonenc_args );
        cJSON_Delete( jsonenc_args );

        args_ref = sw_save_string_to_store( NULL, NULL, tmp );
        free( tmp );

        /* Then create the task */

        jsonenc_dpnds = cJSON_CreateObject();
        cJSON_AddItemToObject( jsonenc_dpnds, "_args", swref_serialize( args_ref ) );
        swref_free( args_ref );

        if( sw_spawntask( thread_task_id,
                          thread_output_id,
                          sw_get_current_task_id(),
                          "cldthread",
                          jsonenc_dpnds,
                          0 )                          ) {

            result = cielID_create( thread_output_id );

        };

        cJSON_Delete( jsonenc_dpnds );


    } else {

        /* There has been an error... */

        perror( "Couldn't checkpoint process so we should perhaps resort to local threading.\n" );

    }

    free( thread_task_id );
    free( thread_output_id );

    free( path );

    return result;

}


size_t cldthread_joins( cielID *id[], size_t const count ){

    size_t i;

    for( i = 0; i < count; i++ ){

        #if DEBUG
        printf("cldthread_joins(): attempting to open stream (%s)\n", id[i]->id_str );
        #endif

        if( cielID_read_stream(id[i]) < 0 ){

            if(_cldthread_continuation_for_inputs( id, count ) >= 0) break;
            i--;

        }

    }

    return i;

}


int cldthread_open_result_as_stream( void ){

    if( _outputstream == NULL ) {

        _outputstream = cielID_create( sw_get_current_output_id() );

    }

    return cielID_publish_stream( _outputstream );

}


swref *cldthread_result_as_ref( cldthread *const thread ){

    swref *ref = swref_at_id( thread );
    cielID_close_stream( thread );

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

