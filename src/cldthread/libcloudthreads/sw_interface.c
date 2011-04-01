#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/curl_helper_functions.h"

#include "sw_interface.h"

#include "_sw_interface.c"

int sw_init( void ){

    return ( curl_global_init( CURL_GLOBAL_ALL ) == 0 );

}


int sw_spawntask( const char *const new_task_id,
                  const char *const output_task_id,
                  const char *const parent_task_id,
                  const char *const handler,
                  cJSON *const jsonenc_dependencies,
                  int const is_continuation ){

    struct MemoryStruct postdata;

    char *post_url;
    char *post_payload;

    CURLcode result;
    CURL *handle;

    cJSON *task_desc;
    cJSON *task_desc_list;

    struct curl_slist *chunk;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    asprintf( &post_url, "http://%s/task/%s/spawn", sw_get_master_loc(), (char *)parent_task_id );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    task_desc = sw_create_json_task_descriptor( new_task_id,
                                                output_task_id,
                                                parent_task_id,
                                                handler,
                                                jsonenc_dependencies,
                                                is_continuation );

    task_desc_list = cJSON_CreateArray();
    cJSON_AddItemToArray( task_desc_list, task_desc );

    post_payload = cJSON_PrintUnformatted( task_desc_list );
    cJSON_Delete( task_desc_list );

    curl_easy_setopt( handle, CURLOPT_POST, 1 );
    curl_easy_setopt( handle, CURLOPT_READFUNCTION, &ReadMemoryCallback );

    postdata.memory = post_payload;
    postdata.size = strlen(post_payload)*sizeof(char);
    postdata.offset = 0;

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, postdata.size );
    curl_easy_setopt( handle, CURLOPT_READDATA, &postdata );

    chunk = NULL;
    chunk = curl_slist_append( chunk, "Content-Type: identity" );
    curl_easy_setopt( handle, CURLOPT_HTTPHEADER, chunk );

    result = curl_easy_perform( handle );

    free( post_payload );

    curl_easy_cleanup( handle );
    curl_slist_free_all( chunk );

    return (result == CURLE_OK);

}

int sw_publish_ref( const char *const master_loc, const char *const task_id, const swref *const ref ){

    char *post_url;

    CURLcode result;
    CURL *handle;
    struct curl_slist *chunk;

    struct MemoryStruct postdata;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    #if VERBOSE
    printf("Attempting to publish ref for task \"%s\".\n", task_id );
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    curl_easy_setopt( handle, CURLOPT_POST, 1 );
    curl_easy_setopt( handle, CURLOPT_READFUNCTION, &ReadMemoryCallback );

    cJSON *json = cJSON_CreateObject();

    cJSON_AddItemToObject( json, ref->id->id_str, swref_serialize( ref ) );

    postdata.memory = cJSON_PrintUnformatted( json );
    postdata.size = strlen(postdata.memory)*sizeof(char);
    postdata.offset = 0;

    cJSON_Delete( json );

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, postdata.size );
    curl_easy_setopt( handle, CURLOPT_READDATA, &postdata );

    chunk = NULL;
    chunk = curl_slist_append( chunk, "Content-Type: identity" );
    curl_easy_setopt( handle, CURLOPT_HTTPHEADER, chunk );

    asprintf( &post_url, "http://%s/task/%s/publish/", master_loc, task_id );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );
    curl_slist_free_all( chunk );

    free( postdata.memory );

    return (result==CURLE_OK);

}

#if ALLOW_NONDETERMINISM

cJSON *sw_query_info_for_output_id( const char *const output_id ){

    cJSON *result = NULL;

    struct MemoryStruct data = { NULL, 0, 0 };

    char *post_url;

    CURLcode response;
    CURL *handle;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, &WriteMemoryCallback );
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, &data );

    asprintf( &post_url, "http://%s/refs/%s/", sw_get_master_loc(), (char *)output_id );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    response = curl_easy_perform( handle );
    curl_easy_cleanup( handle );

    if(response==CURLE_OK) result = cJSON_Parse( data.memory );

    if(data.memory!=NULL) free(data.memory);

    return result;

}



int sw_abort_task( const char *const master_loc, const char *const task_id ){

    char *post_url;

    CURLcode result;
    CURL *handle;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    #if VERBOSE || DEBUG
    printf("Attempting to abort task \"%s\".\n", task_id );
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    asprintf( &post_url, "http://%s/%s/abort/", master_loc, task_id );

    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );

    return (result==CURLE_OK);

}

#endif

char *sw_generate_block_store_path( const char *const id ){

    char *result;

    int len;
    const char *separator = "/";
    const char *path = sw_get_block_store_path();

    len = strlen( path );

    if( (len > 0) && (path[len-1] == separator[0]) ) separator = "";

    return (asprintf( &result, "%s%s%s", path, separator, id ) != -1) ? result : NULL;

}

static char *_sw_get_filename( const char *const id ){

    char *result, *envname;

    asprintf( &envname, "CL_PATH_%s", id );
    result = getenv( envname );
    free( envname );

    return ( result != NULL ) ? strdup( result ) : NULL;

}

int sw_open_fd_for_id( const char *id ){

    int result = -1;
    char *path = _sw_get_filename( id );

    if(path != NULL){

        #if VERBOSE || DEBUG
        printf( "sw_open_fd_for_id(): opening %s from path \"%s\".\n", id, path );
        #endif

        result = open( path, O_RDONLY );
        free( path );

    } else {

        #if DEBUG
        fprintf( stderr, "sw_open_fd_for_id(): no path value for id \"%s\".\n", id );
        #endif

    }

    return result;

}

