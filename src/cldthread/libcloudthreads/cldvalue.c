#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "cldvalue.h"

cldvalue *cldvalue_none( void ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = NONE;
    result->value.integer = 0;
    result->size = 0;
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


cldvalue *cldvalue_from_json( cJSON *json ){

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
        default:
            break;
    }

    free(obj);

}

