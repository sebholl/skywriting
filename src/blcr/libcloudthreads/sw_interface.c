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

static int spawn_count = 0;

swref *sw_save_string_to_worker( const char *worker_loc, const char *id, const char *str ){

    return sw_save_data_to_worker( worker_loc, id, str, strlen(str) );

}

static char *sw_generate_block_store_path( const char *id ){

    char *result;

    int len;
    const char *separator = "/";
    const char *path = sw_get_block_store_path();

    len = strlen( path );

    if( (len > 0) && (path[len-1] == separator[0]) ) separator = "";

    return (asprintf( &result, "%s%s%s", path, separator, id ) != -1) ? result : NULL;

}

static char *sw_generate_temp_path( const char *id ){

    char *result;

    int len;
    char *tmp;

    len = asprintf( &tmp, "%s.%s", sw_get_current_task_id(), id );

    tmp = sw_sha1_hex_digest_from_bytes( tmp, len, 1 );

    asprintf( &result, "/tmp/cldthead.%s", tmp );

    free( tmp );

    return result;

}

static int sw_move_to_block_store( const char *filepath, const char *id ){

    int result = 0;
    char *bpath = sw_generate_block_store_path( id );

    if(bpath!=NULL){

        result = (rename(filepath, bpath) == 0);
        free(bpath);

    }

    return result;

}

static swref *sw_write_block_store( const char *id, const void *data, size_t size ){

    int proceed;
    char *tmppath = sw_generate_temp_path( id );

    if( tmppath != NULL ){

        FILE *file = fopen( tmppath, "w" );

        if(file != NULL){

            proceed = (fwrite( data, 1, size, file ) == size);
            fclose( file );

            proceed = proceed && sw_move_to_block_store( tmppath, id );

        }

        free(tmppath);

    }

    return (proceed ? sw_create_ref( CONCRETE, id, size, sw_get_current_worker_loc() ) : NULL );

}

static swref *sw_post_data_to_worker( const char *worker_loc, const char *id, const void *data, size_t size ){

    struct MemoryStruct post_data;

    char *post_url;

    CURLcode result;
    CURL *handle;

    struct curl_slist *chunk;

    post_data.memory = (char *)data;
    post_data.size = size;
    post_data.offset = 0;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    curl_easy_setopt( handle, CURLOPT_READDATA, &post_data );
    curl_easy_setopt( handle, CURLOPT_READFUNCTION, ReadMemoryCallback );

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, post_data.size );

    asprintf( &post_url, "http://%s/data/%s/", worker_loc, id );

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

    if( result==CURLE_OK ){

        return sw_create_ref( CONCRETE, id, size, worker_loc );

    } else {

        printf( "sw_post_data_to_worker(): error during upload: %s\n", curl_easy_strerror(result) );
        return NULL;

    }

}

swref *sw_save_data_to_worker( const char *worker_loc, const char *id, const void *data, size_t size ){

    swref *result = NULL;

    char *_id = (id == NULL) ? sw_get_new_task_id( sw_get_current_task_id(), "string" )
                             : (char *)id;

    if( (worker_loc == NULL) || ( strcasecmp( worker_loc, sw_get_current_worker_loc() ) == 0) ){

        if( sw_write_block_store( _id, data, size ) ) {

            result = sw_create_ref( CONCRETE, _id, size, sw_get_current_worker_loc() );

        } else {

            printf( "sw_save_data_to_worker(): could not write data to block_store\n" );

        }

    } else {

        if( sw_post_data_to_worker( worker_loc, _id, data, size ) ) {

            result = sw_create_ref( CONCRETE, _id, size, worker_loc );

        }

    }

    if( result == NULL ){

        if(_id!=id) free( _id );

    }

    return result;

}

swref *sw_create_ref( enum swref_type type, const char *ref_id, uintmax_t size, const char *loc_hint ){

    swref *result = calloc( 1, sizeof(swref) );
    result->loc_hints = calloc( 2, sizeof(char *) );

    result->type = type;
    result->ref_id = strdup(ref_id);
    result->size = size;
    result->loc_hints[0] = strdup(loc_hint);
    result->loc_hints_size = 1;

    return result;

}

void sw_fatal_merge_ref( swref *const receiver, swref *const sender ){

    receiver->type = sender->type;
    receiver->size = sender->size;
    receiver->loc_hints = sender->loc_hints;

    free( (char *)sender->ref_id );
    free( sender );

}

