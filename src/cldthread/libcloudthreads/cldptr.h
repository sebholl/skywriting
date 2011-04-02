#pragma once

#include <openssl/sha.h>

#include "cJSON/cJSON.h"

#include "cielID.h"

typedef struct cldptr {

    int key;
    int offset;

} cldptr;

/* API */

cldptr         cldptr_create( int key, size_t offset );
cJSON *        cldptr_to_json( cldptr ptr );
cldptr         cldptr_from_json( cJSON *json );

cldptr         cldptr_malloc( size_t size );
cldptr         cldptr_null( void );
int            cldptr_is_null( cldptr test );
void *         cldptr_deref( cldptr ptr );
void           cldptr_free( cldptr ptr );
const void *   cldptr_get_heap( size_t *size );
void           cldptr_reset_heap( const char * heap_id );
cielID *       cldptr_heap_cielID( void );
