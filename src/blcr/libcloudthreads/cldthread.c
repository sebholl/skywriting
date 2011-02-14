#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "cldthread.h"

#include "sw_blcr_glue.c"

static cldvalue *_result = NULL;

/* API */

int cldthread_init( void ){

    if(_result==NULL) _result = cldvalue_none();
    return blcr_init_framework() && sw_init();

}

cldthread *cldthread_create( void *(*fptr)(void *), void *arg0 ){

    char *path;

    char *thread_task_id;
    char *thread_output_id;

    cldthread *result;

    asprintf( &path, "%s.checkpoint.thread", sw_get_current_output_id() );

    result = NULL;

    /* Create values for the new task */
    thread_task_id = sw_get_new_task_id( sw_get_current_task_id(), "thread" );
    thread_output_id = sw_get_new_output_id( "blcr", thread_task_id );

    if( sw_blcr_task_checkpoint( thread_task_id, NULL, path, fptr, arg0 ) ){

        cJSON *jsonenc_dpnds;
        cJSON *jsonenc_args;
        char *tmp;

        swref *chkpt_ref;
        swref *args_ref;

        printf("Hey...\n");

        /* Post arguments as data in the block store */

        jsonenc_args = cJSON_CreateObject();

        printf("Over here...\n");

        chkpt_ref = sw_move_file_to_worker( NULL, path, NULL );

        printf("What you looking at? %p %s\n", chkpt_ref, path);

        cJSON_AddItemToObject( jsonenc_args, "checkpoint", sw_serialize_ref( chkpt_ref ) );
        sw_free_ref( chkpt_ref );

        printf("Up to here...\n");

        tmp = cJSON_PrintUnformatted( jsonenc_args );
        cJSON_Delete( jsonenc_args );

        printf("And then a little bit further\n");

        args_ref = sw_save_string_to_worker( NULL, NULL, tmp );
        free( tmp );

        /* Then create the task */

        jsonenc_dpnds = cJSON_CreateObject();
        cJSON_AddItemToObject( jsonenc_dpnds, "_args", sw_serialize_ref( args_ref ) );
        sw_free_ref( args_ref );

        if( sw_spawntask( thread_task_id,
                          thread_output_id,
                          sw_get_master_url(),
                          sw_get_current_task_id(),
                          "blcr",
                          jsonenc_dpnds,
                          0 )                          ) {

            result = sw_create_thread_object();
            result->task_id = thread_task_id;
            result->output_ref = sw_create_ref( FUTURE, thread_output_id, 0, sw_get_current_worker_url() );
            result->result = NULL;

        };

        cJSON_Delete( jsonenc_dpnds );


    } else {

        free( thread_task_id );
        free( thread_output_id );

        /* There has been an error... */

        perror( "Couldn't checkpoint process so we should perhaps resort to local threading.\n" );

    }

    free( path );

    return result;

}

void cldthread_free( cldthread *const thread ){

    if(thread->task_id!=NULL) free(thread->task_id);
    if(thread->output_ref!=NULL) sw_free_ref((void *)thread->output_ref);

    free(thread);

}

int cldthread_joins( cldthread *thread[], size_t const thread_count ){

    size_t i;

    int result;

    const swref **output_refs;

    cJSON *json;

    output_refs = calloc( thread_count, sizeof( swref * ) );

    for( i = 0; i < thread_count; i++ ) output_refs[i] = thread[i]->output_ref;

    result = sw_blcr_wait_on_outputs( output_refs, thread_count );

    for( i = 0; i < thread_count; i++ ){
        json = sw_get_json_from_store( output_refs[i] );
        thread[i]->result = cldvalue_deserialize( json );
        cJSON_Delete( json );
    }

    free(output_refs);

    return result;

}

inline int cldthread_join( cldthread *thread ){

    return cldthread_joins( &thread, 1 );

}



void cldthread_submit_output( void *output ){

    cJSON *json;
    char *tmp;

    json = cldvalue_serialize( _result, output );
    tmp = cJSON_PrintUnformatted( json );
    cJSON_Delete( json );

    sw_save_string_to_worker( NULL, sw_get_current_output_id(), tmp );

    free( tmp );

}

void *cldthread_exit( cldvalue *result ){
    if(_result!=NULL) free(_result);
    _result = result;
    return NULL;
}



