#include <stdio.h>

#include "curl_helper_functions.h"

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


static cielID **_input_id = NULL;
static size_t _input_id_count;

static void _cldthread_update_env( void ){

    FILE *named_pipe;
    char *path, *path2;
    char env_name[2048], env_value[2048];
    size_t i;

    asprintf( &path, "/tmp/%s", sw_get_current_task_id() );
    asprintf( &path2, "/tmp/%s:sync", sw_get_current_task_id() );

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

    if( _input_id != NULL ){

        for(i = 0; i < _input_id_count; i++) cielID_read_stream( _input_id[i] );

    }

    fclose( fopen( path2, "w" ) );
    free( path2 );

}

static cielID *_outputstream = NULL;

static void _cldthread_submit_output( cldvalue *value, void *output ){

    if ( _outputstream == NULL ){

        swref* data = swref_create( DATA, sw_get_current_output_id(), value, 0, NULL );

        cJSON *json = swref_serialize( data );

        char *tmp = cJSON_PrintUnformatted( json );

        cJSON_Delete( json );

        swref_free( data );

        sw_save_string_to_store( NULL, sw_get_current_output_id(), tmp );

        free( tmp );

    } else {

        cielID_finalize_stream( _outputstream );
        cielID_free( _outputstream );
        _outputstream = NULL;

    }

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

        if( _outputstream != NULL ){
            cielID_free( _outputstream );
            _outputstream = NULL;
        }

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



static int _cldthread_continuation_for_inputs( cielID *input_id[], size_t input_count ){

    int result;
    size_t i;

    char *path;
    char *cont_task_id;

    /* Create a task ID for the new continuation task */
    cont_task_id = sw_generate_new_task_id( "cldthread", sw_get_current_task_id(), "cont" );

    asprintf( &path, "/tmp/%s.checkpoint.continuation", sw_get_current_task_id() );

    _input_id = input_id;
    _input_id_count = input_count;

    result = _cldthread_task_checkpoint( cont_task_id, NULL, path, NULL, NULL );

    _input_id = NULL;
    _input_id_count = 0;

    if( result > 0 ){

        cJSON *jsonenc_args;
        cJSON *jsonenc_dpnds;

        swref *chkpt_ref;
        swref *args_ref;

        char *tmp;

        jsonenc_args = cJSON_CreateObject();

        chkpt_ref = sw_move_file_to_store( NULL, path, NULL );
        cJSON_AddItemToObject( jsonenc_args, "checkpoint", swref_serialize( chkpt_ref ) );
        swref_free( chkpt_ref );

        tmp = cJSON_PrintUnformatted( jsonenc_args );
        cJSON_Delete( jsonenc_args );

        args_ref = sw_save_string_to_store( NULL, NULL, tmp );
        free( tmp );


        jsonenc_dpnds = cJSON_CreateObject();

        cJSON_AddItemToObject( jsonenc_dpnds, "_args", swref_serialize( args_ref ) );
        swref_free( args_ref );

        swref *ref;

        for(i = 0; i < input_count; i++ ){
            asprintf( &tmp, "input%d", i );
            ref = swref_create( FUTURE, input_id[i]->id_str, NULL, 0, NULL );
            cJSON_AddItemToObject( jsonenc_dpnds, tmp, swref_serialize( ref) );
            swref_free( ref );
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

        fprintf( stderr, "Couldn't checkpoint continuation task - we should try and work out a way to recover from this.\n" );

    }

    free( cont_task_id );
    free( path );

    return result;

}

