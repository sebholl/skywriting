/* included in cldthread.c */

#include "blcr_interface.h"
#include "sw_interface.h"

static inline void strip_new_line( char *string ){
    char *tmp;
    if ((tmp = strchr(string, '\n'))!=NULL) *tmp = 0;
}

static inline cldthread *sw_create_thread_object(){
    return malloc( sizeof( cldthread ) );
}

static void sw_blcr_update_env( void ){

    FILE *named_pipe;
    char *path;
    char env_name[2048], env_value[2048];

    asprintf( &path, "/tmp/%s", sw_get_current_task_id() );

    #if VERBOSE
    printf( "Opening named FIFO \"%s\".\n", path );
    #endif

    named_pipe = fopen( path, "r" );

    if(named_pipe == NULL) exit(-1000);

    while(!feof(named_pipe)){

        if( ( fgets( env_name, 2048, named_pipe ) != NULL) &&
            ( fgets( env_value, 2048, named_pipe ) != NULL)   ) {

            strip_new_line( env_name );
            strip_new_line( env_value );

            #if VERBOSE
            printf( "Setting %s to \"%s\".\n", env_name, env_value );
            #endif

            setenv( env_name, env_value, 1 );

        }

    }

    fclose( named_pipe );
    remove( path );

    free( path );

}

static int sw_blcr_task_checkpoint( const char *resume_task_id,
                                    const char *continuation_task_id,
                                    const char *filepath,
                                    void *(*fptr)(void *),
                                    void *const fptr_arg ){

    int result;

    char *old_task_id;

    old_task_id = strdup( sw_get_current_task_id() ) ;

    sw_set_current_task_id( resume_task_id );

    result = blcr_checkpoint( filepath );

    /* result < 0 iff this execution is resumption of
     * a checkpoint using cr_restart etc.              */
    if( result < 0 ){

        sw_blcr_update_env();

        if( fptr != NULL ){

            cldthread_exit( cldvalue_none() );

            cldthread_submit_output( fptr( fptr_arg ) );

            exit( EXIT_SUCCESS );

        }

    }

    if( (result != 0) && (continuation_task_id != NULL ) ){

        sw_set_current_task_id( continuation_task_id );

    } else {

        sw_set_current_task_id( old_task_id );

    }

    free( old_task_id );

    return result;
}

static int sw_blcr_wait_on_outputs( const swref *output_refs[], size_t id_count ){

    int result;

    char *path;
    char *cont_task_id;

    asprintf( &path, "%s.checkpoint.continuation", sw_get_current_task_id() );

    /* Create a task ID for the new continuation task */
    cont_task_id = sw_get_new_task_id( sw_get_current_task_id(), "cont" );

    result = sw_blcr_task_checkpoint( cont_task_id, NULL, path, NULL, NULL );

    if( result > 0 ){

        int args_size;
        int chkpt_size;

        char *jsonenc_args;
        char *jsonenc_thread_dpnds;
        char *jsonenc_dpnds;

        swref *chkpt_ref;
        swref *args_ref;

        char *tmp;

        {
            struct stat buf;

            if( stat(path, &buf)==-1 ){
                perror( "sw_blcr_flushthreads: cannot retrieve checkpoint filesize" );
                return result;
            }

            chkpt_size = buf.st_size;

        }

        chkpt_ref = sw_move_file_to_worker( NULL, path, NULL );
        tmp = sw_serialize_ref( chkpt_ref );
        sw_free_ref( chkpt_ref );

        args_size = asprintf( &jsonenc_args,  "{\"checkpoint\": %s }", tmp );

        free(tmp);

        args_ref = sw_save_string_to_worker( NULL, NULL, jsonenc_args );
        free( jsonenc_args );

        jsonenc_thread_dpnds = strdup("");

        /* C heap string concatanation for-loop */
        {
            size_t i;
            char *tmp, *serialized_ref;
            for(i = 0; i < id_count; i++ ){
                serialized_ref = sw_serialize_ref( output_refs[i] );
                asprintf( &jsonenc_thread_dpnds, "%s\"thread%d_output\": %s, ",
                          (tmp = jsonenc_thread_dpnds), i, serialized_ref );
                free( serialized_ref );
                free( tmp );
            }
        }

        tmp = sw_serialize_ref( args_ref );
        sw_free_ref( args_ref );

        asprintf( &jsonenc_dpnds, "{%s\"_args\": %s }",
                                  jsonenc_thread_dpnds, tmp );

        free( jsonenc_thread_dpnds );
        free( tmp );

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
        free( jsonenc_dpnds );

    } else if (result < 0) {

        /* Resuming continuation, we need to update ConcreteReferences */
        /* TODO: Do we?
        size_t i;
        swref *deserialized_ref;
        for(i = 0; i < id_count; i++ ){
            deserialized_ref = sw_deserialize_ref( getenv( output_refs[i]->ref_id ) );
            if( deserialized_ref != NULL ) sw_fatal_merge_ref( (swref *)output_refs[i], deserialized_ref );
        }
        */

    } else {

        perror( "Couldn't checkpoint continuation task - we should try and work out a way to recover from this.\n" );

    }

    free( cont_task_id );
    free( path );

    return result;

}
