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

char *sw_post_file_to_worker( const char *worker_url, const char *filepath ){

    char *post_url;
    char *id;
    FILE *fd;

    CURLcode result;
    CURL *handle;

    fd = fopen( filepath, "rb" );

    if( fd == NULL ){
        perror("sw_post_file_to_worker(): Cannot open file for binary reading");
        return NULL;
    }

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );

    {
        struct stat buf;

        if( fstat( fileno(fd), &buf )==-1 ){
            fclose(fd);
            perror("sw_post_file_to_worker(): Error retrieving file properties");
            return NULL;
        }

        curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, buf.st_size );

    }

    id = sw_get_new_task_id( sw_get_current_task_id() );

    asprintf( &post_url, "%s/data/%s/", worker_url, id );
    printf("Uploading file \"%s\" to \"%s\".\n", filepath, post_url );

    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    curl_easy_setopt( handle, CURLOPT_READDATA, fd );

    //curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, &WriteMemoryCallback );
    //curl_easy_setopt( handle, CURLOPT_WRITEDATA, &url );
    //curl_easy_setopt( handle, CURLOPT_WRITEDATA,  fopen("test.txt","w") );

    struct curl_slist *chunk = NULL;
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

char *sw_sha1_hex_digest_from_bytes( const char *bytes, unsigned int len ){

    char *result;
    int hexlen, i;

    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1( (unsigned char *)bytes, len, hash );

    hexlen = ((SHA_DIGEST_LENGTH << 1) +1 );
    result = malloc( hexlen*sizeof(char) );

    for (i = 0; i < SHA_DIGEST_LENGTH; i++) sprintf( &result[i<<1], "%02x", hash[i]);

    return result;

}

char *sw_get_new_task_id( const char *current_task_id ){

    char *str;
    int len;

    len = asprintf( &str, "%s:%d", current_task_id, ++spawn_count );

    return sw_sha1_hex_digest_from_bytes( str, len );

}

char *sw_get_new_output_id( const char *handler, const char *task_id ){

    char *str;
    int len;

    len = asprintf( &str, "%s:%s", handler, task_id );

    free( str );

    return sw_sha1_hex_digest_from_bytes( str, len );

}



int sw_init( void ){

    return ( curl_global_init( CURL_GLOBAL_ALL ) == 0 );

}

//

// C implementation of
//   src/python/skywriting/runtime/task_executor.py
//   spawn_func( self, spawn_expre, args )
char *sw_create_json_task_descriptor( const char* current_task_id, const char *handler, const char *jsonenc_dependencies ){

    char *jsondesc;
    char *new_task_id;
    char *output_task_id;

    new_task_id = sw_get_new_task_id( current_task_id );
    output_task_id = sw_get_new_output_id( handler, new_task_id );

    asprintf( &jsondesc, "{\"task_id\": \"%s\","
                         " \"handler\": \"%s\","
                         " \"dependencies\": {%s},"
    //                   " \"parent\": \"%s\","
                         " \"expected_outputs\": [\"%s\"] }",

                        new_task_id,
                        handler,
                        jsonenc_dependencies,
   //                   current_task_id,
                        output_task_id );

    free( new_task_id );
    free( output_task_id );

    return jsondesc;

}


// C implementation of
//   src/python/skywriting/runtime/worker/master_proxy.py
//   spawn_tasks( self, parent_task_id, tasks )

int sw_spawntasks( const char *master_url, const char *parent_task_id, const char *handler, const char *dependencies ){

    struct MemoryStruct postdata;

    char *tmp;
    char *post_url;
    char *post_payload;

    CURLcode result;
    CURL *handle;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_POST, 1 );
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );


    asprintf( &post_url, "%s/task/%s/spawn", (char *)master_url, (char *)parent_task_id );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    tmp = sw_create_json_task_descriptor( parent_task_id, handler, dependencies );
    asprintf( &post_payload, "[%s]", (char *)tmp );
    free( tmp );

    //curl_easy_setopt( handle, CURLOPT_POSTFIELDS, post_payload );
    curl_easy_setopt( handle, CURLOPT_POST, 1 );
    curl_easy_setopt( handle, CURLOPT_READFUNCTION, &ReadMemoryCallback );

    postdata.memory = post_payload;
    postdata.size = strlen(post_payload)*sizeof(char);
    postdata.offset = 0;

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, postdata.size );
    curl_easy_setopt( handle, CURLOPT_READDATA, &postdata );

    struct curl_slist *chunk = NULL;
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

inline const char* sw_get_current_task_id( void ){
    char *value = (char*)getenv( "SW_TASK_ID" );
    return (value != NULL ? value : "1234" );
}

inline const char* sw_get_master_url( void ){
    char *value = (char*)getenv( "SW_MASTER_URL" );
    return (value != NULL ? value : "http://localhost:9000" );
}