char *sw_dump_id( const char *const id, size_t *const size_out ){

    char *result = NULL;

    int fd = sw_open_fd_for_id( id );

    #if VERBOSE
    printf("sw_dump_id(): attempting to dump fd (%d) for id (%s)\n", fd, id );
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

            fprintf( stderr, "ERROR: Unexpected file type (st_mode: %d) whilst attempting to dump ID (%s)\n", (int)info.st_mode, id );
            exit( EXIT_FAILURE );

        }

        close( fd );

    }

    #if VERBOSE
    printf("::: ID content result: %p\n", result );
    #endif

    return result;

}



swref *sw_move_file_to_store( const char *const worker_url, const char *const filepath, const char *const id ){

    swref *result = NULL;

    char *_id = (id == NULL) ? sw_generate_new_task_id( "cldthread", sw_get_current_task_id(), "file" ) : (char *)id;

    if( (worker_url == NULL) || ( strcasecmp( worker_url, sw_get_current_worker_loc() ) == 0) ){

        struct stat buf;

        if( stat( filepath, &buf ) == -1 ){

            fprintf( stderr, "sw_move_file_to_worker(): Error retrieving file properties" );

            return NULL;

        }

        if( _sw_move_to_block_store( filepath, _id ) != 0 ){

            result = swref_create( CONCRETE, _id, NULL, buf.st_size, sw_get_current_worker_loc() );

        }

    } else {

        result = _sw_post_file_to_worker( worker_url, filepath );
        if(result!=NULL) remove( filepath );

    }

    if( result == NULL ){

        if(_id!=id) free( _id );

    }

    return result;

}


swref *sw_save_data_to_store( const char *const worker_loc, const char *const id, const void *const data, const size_t size ){

    swref *result = NULL;

    char *_id = (id == NULL) ? sw_generate_new_task_id( "cldthread", sw_get_current_task_id(), "string" )
                             : (char *)id;

    if( (worker_loc == NULL) || ( strcasecmp( worker_loc, sw_get_current_worker_loc() ) == 0) ){

        if( _sw_write_block_store( _id, data, size ) ) {

            result = swref_create( CONCRETE, _id, NULL, size, sw_get_current_worker_loc() );

        } else {

            fprintf( stderr, "sw_save_data_to_worker(): could not write data to block_store\n" );

        }

    } else {

        if( _sw_post_data_to_worker( worker_loc, _id, data, size ) ) {

            result = swref_create( CONCRETE, _id, NULL, size, worker_loc );

        }

    }

    if( result == NULL ){

        if(_id!=id) free( _id );

    }

    return result;

}

char *sw_generate_new_task_id( const char *const handler, const char *const group_id, const char *const desc ){

    static int _count = 0;

    char *str;
    int len;

    len = asprintf( &str, "%s:%s:%d:%s", handler, group_id, ++_count, desc );

    str = sha1_hex_digest_from_bytes( str, len, 1 );

    return str;

}


char *sw_generate_task_id( const char *const handler, const char *const group_id, const void *const unique_id ){

    char *str;
    int len;

    len = asprintf( &str, "%s:%s:%p", handler, group_id, unique_id );

    str = sha1_hex_digest_from_bytes( str, len, 1 );

    return str;

}


char *sw_generate_suffixed_id( const char *const task_id, const char *const suffix ){

    char *result;

    asprintf( &result, "%s:%s", task_id, suffix );

    return result;

}


inline int sw_set_current_task_id( const char *const taskid ){
    return ( setenv( "CL_TASK_ID", taskid, 1 ) == 0 );
}

inline const char* sw_get_current_task_id( void ){
    char *value = (char*)getenv( "CL_TASK_ID" );
    return (value != NULL ? value : "1234" );
}

inline const char* sw_get_current_worker_loc( void ){
    char *value = (char*)getenv( "CL_WORKER_LOC" );
    return (value != NULL ? value : "localhost:9001" );

}

inline const char* sw_get_current_output_id( void ){
    char *value = (char*)getenv( "CL_OUTPUT_ID" );
    return (value != NULL ? value : "4321" );
}

inline const char* sw_get_master_loc( void ){
    char *value = (char*)getenv( "CL_MASTER_LOC" );
    return (value != NULL ? value : "localhost:9000" );
}

inline const char* sw_get_block_store_path( void ){
    char *value = (char*)getenv( "CL_BLOCK_STORE" );
    return (value != NULL ? value : "storeW1" );
}


cJSON *sw_create_json_task_descriptor( const char *const new_task_id,
                                       const char *const output_task_id,
                                       const char *const current_task_id,
                                       const char *const handler,
                                       cJSON *const jsonenc_dependencies,
                                       int const is_continuation ){

    cJSON *result = cJSON_CreateObject();

    cJSON_AddStringToObject( result, "task_id", new_task_id );
    cJSON_AddStringToObject( result, "handler", handler );
    cJSON_AddItemReferenceToObject( result, "dependencies", jsonenc_dependencies );
    cJSON_AddItemToObject( result, "expected_outputs", cJSON_CreateStringArray( (const char **)&output_task_id, 1 ) );
    cJSON_AddStringToObject( result, (is_continuation ? "continues_task" : "parent" ), current_task_id );

    return result;

}


