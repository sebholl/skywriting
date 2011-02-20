#pragma once

#include "cJSON.h"
#include <inttypes.h>

typedef struct cldvalue {

    enum cldthread_result_type { NONE=0, INTEGER, REAL, STRING, BINARY } type;

    union {
        intmax_t integer;
        long double real;
        char* string;
        void* data;
    } value;

    size_t size;

} cldvalue;

cldvalue *cldvalue_none( void );
cldvalue *cldvalue_integer( intmax_t intgr );
cldvalue *cldvalue_real( long double dbl );
cldvalue *cldvalue_string( const char *str );
cldvalue *cldvalue_data( void *data, size_t size );

intmax_t cldvalue_to_intmax( const cldvalue *obj );
long cldvalue_to_long( const cldvalue *obj );
int cldvalue_to_int( const cldvalue *obj );
double cldvalue_to_double( const cldvalue *obj );
float cldvalue_to_float( const cldvalue *obj );
const char *cldvalue_to_string( const cldvalue *obj );
void *cldvalue_to_data( const cldvalue *obj, size_t *size );

cldvalue *cldvalue_from_json( cJSON *json );

void cldvalue_free( cldvalue *obj );
