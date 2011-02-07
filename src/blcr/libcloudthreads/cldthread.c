#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "cldthread.h"

#include "sw_blcr_glue.c"

static cldthread_obj *_result = NULL;

/* API */

int cldthread_init( void ){

    if(_result==NULL) _result = cldthread_none();
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

        int args_size;
        int chkpt_size;

        char *chkpt_file_id;
        char *args_id;
        char *jsonenc_dpnds;
        char *jsonenc_args;

        chkpt_file_id = sw_post_file_to_worker( sw_get_current_worker_url(), path );

        {
            struct stat buf;

            if( stat(path, &buf)==-1 ){
                perror("cldthread_create: cannot retrieve checkpoint filesize");
                return result;
            }

            chkpt_size = buf.st_size;

        }

        args_size = asprintf( &jsonenc_args, "{\"checkpoint\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]} }",
                                             chkpt_file_id, chkpt_size, sw_get_current_worker_url() );

        free( chkpt_file_id );

        args_id = sw_post_string_to_worker( sw_get_current_worker_url(), NULL, jsonenc_args );

        free( jsonenc_args );

        asprintf( &jsonenc_dpnds, "{\"_args\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]} }",
                                  args_id, args_size, sw_get_current_worker_url() );

        free( args_id );

        if( sw_spawntask( thread_task_id,
                          thread_output_id,
                          sw_get_master_url(),
                          sw_get_current_task_id(),
                          "blcr",
                          jsonenc_dpnds,
                          0 )                          ) {

            result = sw_create_thread_object();
            result->task_id = thread_task_id;
            result->output_id = thread_output_id;
            result->result = NULL;

        };

        free( jsonenc_dpnds );


    } else {

        free( thread_task_id );
        free( thread_output_id );

        /* There has been an error... */

        perror( "Couldn't checkpoint process so we should perhaps resort to local threading.\n" );

    }

    free( path );

    return result;

}

int cldthread_joins( cldthread *thread[], size_t const thread_count ){

    size_t i;

    int result;

    const char **output_ids;

    output_ids = calloc( thread_count, sizeof( const char * ) );

    for( i = 0; i < thread_count; i++ ) output_ids[i] = thread[i]->output_id;

    result = sw_blcr_wait_on_outputs( output_ids, thread_count );

    free(output_ids);

    for( i = 0; i < thread_count; i++ ){
        thread[i]->result = cldthread_deserialize_obj( sw_get_data_from_store( sw_get_current_worker_url(),
                                                                               thread[i]->output_id,
                                                                               NULL ) );
    }

    return result;

}

inline int cldthread_join( cldthread *thread ){

    return cldthread_joins( &thread, 1 );

}



void cldthread_submit_output( void *output ){

    char *data;

    data = cldthread_serialize_obj( _result, output );

    sw_post_string_to_worker( sw_get_current_worker_url(), sw_get_current_output_id(), data );

    free( data );

}

void *cldthread_exit( cldthread_obj *result ){
    if(_result!=NULL) free(_result);
    _result = result;
    return NULL;
}



