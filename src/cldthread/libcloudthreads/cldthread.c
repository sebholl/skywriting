#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

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

    /*
    if(group_id != NULL){

        cJSON *info = sw_query_info_for_output_id( thread_output_id );

        if( info != NULL ){

            cJSON *task, *task_id, *ref;

            if( ((task = cJSON_GetObjectItem( info, "task" ))!=NULL) &&
                ((task_id = cJSON_GetObjectItem( task, "task_id" ))!=NULL) &&
                ((ref = cJSON_GetObjectItem( info, "ref" ))!=NULL) ){

                result = _create_cldthread_object();
                result->task_id = strdup( task_id->valuestring );
                result->output_ref = swref_deserialize( ref );
                result->result = NULL;

                free( thread_task_id );
                free( thread_output_id );

                return result;

            }

            cJSON_Delete(info);

        }

    }

    */
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

            result = _create_cldthread_object();
            result->task_id = thread_task_id;
            result->output_ref = swref_create( FUTURE, thread_output_id, NULL, 0, sw_get_current_worker_loc() );
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


int cldthread_open_stream( swref *ref ){

    if( (ref->fd) < 0 ){

        ref->fd = sw_open_fd_for_ref( ref->ref_id );

        /*
        if( (ref->fd) < 0 ){

            _cldthread_continuation_for_inputs( &ref, 1 );
            ref->fd = sw_open_fd_for_ref( ref->ref_id );

        }
        */

    }

    return ref->fd;

}


void cldthread_close_stream( swref *ref ){

    if( ref->fd >= 0 ){

        close( ref->fd );
        ref->fd = -1;

    }

}

size_t cldthread_multi_dereference( swref *refs[], size_t const ref_count ){

    size_t i;
    cJSON *json;

    for( i = 0; i < ref_count; i++ ){

        json = _cldthread_dump_ref_as_json( refs[i] );

        cldthread_close_stream( refs[i] );

        if(json != NULL){

            swref *ref = swref_deserialize( json );

            if(ref!=NULL){

                switch(ref->type){

                    case CONCRETE:
                        if(ref->value==NULL) ref->value = cldvalue_from_json( json );
                        break;

                    default:
                        break;

                }

                swref_fatal_merge( refs[i], ref );

            }

            cJSON_Delete( json );

        } else {

            if(_cldthread_continuation_for_inputs( &refs[i], (ref_count - i) ) >= 0) break;
            i--;

        }

    }

    return i;

}


int cldthread_joins( cldthread *thread[], size_t const thread_count ){

    size_t i;

    int result;

    swref **output_refs;

    output_refs = calloc( thread_count, sizeof( swref * ) );

    for( i = 0; i < thread_count; i++ ) output_refs[i] = thread[i]->output_ref;

    result = cldthread_multi_dereference( output_refs, thread_count );

    for( i = 0; i < thread_count; i++ ) thread[i]->result = (cldvalue *)output_refs[i]->value;

    free(output_refs);

    return result;

}


char *cldthread_dump_ref( const swref* const ref, size_t * const size_out ){

    char *result = NULL;

    #if VERBOSE
    printf("Attempting to dump fd (%p)\n", ref );
    #endif

    if( ref->fd >= 0 ){

        long len;
        struct stat info;

        fstat( ref->fd, &info);
        len = info.st_size;

        if( S_ISREG(info.st_mode) ){

            if( (result = malloc(len)) != NULL ){

                if(read(ref->fd, result, len)==len){

                    #if VERBOSE
                    printf( "--> Read %ld bytes directly from block store file\n", len );
                    #endif

                    if( size_out != NULL ) *size_out = (size_t)len;

                } else {

                    #if VERBOSE
                    printf( "--> Fail when attempting to read %ld bytes from block store\n", len );
                    #endif

                    free( result );
                    result = NULL;

                }

            }

        } else if ( S_ISFIFO(info.st_mode) ) {

            #if VERBOSE
            printf( "--> Reading from a FIFO (named pipe)\n" );
            #endif

            struct MemoryStruct mem = { NULL, 0, 0 };
            void *buffer = malloc(4096);
            size_t tmp;

            while( (tmp = read( ref->fd, buffer, 4096 )) ){
                #if VERBOSE
                printf( "--> Read %d byte(s) from FIFO...\n", (int)tmp );
                #endif
                WriteMemoryCallback( buffer, 1, tmp, &mem );
            }

            free( buffer );

            result = mem.memory;
            if( size_out != NULL) *size_out = mem.size;

        } else {

            fprintf( stderr, "ERROR: Unexpected file type (st_mode: %d) whilst attempting to dereference\n", (int)info.st_mode );
            exit( EXIT_FAILURE );

        }

    }

    #if VERBOSE
    printf("::: Ref content result: %p\n", result );
    #endif

    return result;

}


void *cldthread_exit( cldvalue *result ){
    _cldthread_submit_output( result, NULL );
    exit(EXIT_SUCCESS);
    return 0;
}

void cldthread_free( cldthread *const thread ){

    if(thread->task_id!=NULL) free(thread->task_id);
    if(thread->output_ref!=NULL) swref_free((void *)thread->output_ref);
    if(thread->result!=NULL) cldvalue_free(thread->result);

    free(thread);

}

