#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <openssl/sha.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "sw_interface.h"
#include "curl_helper_functions.h"

int spawn_count = 0;

char *sw_post_string_to_worker( const char *worker_url, const char *id, const char *str ){

    return sw_post_data_to_worker( worker_url, id, str, strlen(str) );

}

char *sw_post_data_to_worker( const char *worker_url, const char *_id, const void *data, size_t size ){

    struct MemoryStruct post_data;

    char *id;
    char *post_url;

    CURLcode result;
    CURL *handle;

    struct curl_slist *chunk;

    post_data.memory = (char *)data;
    post_data.size = size;
    post_data.offset = 0;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    curl_easy_setopt( handle, CURLOPT_READDATA, &post_data );
    curl_easy_setopt( handle, CURLOPT_READFUNCTION, ReadMemoryCallback );

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, post_data.size );

    id = (_id == NULL) ? sw_get_new_task_id( sw_get_current_task_id(), "string" )
                       : (char *)_id;

    asprintf( &post_url, "%s/data/%s/", worker_url, id );

    #if VERBOSE
    printf("Uploading data to \"%s\".\n", post_url );
    #endif

    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    chunk = NULL;
    chunk = curl_slist_append( chunk, "Content-Type: identity" );
    curl_easy_setopt( handle, CURLOPT_HTTPHEADER, chunk );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );
    curl_slist_free_all( chunk );

    if(result!=CURLE_OK){

        printf( "sw_post_data_to_worker(): error during upload: %s\n", curl_easy_strerror(result) );
        if(id!=_id) free( id );
        return NULL;

    }

    return id;

}

char *sw_get_data_from_store( const char *worker_url, const char *id, size_t *size_out ){

    char *url;

    CURLcode result;
    CURL *handle;

    struct MemoryStruct data = { NULL, 0, 0 };

    handle = curl_easy_init();

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    asprintf( &url, "%s/data/%s/", worker_url, id );

    #if VERBOSE
    printf("Retrieving data from \"%s\".\n", url );
    #endif

    curl_easy_setopt( handle, CURLOPT_URL, url );
    free( url );


    curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, &WriteMemoryCallback );
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, &data );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );

    if(result!=CURLE_OK){

        printf( "sw_get_data_from_store(): error during download: %s\n", curl_easy_strerror(result) );
        if(data.size != 0) free(data.memory);
        return NULL;

    }

    if( size_out != NULL ) *size_out = data.size;

    return data.memory;

}

int sw_abort_task( const char *master_url, const char *task_id ){

    char *post_url;

    CURLcode result;
    CURL *handle;

    handle = curl_easy_init();

    #if VERBOSE
    printf("Attempting to abort task \"%s\".\n", task_id );
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    asprintf( &post_url, "%s/%s/abort/", master_url, task_id );

    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );

    return (result==CURLE_OK);

}

char *sw_post_file_to_worker( const char *worker_url, const char *filepath ){

    char *post_url;
    char *id;
    FILE *fd;

    CURLcode result;
    CURL *handle;

    struct curl_slist *chunk;

    fd = fopen( filepath, "rb" );

    if( fd == NULL ){
        perror("sw_post_file_to_worker(): Cannot open file for binary reading");
        return NULL;
    }

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    {
        struct stat buf;

        if( fstat( fileno(fd), &buf )==-1 ){
            fclose(fd);
            perror("sw_post_file_to_worker(): Error retrieving file properties");
            return NULL;
        }

        curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, buf.st_size );

    }

    id = sw_get_new_task_id( sw_get_current_task_id(), "file" );

    asprintf( &post_url, "%s/data/%s/", worker_url, id );

    #if VERBOSE
    printf("Uploading file \"%s\" to \"%s\".\n", filepath, post_url );
    #endif

    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    curl_easy_setopt( handle, CURLOPT_READDATA, fd );

    chunk = NULL;
    chunk = curl_slist_append( chunk, "Content-Type: identity" );
    curl_easy_setopt( handle, CURLOPT_HTTPHEADER, chunk );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );
    curl_slist_free_all( chunk );

    fclose( fd );

    if(result!=CURLE_OK){

        printf( "sw_post_file_to_worker(): error during upload: %s\n", curl_easy_strerror(result) );
        if( id!=NULL) free( id );
        return NULL;

    }

    return id;

}

