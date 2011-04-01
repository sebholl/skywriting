#pragma once

#include <stdlib.h>
#include <inttypes.h>

#include <cJSON/cJSON.h>

#include "cldptr.h"

typedef struct cldvalue {

    enum cldvalue_type { NONE=0, INTEGER, REAL, STRING, CLDPTR } type;

    union {
        intmax_t integer;
        long double real;
        char* string;
        cldptr ptr;
    } value;

} cldvalue;

cldvalue *    cldvalue_none( void );
cldvalue *    cldvalue_integer( intmax_t intgr );
cldvalue *    cldvalue_real( long double dbl );
cldvalue *    cldvalue_string( const char *str );
cldvalue *    cldvalue_ptr( cldptr ptr );

/*
intmax_t      cldvalue_to_intmax( const cldvalue *obj );
long          cldvalue_to_long( const cldvalue *obj );
int           cldvalue_to_int( const cldvalue *obj );
double        cldvalue_to_double( const cldvalue *obj );
float         cldvalue_to_float( const cldvalue *obj );
const char *  cldvalue_to_string( const cldvalue *obj );
cldptr        cldvalue_to_ptr( const cldvalue *obj );
*/

cJSON *       cldvalue_to_json( const cldvalue *val );
cldvalue *    cldvalue_from_json( cJSON *json );

void          cldvalue_free( cldvalue *obj );