void sw_free_ref( swref *ref ){

    if(ref!=NULL){

        int i = 0;

        const char *hint;
        for( hint = ref->loc_hints[i]; hint != NULL; hint = ref->loc_hints[++i] ){
            free((char *)hint);
        }

        free((char *)ref->ref_id);
        free((swref *)ref);

    }

}

cJSON *sw_serialize_ref( const swref * const ref ){

    cJSON *result = NULL;

    if( ref != NULL ){

        cJSON *array;

        result = cJSON_CreateObject();

        array = cJSON_CreateArray();

        switch( ref->type ){
            case FUTURE:
                cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
                cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );
                break;
            case STREAMING:
                cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
                cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( ref->loc_hints, ref->loc_hints_size ) );
                break;
            case CONCRETE:
            default:
                cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
                cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );
                cJSON_AddItemToArray( array, cJSON_CreateNumber(ref->size) );
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( ref->loc_hints, ref->loc_hints_size ) );
                break;
        }

        cJSON_AddItemToObject( result, "__ref__", array );

    }

    return result;

}

swref *sw_deserialize_ref( cJSON *json ){

    cJSON *ref;
    swref *result = NULL;

    if( (json != NULL) && ( (ref = cJSON_GetObjectItem(json, "__ref__")) != NULL ) ){

        int i;

        cJSON *netlocs;

        char *parse_type;

        result = calloc( 1, sizeof( swref ) );

        parse_type = cJSON_GetArrayItem( ref, 0 )->valuestring;

        for(i = SWREFTYPE_ENUMMIN; i <= SWREFTYPE_ENUMMAX; i++){
            if( strcmp( parse_type, swref_map_type_tuple_id[i] ) == 0 ){
                result->type = i;
                break;
            }
        }

        switch(result->type){

            case FUTURE:
                result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);
                break;

            case STREAMING:
                result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);

                netlocs = cJSON_GetArrayItem( ref, 2 );
                result->loc_hints_size = cJSON_GetArraySize( netlocs );
                result->loc_hints = calloc( result->loc_hints_size, sizeof( const char * ) );

                for(i = 0; i < result->loc_hints_size; i++){
                    result->loc_hints[i] = strdup(cJSON_GetArrayItem( ref, i )->valuestring);
                }

                break;

            case CONCRETE:
            default:

                result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);
                result->size = cJSON_GetArrayItem( ref, 2 )->valueint;

                netlocs = cJSON_GetArrayItem( ref, 3 );
                result->loc_hints_size = cJSON_GetArraySize( netlocs );
                result->loc_hints = calloc( result->loc_hints_size, sizeof( const char * ) );

                for(i = 0; i < result->loc_hints_size; i++){
                    result->loc_hints[i] = strdup(cJSON_GetArrayItem( ref, i )->valuestring);
                }

                break;

        }

    }

    return result;

}



static char *sw_get_data_through_http( const swref *ref, size_t *size_out ){

    char *url;

    CURLcode result;
    CURL *handle;

    struct MemoryStruct data = { NULL, 0, 0 };

    size_t i;
    const char *loc_hint;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    curl_easy_setopt( handle, CURLOPT_WRITEFUNCTION, &WriteMemoryCallback );
    curl_easy_setopt( handle, CURLOPT_WRITEDATA, &data );

    for( loc_hint = ref->loc_hints[0]; loc_hint != NULL; loc_hint = ref->loc_hints[++i] ){

        asprintf( &url, "http://%s/data/%s/", loc_hint, ref->ref_id );

        #if VERBOSE
        printf("Retrieving data from \"%s\".\n", url );
        #endif

        curl_easy_setopt( handle, CURLOPT_URL, url );
        free( url );

        result = curl_easy_perform( handle );

        if(result!=CURLE_OK){

            #if VERBOSE
            printf( "sw_get_data_from_store(): error during download: %s\n", curl_easy_strerror(result) );
            #endif

            if(data.size != 0){
                free(data.memory);
                data.size = 0;
                data.memory = NULL;
            }

            continue;

        }

        break;

    }

    curl_easy_cleanup( handle );

    if( size_out != NULL ) *size_out = data.size;
    return data.memory;

}

cJSON *sw_get_json_from_store( const swref *ref ){

    cJSON *result = NULL;

    char *str = sw_get_data_from_store( ref, NULL );

    if( str != NULL ){

        result = cJSON_Parse( str );
        free(str);

    }

    return result;

}

