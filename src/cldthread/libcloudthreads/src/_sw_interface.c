#include <sys/stat.h>
#include <fcntl.h>

#include "helper/sha.h"

static char *_sw_generate_temp_path( const char *const id ){

    char *result;

    int len;
    char *tmp;

    len = asprintf( &tmp, "%s.%s", sw_get_current_task_id(), id );

    tmp = sha1_hex_digest_from_bytes( tmp, len, 1 );

    ASPRINTF_ORNULL( &result, "/tmp/cldthread.%s", tmp );

    free( tmp );

    return result;

}


static swref *_sw_post_data_to_worker( const char *const worker_loc,
                                       const char *const id,
                                       const void *const data,
                                       size_t const size ){

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

    ASPRINTF_ORDIE( _sw_post_data_to_worker(), &post_url, "http://%s/data/%s/", worker_loc, id );

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

        return swref_create( CONCRETE, id, NULL, size, worker_loc );

    } else {

        printf( "sw_post_data_to_worker(): error during upload: %s\n", curl_easy_strerror(result) );
        return NULL;

    }

}

static int _sw_move_to_block_store( const char *const filepath, const char *const id ){

    int result = 0;
    char *bpath = sw_generate_block_store_path( id );

    if(bpath!=NULL){

        result = (rename(filepath, bpath) == 0);
        free(bpath);

    }

    return result;

}


static swref *_sw_write_block_store( const char *const id, const void *const data, size_t const size ){

    int proceed;
    char *tmppath = _sw_generate_temp_path( id );

    if( tmppath != NULL ){

        FILE *file = fopen( tmppath, "w" );

        if(file != NULL){

            proceed = (fwrite( data, 1, size, file ) == size);
            fclose( file );

            proceed = proceed && _sw_move_to_block_store( tmppath, id );

            remove( tmppath );

        }

        free(tmppath);

    }

    return (proceed ? swref_create( CONCRETE, id, NULL, size, sw_get_current_worker_loc() ) : NULL );

}


static swref *_sw_post_file_to_worker( const char *const worker_loc, const char *const filepath ){

    char *post_url;
    char *id;
    FILE *fp;

    CURLcode result;
    CURL *handle;

    swref *ref;

    struct curl_slist *chunk;

    fp = fopen( filepath, "rb" );

    if( fp == NULL ){
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

    if( fstat( fileno(fp), &buf )==-1 ){
        fclose(fp);
        perror("sw_post_file_to_worker(): Error retrieving file properties");
        return NULL;
    }

    curl_easy_setopt( handle, CURLOPT_POSTFIELDSIZE, buf.st_size );

    id = sw_generate_task_id( "cldthread", sw_get_current_task_id(), "file" );

    ASPRINTF_ORDIE( _sw_post_file_to_worker(), &post_url, "http://%s/data/%s/", worker_loc, id );

    #if VERBOSE
    printf("Uploading file \"%s\" to \"%s\".\n", filepath, post_url );
    #endif

    curl_easy_setopt( handle, CURLOPT_URL, post_url );
    free( post_url );

    curl_easy_setopt( handle, CURLOPT_READDATA, fp );

    chunk = NULL;
    chunk = curl_slist_append( chunk, "Content-Type: identity" );
    curl_easy_setopt( handle, CURLOPT_HTTPHEADER, chunk );

    result = curl_easy_perform( handle );

    curl_easy_cleanup( handle );
    curl_slist_free_all( chunk );

    fclose( fp );

    if( result != CURLE_OK ){

        fprintf( stderr, "sw_post_file_to_worker(): error during upload: %s\n", curl_easy_strerror(result) );
        if( id!=NULL) free( id );
        return NULL;

    }

    ref = swref_create( CONCRETE, id, NULL, buf.st_size, worker_loc );
    free( id );

    return ref;

}
