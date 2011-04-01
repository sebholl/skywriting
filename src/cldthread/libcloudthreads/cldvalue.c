#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "cldvalue.h"

cldvalue *cldvalue_none( void ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = NONE;
    result->value.integer = 0;
    return result;
}

cldvalue *cldvalue_integer( intmax_t const intgr ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = INTEGER;
    result->value.integer = intgr;
    return result;
}

cldvalue *cldvalue_real( long double const dbl ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = REAL;
    result->value.real = dbl;
    return result;
}

cldvalue *cldvalue_string( const char *const str ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = STRING;
    result->value.string = strdup( str );
    return result;
}

cldvalue *cldvalue_ptr( const cldptr ptr ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = CLDPTR;
    result->value.ptr = ptr;
    return result;
}


cJSON *cldvalue_to_json( const cldvalue *const val ){

    cJSON *result = NULL;

    switch(val->type){
        case INTEGER:
            result = cJSON_CreateNumber( val->value.integer );
            break;
        case REAL:
            result = cJSON_CreateNumber( val->value.real );
            break;
        case STRING:
            result = cJSON_CreateString( val->value.string );
            break;
        case CLDPTR:
            result = cldptr_to_json( val->value.ptr );
            break;
        case NONE:
            result = cJSON_CreateNull();
            break;
    }

    return result;

}

cldvalue *cldvalue_from_json( cJSON *const json ){

    cldvalue *result = NULL;

    if( json != NULL ){

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

            case cJSON_Array:
            {
                result->type = CLDPTR;
                result->value.ptr = cldptr_from_json( json );
            }
                break;
            case cJSON_NULL:
                result->type = NONE;
                break;

        }

    }

    return result;

}

void cldvalue_free( cldvalue *const obj ){

    switch(obj->type){
        case STRING:
            free(obj->value.string);
            break;
        default:
            break;
    }

    free(obj);

}