char *sw_get_data_from_store( const swref *ref, size_t *size_out ){

    char* path = sw_generate_block_store_path( ref->ref_id );
    FILE* fd = fopen( path, "rb" );

    if( fd != NULL ){

        long pos;
        char *bytes;

        fseek(fd, 0, SEEK_END);
        pos = ftell(fd);
        fseek(fd, 0, SEEK_SET);

        bytes = malloc(pos);
        fread(bytes, pos, 1, fd);
        fclose(fd);

        free( path );

        if( size_out != NULL) *size_out = (size_t)pos;
        return bytes;

    }

    free( path );

    return sw_get_data_through_http( ref, size_out );

}

int sw_abort_task( const char *master_loc, const char *task_id ){

    char *post_url;

    CURLcode result;
    CURL *handle;

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    #if VERBOSE
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

static swref *sw_post_file_to_worker( const char *worker_loc, const char *filepath ){

    char *post_url;
    char *id;
    FILE *fd;

    CURLcode result;
    CURL *handle;

    swref *ref;

    struct curl_slist *chunk;

    fd = fopen( filepath, "rb" );

    if( fd == NULL ){
        perror("sw_post_file_to_worker(): cannot open file for binary reading");
        return NULL;
    }

    handle = curl_easy_init();

    curl_easy_setopt( handle, CURLOPT_FAILONERROR, 1 );

    curl_easy_setopt( handle, CURLOPT_POST, 1 );

    #if VERBOSE
    curl_easy_setopt( handle, CURLOPT_VERBOSE, 1 );
    #endif

    struct stat buf;

    if( fstat( fileno(fd), &buf )==-1 ){
        fclose(fd);
        perror("sw_post_file_to_worker(): Error retrieving file properties");
        return NULL;
    }

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, buf.st_size );

    id = sw_get_new_task_id( sw_get_current_task_id(), "file" );

    asprintf( &post_url, "http://%s/data/%s/", worker_loc, id );

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

    ref = sw_create_ref( CONCRETE, id, buf.st_size, worker_loc );
    free( id );

    return ref;

}

swref *sw_move_file_to_worker( const char *worker_url, const char *filepath, const char *id ){

    swref *result = NULL;

    char *_id = (id == NULL) ? sw_get_new_task_id( sw_get_current_task_id(), "string" )
                             : (char *)id;

    if( (worker_url == NULL) || ( strcasecmp( worker_url, sw_get_current_worker_loc() ) == 0) ){

        struct stat buf;

        if( stat( filepath, &buf )==-1 ){
            perror("sw_move_file_to_worker(): Error retrieving file properties");
            return NULL;
        }

        if(sw_move_to_block_store( filepath, _id )){

            result = sw_create_ref( CONCRETE, _id, buf.st_size, sw_get_current_worker_loc() );

        }

    } else {

        result = sw_post_file_to_worker( worker_url, filepath );
        if(result!=NULL) remove( filepath );

    }

    if( result == NULL ){

        if(_id!=id) free( _id );

    }

    return result;

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
cJSON *sw_create_json_task_descriptor( const char *new_task_id,
                                       const char *output_task_id,
                                       const char *current_task_id,
                                       const char *handler,
                                       cJSON *jsonenc_dependencies,
                                       int const is_continuation ){

    cJSON *result = cJSON_CreateObject();

    cJSON_AddStringToObject( result, "task_id", new_task_id );
    cJSON_AddStringToObject( result, "handler", handler );
    cJSON_AddItemReferenceToObject( result, "dependencies", jsonenc_dependencies );
    cJSON_AddItemToObject( result, "expected_outputs", cJSON_CreateStringArray( &output_task_id, 1 ) );
    cJSON_AddStringToObject( result, (is_continuation ? "continues_task" : "parent" ), current_task_id );

    return result;

}


/* C implementation of
 *   src/python/skywriting/runtime/worker/master_proxy.py
 *   spawn_tasks( self, parent_task_id, tasks )
 */
int sw_spawntask( const char *new_task_id,
                  const char *output_task_id,
                  const char *master_loc,
                  const char *parent_task_id,
                  const char *handler,
                  cJSON *jsonenc_dependencies,
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

    asprintf( &post_url, "http://%s/task/%s/spawn", (char *)master_loc, (char *)parent_task_id );
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


inline const char* sw_get_current_worker_loc( void ){
    char *value = (char*)getenv( "SW_WORKER_LOC" );
    return (value != NULL ? value : "localhost:9001" );

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

inline const char* sw_get_master_loc( void ){
    char *value = (char*)getenv( "SW_MASTER_LOC" );
    return (value != NULL ? value : "localhost:9000" );
}

inline const char* sw_get_block_store_path( void ){
    char *value = (char*)getenv( "SW_BLOCK_STORE" );
    return (value != NULL ? value : "storeW1" );
}

