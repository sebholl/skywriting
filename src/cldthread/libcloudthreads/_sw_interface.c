#include <openssl/sha.h>

static char *_sha1_hex_digest_from_bytes( const char *bytes, unsigned int len, int shouldFreeInput ){

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


static char *_sw_generate_block_store_path( const char *id ){

    char *result;

    int len;
    const char *separator = "/";
    const char *path = sw_get_block_store_path();

    len = strlen( path );

    if( (len > 0) && (path[len-1] == separator[0]) ) separator = "";

    return (asprintf( &result, "%s%s%s", path, separator, id ) != -1) ? result : NULL;

}

static char *_sw_generate_temp_path( const char *id ){

    char *result;

    int len;
    char *tmp;

    len = asprintf( &tmp, "%s.%s", sw_get_current_task_id(), id );

    tmp = _sha1_hex_digest_from_bytes( tmp, len, 1 );

    asprintf( &result, "/tmp/cldthread.%s", tmp );

    free( tmp );

    return result;

}


static swref *_sw_post_data_to_worker( const char *worker_loc, const char *id, const void *data, size_t size ){

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

static int _sw_move_to_block_store( const char *filepath, const char *id ){

    int result = 0;
    char *bpath = _sw_generate_block_store_path( id );

    if(bpath!=NULL){

        result = (rename(filepath, bpath) == 0);
        free(bpath);

    }

    return result;

}


static swref *_sw_write_block_store( const char *id, const void *data, size_t size ){

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

    return (proceed ? sw_create_ref( CONCRETE, id, size, sw_get_current_worker_loc() ) : NULL );

}


static char *_sw_get_data_through_http( const swref *ref, size_t *size_out ){

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
            printf( "sw_get_data_through_http(): error during download: %s\n", curl_easy_strerror(result) );
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


static swref *_sw_post_file_to_worker( const char *worker_loc, const char *filepath ){

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

    id = sw_generate_new_task_id( "file" );

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





