#define _GNU_SOURCE
#include <stdio.h>

#include "cldptr.h"

#include "_cldptr_table.c"

typedef struct cldptr_header {
    size_t size;
    size_t inuse;
} cldptr_header;

static void *   _heapptr = NULL;
static size_t   _heapoffset = 0;
static size_t   _heapsize = 0;
static int      _heapkey = 0;

void cldptr_reset_heap( const char *const heap_id ){

    if( _heapptr && _heapkey){

        /* Resize heap in case we have a lot of left over space */
        void *tmp;
        if( (tmp = realloc( _heapptr, _heapoffset )) != NULL ) _heapptr = tmp;

        /* Mark the old heap as read-only */
        mprotect( _heapptr, _heapoffset, PROT_READ );

        /* And put its pointer into the hashtable */
        _cldptr_table_put( _heapkey, _heapptr );

    } else {

        free( _heapptr );

    }

    _heapptr = NULL;

    _heapoffset = 0;
    _heapsize = 0;

    _heapkey = _cldptr_hash_heapid( heap_id );

}

cielID *cldptr_heap_cielID( void ){

    return _cldptr_heap_to_cielID( _heapkey );

}

static int _cldptr_heapalloc( size_t size ){

    int offset = 0;
    cldptr_header *header_ptr;

    while( offset < _heapoffset ){

        header_ptr = (cldptr_header *)_heapptr + offset;

        if( header_ptr->inuse == 0 ) {

            if( ( header_ptr->size == size ) ||
                ( header_ptr->size >= ( size + sizeof(cldptr_header) ) ) ){

                offset += sizeof(cldptr_header);

                if( header_ptr->size != size ){

                    cldptr_header *header_ptr2 = (cldptr_header *)_heapptr + offset + size;

                    header_ptr2->inuse = 0;
                    header_ptr2->size = header_ptr->size - ( size + sizeof(cldptr_header) );

                }

                header_ptr->inuse = 1;
                header_ptr->size = size;

                return offset;

            }

        }

        offset += sizeof(cldptr_header) + header_ptr->size;

    }

    size += sizeof( cldptr_header );

    if( size > ( _heapsize - _heapoffset ) ){
        _heapsize += size;
        _heapsize = _heapsize*3/2;
        _heapptr = realloc( _heapptr, _heapsize );
    }

    size -= sizeof( cldptr_header );

    if( _heapptr != NULL ) {

        offset = _heapoffset;

        header_ptr = (cldptr_header *)(_heapptr + offset);

        header_ptr->inuse = 1;
        header_ptr->size = size;

        offset += sizeof( cldptr_header );

        _heapoffset = offset + size;

        return offset;

    }

    return -1;

}

static void _cldptr_heapfree( int offset ){

    cldptr_header *header = _heapptr + offset - sizeof( cldptr_header );

    #ifdef DEBUG
    if( header->inuse == 0 ) fprintf( stderr, "cldptr_heapfree(): double free on offset %d\n", offset );
    #endif

    header->inuse = 0;

}



inline static int _cldptr_current_heap_key( void ){

    return _heapkey;

}

cldptr cldptr_create( const int key, const size_t offset ){

    cldptr result;
    result.key = key;
    result.offset = offset;

    return result;

}

cldptr cldptr_malloc( const size_t size ){

    cldptr result;

    int offset = _cldptr_heapalloc( size );

    if (offset >= 0) result = cldptr_create( _cldptr_current_heap_key(), offset );
    #ifdef DEBUG
    else fprintf( stderr, "cldptr_malloc(): unable to allocate %d byte(s)\n", (int)size );
    #endif

    return result;

}

cldptr cldptr_null( void ){

    return (cldptr){ 0, 0 };

}

int cldptr_is_null( cldptr test ){

    return (test.key == 0 && test.offset == 0);

}

void cldptr_free( cldptr const ptr ){

    if( !cldptr_is_null( ptr ) && (_cldptr_current_heap_key() == ptr.key) )
        _cldptr_heapfree( ptr.offset );
    #ifdef DEBUG
    else
        fprintf( stderr, "shrd_free(): attempt to free a cldptr from another task's heap\n" );
    #endif

}


void *cldptr_deref( cldptr ptr ){

    //printf( "Dereference cldptr {%d, %d}\n", ptr.key, ptr.offset );
    if( _cldptr_current_heap_key() == ptr.key ){

        return ( _heapptr + ptr.offset );

    } else {

        return _cldptr_table_get( ptr.key ) + ptr.offset;

    }


}

const void *cldptr_get_heap( size_t *const size ){

    *size = _heapoffset;
    return _heapptr;

}

cJSON * cldptr_to_json( cldptr const ptr ){

    cJSON *result = cJSON_CreateObject();

    cJSON *array = cJSON_CreateArray();

    cJSON_AddItemToArray( array, cJSON_CreateNumber( ptr.key ) );

    cJSON_AddItemToArray( array, cJSON_CreateNumber( ptr.offset ) );

    cJSON_AddItemToObject( result, "__cldptr__", array);

    return result;

}

cldptr cldptr_from_json( cJSON *const json ){

    cJSON *ref = cJSON_GetObjectItem(json, "__cldptr__");

    if( ref != NULL ){

        return cldptr_create(   (int)cJSON_GetArrayItem( ref, 0 )->valueint,
                             (size_t)cJSON_GetArrayItem( ref, 1 )->valueint  );

    }

    return cldptr_null();

}


