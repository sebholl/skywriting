#define _GNU_SOURCE
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "curl_helper_functions.h"
#include "sw_interface.h"
#include "swref.h"

#include "cielID.h"


cielID *cielID_create( const char *id_str ){

    cielID *id = calloc( 1, sizeof( cielID ) );
    id->id_str = strdup(id_str);
    id->fd = -1;

    return id;

}

int cielID_read_stream( cielID *id ){

    if(id->fd < 0) id->fd = sw_open_fd_for_id( id->id_str );

    return id->fd;

}


void cielID_close_stream( cielID *id ){

    if(id->fd >= 0){

        close( id->fd );
        id->fd = 0;

    }

}

void cielID_free( cielID *id ){

    cielID_close_stream( id );
    free( id->id_str );
    free( id );

}


char *cielID_dump_stream( cielID *id, size_t * const size_out ){

    char *result = NULL;

    int fd = cielID_read_stream( id );

    #if VERBOSE
    printf("Attempting to dump fd (%d) for id (%s)\n", fd, id->id_str );
    #endif

    if( fd >= 0 ){

        long len;
        struct stat info;

        fstat( fd, &info);
        len = info.st_size;

        if( S_ISREG(info.st_mode) ){

            if( (result = malloc(len)) != NULL ){

                if(read(fd, result, len)==len){

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

            while( (tmp = read( fd, buffer, 4096 )) ){
                #if VERBOSE
                printf( "--> Read %d byte(s) from FIFO...\n", (int)tmp );
                #endif
                WriteMemoryCallback( buffer, 1, tmp, &mem );
            }

            free( buffer );

            result = mem.memory;
            if( size_out != NULL) *size_out = mem.size;

        } else {

            fprintf( stderr, "ERROR: Unexpected file type (st_mode: %d) whilst attempting to dump ID (%s)\n", (int)info.st_mode, id->id_str );
            exit( EXIT_FAILURE );

        }

        close( fd );

    }



    #if VERBOSE
    printf("::: ID content result: %p\n", result );
    #endif

    return result;

}

int cielID_publish_stream( cielID *id ){

    if( id->fd < 0 ){

        char *streamfilename, *streamfilepath;

        asprintf( &streamfilename, ".%s", id->id_str );

        streamfilepath = sw_generate_block_store_path( streamfilename );

        free( streamfilename );

        id->fd = open( streamfilepath, O_CREAT|O_WRONLY );

        free( streamfilepath );

        if(id->fd >= 0){

            swref* ref = swref_create( STREAMING, id->id_str, NULL, 0, sw_get_current_worker_loc() );

            if( !sw_publish_ref( sw_get_master_loc(), sw_get_current_task_id(), ref ) ){

                close( id->fd );
                id->fd = -1;

            }

            free( ref );

        }

    }

    return id->fd;

}


int cielID_finalize_stream( cielID *id ){

    int result = 0;

    if( id->fd >= 0 ){

        char *streamfilename, *streamfilepath, *concretefilepath;

        asprintf( &streamfilename, ".%s", id->id_str );

        streamfilepath = sw_generate_block_store_path( streamfilename );
        free( streamfilename );

        concretefilepath = sw_generate_block_store_path( id->id_str );

        close( id->fd );
        id->fd = -1;

        if( !(result = ( rename( streamfilepath, concretefilepath ) == 0 )) ){

            perror( "Unable to finalize stream - block-store renaming failed.\n" );
            fprintf( stderr, "FROM: %s\nTO: %s\n", streamfilepath, concretefilepath );

        } else {

            symlink( concretefilepath, streamfilepath );

        }

        free( streamfilepath );
        free( concretefilepath );

    }

    return result;

}