char *sw_sha1_hex_digest_from_bytes( const char *bytes, unsigned int len, int shouldFreeInput ){

    char *result;
    int hexlen, i;

    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1( (unsigned char *)bytes, len, hash );

    hexlen = ((SHA_DIGEST_LENGTH << 1) +1 );
    result = malloc( hexlen*sizeof(char) );

    for (i = 0; i < SHA_DIGEST_LENGTH; i++) sprintf( &result[i<<1], "%02x", hash[i]);

    if(shouldFreeInput) free( (char *)bytes );

    return result;

}

char *sw_get_new_task_id( const char *current_task_id, const char *task_type ){

    char *str;
    char *result;
    int len;

    len = asprintf( &str, "%s:%d", current_task_id, ++spawn_count );

    str = sw_sha1_hex_digest_from_bytes( str, len, 1 );

    asprintf( &result, "%s:%s", str, task_type );

    free( str );

    return result;

}

char *sw_get_new_output_id( const char *handler, const char *task_id ){

    char *str;
    char *result;
    int len;

    len = asprintf( &str, "%s:%s", handler, task_id );

    str = sw_sha1_hex_digest_from_bytes( str, len, 1 );

    asprintf( &result, "%s:output", str );

    free( str );

    return result;

}



int sw_init( void ){

    return ( curl_global_init( CURL_GLOBAL_ALL ) == 0 );

}

/* C implementation of
 *   src/python/skywriting/runtime/task_executor.py
 *   spawn_func( self, spawn_expre, args )
 */
char *sw_create_json_task_descriptor( const char *new_task_id,
                                      const char *output_task_id,
                                      const char *current_task_id,
                                      const char *handler,
                                      const char *jsonenc_dependencies,
                                      int const is_continuation ){

    char *jsondesc;

    asprintf( &jsondesc, "{\"task_id\": \"%s\","
                         " \"handler\": \"%s\","
                         " \"dependencies\": %s,"
                         " \"expected_outputs\": [\"%s\"],"
                         " \"%s\": \"%s\" }",

                        new_task_id,
                        handler,
                        jsonenc_dependencies,
                        output_task_id,
                        (is_continuation ? "continues_task" : "parent" ), current_task_id );

    return jsondesc;

}


/* C implementation of
 *   src/python/skywriting/runtime/worker/master_proxy.py
 *   spawn_tasks( self, parent_task_id, tasks )
 */
int sw_spawntask( const char *new_task_id,
                  const char *output_task_id,
                  const char *master_url,
                  const char *parent_task_id,
                  const char *handler,
                  const char *jsonenc_dependencies,
                  int const is_continuation ){

    struct MemoryStruct postdata;

    char *tmp;
    char *post_url;
    char *post_payload;

    CURLcode result;
    CURL *handle;

    struct curl_slist *chunk;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    asprintf( &post_url, "%s/task/%s/spawn", (char *)master_url, (char *)parent_task_id );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    tmp = sw_create_json_task_descriptor( new_task_id,
                                          output_task_id,
                                          parent_task_id,
                                          handler,
                                          jsonenc_dependencies,
                                          is_continuation );


    asprintf( &post_payload, "[%s]", (char *)tmp );
    free( tmp );

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

    return (result == 0);

}


inline const char* sw_get_current_worker_url( void ){
    char *value = (char*)getenv( "SW_WORKER_URL" );
    return (value != NULL ? value : "http://localhost:9001" );

}

inline const char* sw_get_current_output_id( void ){
    char *value = (char*)getenv( "SW_OUTPUT_ID" );
    return (value != NULL ? value : "4321" );
}

inline int sw_set_current_task_id( const char *taskid ){
    return ( setenv( "SW_TASK_ID", taskid, 1 ) == 0 );
}

inline const char* sw_get_current_task_id( void ){
    char *value = (char*)getenv( "SW_TASK_ID" );
    return (value != NULL ? value : "1234" );
}

inline const char* sw_get_master_url( void ){
    char *value = (char*)getenv( "SW_MASTER_URL" );
    return (value != NULL ? value : "http://localhost:9000" );
}

