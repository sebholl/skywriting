#include "blcr_interface.h"
#include "sw_interface.h"

/* private functions for cldthread.c */

static inline cldthread *_create_cldthread_object(){
    return calloc( 1, sizeof( cldthread ) );
}

static inline void _strip_new_line( char *string ){
    char *tmp;
    if ((tmp = strchr(string, '\n'))!=NULL) *tmp = 0;
}

static void _cldthread_update_env( void ){

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

            _strip_new_line( env_name );
            _strip_new_line( env_value );

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

static void _cldthread_submit_output( cldvalue *value, void *output ){

    swref* ref = swref_create( DATA, sw_get_current_output_id(), value, 0, NULL );

    cJSON *json = swref_serialize( ref );

    char *tmp = cJSON_PrintUnformatted( json );

    cJSON_Delete( json );

    swref_free( ref );

    sw_save_string_to_store( NULL, sw_get_current_output_id(), tmp );

    free( tmp );

}


static int _cldthread_task_checkpoint( const char *resume_task_id,
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

        _cldthread_update_env();

        if( fptr != NULL ){

            _cldthread_submit_output( cldvalue_none(), fptr( fptr_arg ) );
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
