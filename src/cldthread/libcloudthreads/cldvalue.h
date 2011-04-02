#pragma once

#include <stdlib.h>
#include <inttypes.h>

#include <cJSON/cJSON.h>

#include "cldptr.h"

#include "swref.h"

struct swref;

typedef struct cldvalue {

    /* Note: Ensure integrity with cldvalue.c's cldvalue_type_names[] */
    enum { NONE=0, INTEGER, REAL, STRING, CLDPTR, CLDREF, ARRAY } type;

    union {
        intmax_t integer;
        long double real;
        char *string;
        cldptr ptr;
        struct swref *ref;
        struct {
            struct cldvalue **values;
            size_t size;
        } array;
    } value;

} cldvalue;



cldvalue *    cldvalue_none( void );
cldvalue *    cldvalue_integer( intmax_t intgr );
cldvalue *    cldvalue_real( long double dbl );
cldvalue *    cldvalue_string( const char *str );
cldvalue *    cldvalue_ptr( cldptr ptr );
cldvalue *    cldvalue_ref( struct swref *ref );
cldvalue *    cldvalue_array( cldvalue ** values, size_t count );
cldvalue *    cldvalue_vargs( size_t const count, ... );

intmax_t      cldvalue_to_integer( const cldvalue *obj );
#define       cldvalue_to_long( c ) ((long)cldvalue_to_integer( c ))
#define       cldvalue_to_int( c ) ((int)cldvalue_to_integer( c ))
double        cldvalue_to_real( const cldvalue *obj );
#define       cldvalue_to_float( c ) ((float)cldvalue_to_real( c ))
const char *  cldvalue_to_string( const cldvalue *obj );
cldptr        cldvalue_to_cldptr( const cldvalue *obj );
cldvalue **   cldvalue_to_array( const cldvalue * c, size_t * size_out );
const struct swref * cldvalue_to_swref( const cldvalue *obj );

cJSON *       cldvalue_to_json( const cldvalue *val );
cldvalue *    cldvalue_from_json( cJSON *json );

void          cldvalue_free( cldvalue *obj );
