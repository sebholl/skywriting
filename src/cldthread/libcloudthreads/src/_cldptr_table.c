#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../lib/klib/khash.h"

KHASH_MAP_INIT_INT( Int2Ptr, void *)

inline static khash_t(Int2Ptr) *_cldptr_table( void ){

    static khash_t(Int2Ptr) *_table = NULL;

    return (_table != NULL) ? _table : (_table = kh_init( Int2Ptr ));

}

static int _cldptr_hash_heapid( const char *s ){
	int result = *s;
	if (result){
	    for (++s ; *s; ++s) result = (result << 5) - result + *s;
	}
	return result;
}

static cielID *_cldptr_heap_to_cielID( int const heapkey ){

    char *str;

    asprintf( &str, "heap:%d", heapkey );

    return cielID_create2( str );

}

static void _cldptr_table_put( int const key, void *const baseptr ){

    khash_t(Int2Ptr) *const table = _cldptr_table();

    int ret;
    khiter_t i = kh_put( Int2Ptr, table, key, &ret );

    kh_value( table, i ) = baseptr;

}


static void *_cldptr_table_pull( int key ){

    void *result = NULL;
    struct stat buf;

    cielID *id = _cldptr_heap_to_cielID( key );

    int fd = cielID_read_stream( id );

    if( (fd >= 0) && (fstat( fd, &buf ) == 0) ){

        result = mmap( NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );

        _cldptr_table_put( key, result );

    } else {

        if( fd >= 0 ) close(fd);

        fprintf( stderr, "_cldptr_table_put(): cannot retrieve file size for key %d (fd: %d)\n", key, fd );
        exit( EXIT_FAILURE );

    }

    cielID_free( id );

    return result;

}

static void *_cldptr_table_get( int key ){

    khash_t(Int2Ptr) *const table = _cldptr_table();

    khiter_t i = kh_get( Int2Ptr, table, key );

    if( i != kh_end( table ) ){

        #ifdef DEBUG
        fprintf( stderr, "_cldptr_table_get(): retrieving heap offset for %d directly from table\n", key );
        #endif
        return kh_value( _cldptr_table(), i );

    }

    #ifdef DEBUG
    fprintf( stderr, "_cldptr_table_get(): retrieving heap offset for %d directly from cielID\n", key );
    #endif
    return _cldptr_table_pull( key );

}


