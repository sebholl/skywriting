#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <klib/khash.h>

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


static void *_cldptr_table_put( int key ){

    void *result = NULL;
    struct stat buf;

    printf( "Retrieving heap offset for %d from cielID\n", key );

    cielID *id = _cldptr_heap_to_cielID( key );

    int fd = dup( cielID_read_stream( id ) );

    if( (fd >= 0) && (fstat( fd, &buf ) == 0) ){

        result = mmap( NULL, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0 );

        kh_put( Int2Ptr, _cldptr_table(), key, result );

    } else {

        if( fd >= 0 ) close(fd);

        #if DEBUG
        fprintf( stderr, "_cldptr_table_put(): cannot retrieve file size for key %d (fd: %d)\n", key, fd );
        #endif

    }

    cielID_free( id );

    return result;

}

static void *_cldptr_table_get( int key ){

    khash_t(Int2Ptr) *const table = _cldptr_table();

    khiter_t i = kh_get( Int2Ptr, table, key );

    if( i != kh_end( table ) ){

        printf( "Retrieving heap offset for %d directly from table\n", key );
        return kh_value( _cldptr_table(), i );

    }

    return _cldptr_table_put( key );

}


