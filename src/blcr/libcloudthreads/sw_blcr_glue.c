#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>

#include "blcr_interface.h"
#include "sw_interface.h"

int sw_blcr_init( void ){

    return blcr_init_framework() &&
           sw_init();

    return 1;

}

static inline void strip_new_line( char *string ){
    char *tmp;
    if ((tmp = strchr(string, '\n'))) *tmp = 0;
}

void sw_blcr_update_env( void ){

    FILE *named_pipe;
    const char *task_id;
    char *path;
    char env_name[2048], env_value[2048];

    /* Retrieve the current task id (which is actually the parent
     * task id at this point.                                      */
    task_id = sw_get_current_task_id();

    asprintf( &path, "/tmp/%s", task_id );

    printf( "Opening named FIFO \"%s\".\n", path );

    named_pipe = fopen( path, "r" );

    if(named_pipe == NULL) exit(-1000);

    while(!feof(named_pipe)){
        fgets( env_name, 2048, named_pipe );
        strip_new_line( env_name );
        fgets( env_value, 2048, named_pipe );
        strip_new_line( env_value );
        printf( "Setting %s to \"%s\".\n", env_name, env_value );
        setenv( env_name, env_value, 1 );
    }

    fclose( named_pipe );
    remove( path );

    free( path );

}

int sw_blcr_flushthreads( const char *thread_id ){

    int result;

    char *path;

    path = blcr_generate_context_filename();

    result = blcr_checkpoint( path, sw_blcr_update_env, NULL );

    if( result != 0 ){

        int args_size;
        int chkpt_size;

        char *jsonenc_args;
        char *jsonenc_dpnds;
        char *chkpt_file_id;
        char *cont_task_id;
        char *args_id;

        chkpt_file_id = sw_post_file_to_worker( sw_get_current_worker_url(), path );

        {
            struct stat buf;

            if( stat(path, &buf)==-1 ){
                perror( "sw_blcr_flushthreads: cannot retrieve checkpoint filesize" );
                free( chkpt_file_id );
                return result;
            }

            chkpt_size = buf.st_size;

        }

        args_size = asprintf( &jsonenc_args,  "{\"checkpoint\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]},"
                                              " \"old_task_id\": \"%s\"}",
                                              chkpt_file_id, chkpt_size, sw_get_current_worker_url(),
                                              sw_get_current_task_id() );

        free( chkpt_file_id );

        args_id = sw_post_string_to_worker( sw_get_current_worker_url(), jsonenc_args );

        free( jsonenc_args );

        asprintf( &jsonenc_dpnds, "{\"_thread\": {\"__ref__\": [\"f2\", \"%s\"]},"
                                  " \"_args\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]} }",
                                  thread_id,
                                  args_id, args_size, sw_get_current_worker_url() );

        free( args_id );

        /* Create values for the new continuation task */
        cont_task_id = sw_get_new_task_id( sw_get_current_task_id() );

        /* Attempt to POST a new task to CIEL */
        result = sw_spawntask( cont_task_id,
                               sw_get_current_output_id(),
                               sw_get_master_url(),
                               sw_get_current_task_id(),
                               "blcr",
                               jsonenc_dpnds,
                               1 );

        /* If we managed to spawn a new continuation task, then terminate this process */
        if( result ) exit( 20 );

        /* Otherwise, clean-up and return (we can't really do anything else) */
        free( cont_task_id );
        free( jsonenc_dpnds );

    } else if( !result ) {

        perror( "Couldn't checkpoint process so we should perhaps resort to local threading.\n" );

    }

    free( path );

    return result;

}

int sw_blcr_spawnthread( void(*fptr)(void) ){

    int result;

    char *path;

    path = blcr_generate_context_filename();

    result = -1;

    if( blcr_checkpoint( path, sw_blcr_update_env, fptr ) ){

        int args_size;
        int chkpt_size;

        char *chkpt_file_id;
        char *args_id;
        char *jsonenc_dpnds;
        char *jsonenc_args;
        char *thread_task_id;
        char *thread_output_id;

        chkpt_file_id = sw_post_file_to_worker( sw_get_current_worker_url(), path );

        {
            struct stat buf;

            if( stat(path, &buf)==-1 ){
                perror("sw_blcr_spawnthread: cannot retrieve checkpoint filesize");
                return result;
            }

            chkpt_size = buf.st_size;

        }

        args_size = asprintf( &jsonenc_args, "{\"checkpoint\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]},"
                                             " \"old_task_id\": \"%s\"}",
                                             chkpt_file_id, chkpt_size, sw_get_current_worker_url(),
                                             sw_get_current_task_id() );

        free( chkpt_file_id );

        args_id = sw_post_string_to_worker( sw_get_current_worker_url(), jsonenc_args );

        free( jsonenc_args );

        asprintf( &jsonenc_dpnds, "{\"_args\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]} }",
                                  args_id, args_size, sw_get_current_worker_url() );

        free( args_id );

        /* Create values for the new task */
        thread_task_id = sw_get_new_task_id( sw_get_current_task_id() );
        thread_output_id = sw_get_new_output_id( "blcr", thread_task_id );


        result = sw_spawntask( thread_task_id,
                               thread_output_id,
                               sw_get_master_url(),
                               sw_get_current_task_id(),
                               "blcr",
                               jsonenc_dpnds,
                               0 );

        sw_blcr_flushthreads( thread_output_id );

        free( jsonenc_dpnds );
        free( thread_task_id );
        free( thread_output_id );

    } else {

        perror( "Couldn't checkpoint process so we should perhaps resort to local threading.\n" );

    }

    free( path );

    return result;

}
