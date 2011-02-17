#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

#include "cldthread.h"

#include "_cldthread.c"

static cldvalue *_result = NULL;

/* API */

int cldthread_init( void ){

    if(_result==NULL) _result = cldvalue_none();
    return blcr_init_framework() && sw_init();

}

cldthread *cldthread_smart_create( void *(*fptr)(void *), void *arg0, char *group_id ){

    char *path;

    char *thread_task_id;
    char *thread_output_id;

    cldthread *result;

    result = NULL;

    /* Create values for the new task */
    thread_task_id = sw_generate_new_task_id( "thread" );
    thread_output_id = sw_generate_output_id( (group_id != NULL ? group_id : thread_task_id), arg0, "cldthread" );

    if(group_id != NULL){

        cJSON *info = sw_query_info_for_output_id( thread_output_id );

        if( info != NULL ){

            cJSON *task, *task_id, *ref;

            if( ((task = cJSON_GetObjectItem( info, "task" ))!=NULL) &&
                ((task_id = cJSON_GetObjectItem( task, "task_id" ))!=NULL) &&
                ((ref = cJSON_GetObjectItem( info, "ref" ))!=NULL) ){

                result = _create_cldthread_object();
                result->task_id = strdup( task_id->valuestring );
                result->output_ref = sw_deserialize_ref( ref );
                result->result = NULL;

                free( thread_task_id );
                free( thread_output_id );

                return result;

            }

            cJSON_Delete(info);

        }

    }


    asprintf( &path, "%s.checkpoint.thread", thread_task_id );

    if( _cldthread_task_checkpoint( thread_task_id, NULL, path, fptr, arg0 ) ){

        cJSON *jsonenc_dpnds;
        cJSON *jsonenc_args;
        char *tmp;

        swref *chkpt_ref;
        swref *args_ref;

        /* Post arguments as data in the block store */

        jsonenc_args = cJSON_CreateObject();

        chkpt_ref = sw_move_file_to_store( NULL, path, NULL );

        cJSON_AddItemToObject( jsonenc_args, "checkpoint", sw_serialize_ref( chkpt_ref ) );
        sw_free_ref( chkpt_ref );

        tmp = cJSON_PrintUnformatted( jsonenc_args );
        cJSON_Delete( jsonenc_args );

        args_ref = sw_save_string_to_store( NULL, NULL, tmp );
        free( tmp );

        /* Then create the task */

        jsonenc_dpnds = cJSON_CreateObject();
        cJSON_AddItemToObject( jsonenc_dpnds, "_args", sw_serialize_ref( args_ref ) );
        sw_free_ref( args_ref );

        if( sw_spawntask( thread_task_id,
                          thread_output_id,
                          sw_get_current_task_id(),
                          "cldthread",
                          jsonenc_dpnds,
                          0 )                          ) {

            result = _create_cldthread_object();
            result->task_id = thread_task_id;
            result->output_ref = sw_create_ref( FUTURE, thread_output_id, 0, sw_get_current_worker_loc() );
            result->result = NULL;

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

static int _cldthread_wait_on_outputs( const swref *output_refs[], size_t id_count ){

    int result;

    char *path;
    char *cont_task_id;

    /* Create a task ID for the new continuation task */
    cont_task_id = sw_generate_new_task_id( "cont" );

    asprintf( &path, "%s.checkpoint.continuation", sw_get_current_task_id() );

    result = _cldthread_task_checkpoint( cont_task_id, NULL, path, NULL, NULL );

    if( result > 0 ){

        size_t i;

        cJSON *jsonenc_args;
        cJSON *jsonenc_dpnds;

        swref *chkpt_ref;
        swref *args_ref;

        char *tmp;

        jsonenc_args = cJSON_CreateObject();

        chkpt_ref = sw_move_file_to_store( NULL, path, NULL );
        cJSON_AddItemToObject( jsonenc_args, "checkpoint", sw_serialize_ref( chkpt_ref ) );
        sw_free_ref( chkpt_ref );

        tmp = cJSON_PrintUnformatted( jsonenc_args );
        cJSON_Delete( jsonenc_args );

        args_ref = sw_save_string_to_store( NULL, NULL, tmp );
        free( tmp );


        jsonenc_dpnds = cJSON_CreateObject();

        cJSON_AddItemToObject( jsonenc_dpnds, "_args", sw_serialize_ref( args_ref ) );
        sw_free_ref( args_ref );

        for(i = 0; i < id_count; i++ ){
            asprintf( &tmp, "thread%d_output", i );
            cJSON_AddItemToObject( jsonenc_dpnds, tmp, sw_serialize_ref( output_refs[i] ) );
            free(tmp);
        }


        /* Attempt to POST a new task to CIEL */
        result = sw_spawntask( cont_task_id,
                               sw_get_current_output_id(),
                               sw_get_current_task_id(),
                               "cldthread",
                               jsonenc_dpnds,
                               1 );

        cJSON_Delete( jsonenc_dpnds );

        /* If we managed to spawn a new continuation task, then terminate this process */
        if( result ) exit( 20 );

        /* Otherwise, clean-up and return (we can't really do anything else) */
        free( tmp );

    } else if (result < 0) {



    } else {

        perror( "Couldn't checkpoint continuation task - we should try and work out a way to recover from this.\n" );

    }

    free( cont_task_id );
    free( path );

    return result;

}


int cldthread_joins( cldthread *thread[], size_t const thread_count ){

    size_t i;

    int result;

    const swref **output_refs;

    cJSON *json;

    output_refs = calloc( thread_count, sizeof( swref * ) );

    for( i = 0; i < thread_count; i++ ) output_refs[i] = thread[i]->output_ref;

    result = _cldthread_wait_on_outputs( output_refs, thread_count );

    for( i = 0; i < thread_count; i++ ){
        json = sw_get_json_from_store( output_refs[i] );
        thread[i]->result = cldvalue_deserialize( json );
        cJSON_Delete( json );
    }

    free(output_refs);

    return result;

}

void *cldthread_exit( cldvalue *result ){
    _cldthread_submit_output( result, NULL );
    exit(EXIT_SUCCESS);
    return 0;
}

void cldthread_free( cldthread *const thread ){

    if(thread->task_id!=NULL) free(thread->task_id);
    if(thread->output_ref!=NULL) sw_free_ref((void *)thread->output_ref);
    if(thread->result!=NULL) cldvalue_free(thread->result);

    free(thread);

}

