#define _GNU_SOURCE
#include <stdio.h>

#include <stdio.h>
#include <string.h>

#include "cielID.h"
#include "cldptr.h"
#include "sw_interface.h"
#include "blcr_interface.h"

void _ciel_set_task_id( const cielID *const new_id ){

    sw_set_current_task_id( new_id->id_str );

    char *heap_idstr = sw_generate_suffixed_id( new_id->id_str, "heap" );

    cldptr_reset_heap( heap_idstr );

    free( heap_idstr );

}

static inline void _strip_new_line( char *const string ){
    char *tmp;
    if ((tmp = strchr(string, '\n'))!=NULL) *tmp = 0;
}

static void _ciel_update_env( const cielID *const new_task_id, cielID *input_id[], size_t const input_count ){

    FILE *named_pipe;
    char *path, *path2;
    char env_name[2048], env_value[2048];
    size_t i;

    #if DEBUG
    printf( "_ciel_update_env(): setting new task id \"%s\"\n", new_task_id->id_str );
    #endif

    _ciel_set_task_id( new_task_id );

    asprintf( &path, "/tmp/%s", sw_get_current_task_id() );
    asprintf( &path2, "/tmp/%s:sync", sw_get_current_task_id() );

    #if DEBUG
    printf( "_ciel_update_env(): opening named FIFO \"%s\"\n", path );
    #endif

    named_pipe = fopen( path, "r" );

    if(named_pipe == NULL) exit(-1000);

    while(!feof(named_pipe)){

        if( ( fgets( env_name, 2048, named_pipe ) != NULL) &&
            ( fgets( env_value, 2048, named_pipe ) != NULL)   ) {

            _strip_new_line( env_name );
            _strip_new_line( env_value );

            #if debug
            printf( "_ciel_update_env(): setting %s to \"%s\"\n", env_name, env_value );
            #endif

            setenv( env_name, env_value, 1 );

        }

    }

    fclose( named_pipe );
    remove( path );
    free( path );

    if( input_id != NULL ){

        for(i = 0; i < input_count; i++) cielID_open_fd( input_id[i] );

    }

    fclose( fopen( path2, "w" ) );
    free( path2 );

}


int _ciel_spawn_chkpt_task( cielID *new_task_id, cielID *output_task_id,
                            cielID *input_id[], size_t input_count,
                            int is_continuation ){

    int result;

    char *path;

    asprintf( &path, "/tmp/checkpoint.%s", new_task_id->id_str );

    result = blcr_checkpoint( path );

    if( result > 0 ){    /* checkpointing succeeded */

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
        size_t i;

        for(i = 0; i < input_count; i++ ){
            asprintf( &tmp, "input%d", i );
            ref = swref_create( FUTURE, input_id[i]->id_str, NULL, 0, NULL );
            cJSON_AddItemToObject( jsonenc_dpnds, tmp, swref_serialize( ref) );
            swref_free( ref );
            free(tmp);
        }

        /* Attempt to POST a new task to CIEL */
        result = sw_spawntask( new_task_id->id_str,
                               is_continuation ? sw_get_current_output_id() : output_task_id->id_str,
                               sw_get_current_task_id(),
                               "cldthread",
                               jsonenc_dpnds,
                               is_continuation );

        cJSON_Delete( jsonenc_dpnds );

        if( !result ){
            fprintf( stderr, "<FATAL ERROR> unable to spawn task %s\n", new_task_id->id_str );
            exit( EXIT_FAILURE );
        }

        /* If we managed to spawn a new continuation task, then terminate this process */
        if( is_continuation ) exit( 20 );


    } else if (result < 0) { /* resumed checkpoint */

        _ciel_update_env( new_task_id, input_id, input_count );

    } else { /* error while attempting to checkpoint */

        fprintf( stderr, "<FATAL ERROR> unable to checkpoint process\n" );
        exit( EXIT_FAILURE );

    }

    free( path );

    return result;

}

