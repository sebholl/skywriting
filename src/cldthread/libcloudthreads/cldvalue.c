#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "cldthread.h"
#include "cldvalue.h"
#include "sw_interface.h"

cldvalue *cldvalue_none( void ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = NONE;
    result->size = 0;
    result->value.data = NULL;
    return result;
}

cldvalue *cldvalue_integer( intmax_t intgr ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = INTEGER;
    result->value.integer = intgr;
    result->size = 0;
    return result;
}

cldvalue *cldvalue_real( long double dbl ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = REAL;
    result->value.real = dbl;
    result->size = 0;
    return result;
}

cldvalue *cldvalue_string( const char *str ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = STRING;
    result->value.string = strdup( str );
    result->size = 0;
    return result;
}

cldvalue *cldvalue_data( void *data, size_t size ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = BINARY;
    result->size = size;
    result->value.data = memcpy( malloc( size ), &data, size );
    return result;
}

inline intmax_t cldvalue_to_intmax( const cldvalue *obj ){
    if( obj->type != INTEGER ) printf("cldvalue invalid cast");
    return obj->value.integer;
}

inline long cldvalue_to_long( const cldvalue *obj ){
    if( obj->type != INTEGER ) printf("cldvalue invalid cast");
    return (long)obj->value.integer;
}

inline int cldvalue_to_int( const cldvalue *obj ){
    if( obj->type != INTEGER ) printf("cldvalue invalid cast");
    return (int)obj->value.integer;
}

inline double cldvalue_to_double( const cldvalue *obj ){
    if( obj->type != REAL ) printf("cldvalue invalid cast");
    return (double)obj->value.real;
}

inline float cldvalue_to_float( const cldvalue *obj ){
    if( obj->type != REAL ) printf("cldvalue invalid cast");
    return (float)obj->value.real;
}

inline const char * cldvalue_to_string( const cldvalue *obj ){
    if( obj->type != STRING ) printf("cldvalue invalid cast");
    return obj->value.string;
}

inline void * cldvalue_to_data( const cldvalue *obj, size_t *size ){
    if( obj->type != BINARY ) printf("cldvalue invalid cast");
    *size = obj->size;
    return obj->value.data;
}

cldvalue *cldvalue_from_json( cJSON *json ){

    cldvalue *result = NULL;

    if( json != NULL ){

        swref * ref;

        result = calloc( 1, sizeof(cldvalue) );

        switch( json->type ) {

            case cJSON_Number:
            {
                double d = json->valuedouble;
            	if (fabs(((double)json->valueint)-d)<=DBL_EPSILON && d<=INTMAX_MAX && d>=INTMAX_MIN){
                    result->type = INTEGER;
                    result->value.integer = (intmax_t)json->valueint;
            	} else {
            	    result->type = REAL;
            	    result->value.real = d;
            	}
            }
                break;

            case cJSON_String:
                result->type = STRING;
                result->value.string = strdup(json->valuestring);
                break;

            case cJSON_Object:
                result->type = DATA;
                ref = swref_deserialize( json );

                if( ref != NULL ){

                    result->value.data = cldthread_dump_ref( ref, &(result->size) );
                    swref_free( ref );

                }

                break;

            case cJSON_NULL:
                result->type = NONE;
                break;

        }

    }

    return result;

}

void cldvalue_free( cldvalue* obj ){

    switch(obj->type){
        case STRING:
            free(obj->value.string);
            break;
        case BINARY:
            free(obj->value.data);
            break;
        default:
            break;
    }

    free(obj);

}

