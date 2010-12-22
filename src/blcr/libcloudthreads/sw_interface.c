#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>
#include <openssl/sha.h>

#include "curl_helper_functions.h"

int spawn_count = 0;

const char *sw_post_file_to_worker( const unsigned char *worker_url, const unsigned char *filepath ){

    unsigned char *post_url;
    FILE* fd;

    CURLcode result;
    CURL *handle;

    struct MemoryStruct id = { malloc(1), 0, 0 };

    fd = fopen( filepath, "rb" );

    handle = curl_easy_init();

    asprintf( &post_url, "%s/data/", worker_url );
    curl_easy_setopt( handle, CURLOPT_POSTFIELDS, "" );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    curl_easy_setopt( handle, CURLOPT_READDATA, fd );

    curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, &WriteMemoryCallback );
    curl_easy_setopt( handle, CURLOPT_WRITEDATA,  &id );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );

    fclose(fd);

    if(result==0) return id.memory;

    return "";

}

unsigned char *sw_sha1_hex_digest_from_bytes( const unsigned char *bytes, unsigned int len ){

    char *result;
    int strlen, i;

    unsigned char hash[SHA_DIGEST_LENGTH];

    SHA1( bytes, len, hash );

    strlen = ((SHA_DIGEST_LENGTH >> 2) +1 );
    result = malloc( strlen*sizeof(char) );

    for (i = 0; i < strlen; i++) sprintf( result[i], "%02x", hash[i]);

    return result;

}

unsigned char *sw_get_new_task_id( const unsigned char *current_task_id ){

    unsigned char *str;
    int len;

    len = asprintf( &str, "%s:%d", current_task_id, ++spawn_count );

    free( str );

    return sw_sha1_hex_digest_from_bytes( str, len );

}

unsigned char *sw_get_new_output_id( const unsigned char *handler, const unsigned char *task_id ){

    unsigned char *str;
    int len;

    len = asprintf( &str, "%s:%s", handler, task_id );

    free( str );

    return sw_sha1_hex_digest_from_bytes( str, len );

}



void sw_init( void ){

    curl_global_init( CURL_GLOBAL_ALL );

}

//

// C implementation of
//   src/python/skywriting/runtime/task_executor.py
//   spawn_func( self, spawn_expre, args )
unsigned char *sw_create_json_task_descriptor( const unsigned char* current_task_id, const unsigned char *handler, const unsigned char *jsonenc_dependencies ){

    unsigned char *jsondesc;
    unsigned char *new_task_id;
    unsigned char *output_task_id;

    new_task_id = sw_get_new_task_id( current_task_id );
    output_task_id = sw_get_new_output_id( handler, new_task_id );

    asprintf( &jsondesc, "{\"task_id\": \"%s\","
                         " \"handler\": \"%s\","
                         " \"dependencies\": {%s},"
                         " \"expected_outputs\": [\"%s\"] }",

                        new_task_id,
                        handler,
                        jsonenc_dependencies,
                        output_task_id );

    free( new_task_id );
    free( output_task_id );

    return jsondesc;

}


// C implementation of
//   src/python/skywriting/runtime/worker/master_proxy.py
//   spawn_tasks( self, parent_task_id, tasks )

int sw_spawntasks( const unsigned char *master_url, const unsigned char *parent_task_id, const unsigned char *handler, const unsigned char *dependencies ){

    struct MemoryStruct postdata;

    unsigned char *tmp;
    unsigned char *post_url;
    unsigned char *post_payload;

    CURLcode result;
    CURL *handle;

    handle = curl_easy_init();

    asprintf( &post_url, "%s/task/%s/spawn", (char *)master_url, (char *)parent_task_id );
    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    tmp = sw_create_json_task_descriptor( parent_task_id, handler, dependencies );
    asprintf( &post_payload, "[%s]", (char *)tmp );
    free( tmp );

    //curl_easy_setopt( handle, CURLOPT_POSTFIELDS, post_payload );
    curl_easy_setopt( handle, CURLOPT_POSTFIELDS, "" );
    curl_easy_setopt( handle, CURLOPT_READFUNCTION, &ReadMemoryCallback );

    postdata.memory = post_payload;
    postdata.size = (strlen(post_payload)+1)*sizeof(char);
    postdata.offset = 0;

    curl_easy_setopt( handle, CURLOPT_READDATA, &postdata );

    free( post_payload );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );

    return (result == 0);

}


inline unsigned char* sw_get_current_worker_url( void ){

    return (unsigned char*)getenv( "SW_WORKER_URL" );

}

inline unsigned char* sw_get_current_task_id( void ){

    return (unsigned char*)getenv( "SW_TASK_ID" );

}

inline unsigned char* sw_get_master_url( void ){

    return (unsigned char*)getenv( "SW_MASTER_URL" );

}

