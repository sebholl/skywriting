#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    if( obj->type != INTEGER ) perror("cldvalue invalid cast");
    return obj->value.integer;
}

inline long cldvalue_to_long( const cldvalue *obj ){
    if( obj->type != INTEGER ) perror("cldvalue invalid cast");
    return (long)obj->value.integer;
}

inline int cldvalue_to_int( const cldvalue *obj ){
    if( obj->type != INTEGER ) perror("cldvalue invalid cast");
    return (int)obj->value.integer;
}

inline double cldvalue_to_double( const cldvalue *obj ){
    if( obj->type != REAL ) perror("cldvalue invalid cast");
    return (double)obj->value.real;
}

inline float cldvalue_to_float( const cldvalue *obj ){
    if( obj->type != REAL ) perror("cldvalue invalid cast");
    return (float)obj->value.real;
}

inline const char * cldvalue_to_string( const cldvalue *obj ){
    if( obj->type != STRING ) perror("cldvalue invalid cast");
    return obj->value.string;
}

inline void * cldvalue_to_data( const cldvalue *obj, size_t *size ){
    if( obj->type != BINARY ) perror("cldvalue invalid cast");
    *size = obj->size;
    return obj->value.data;
}

cJSON *cldvalue_serialize( cldvalue *obj, void *default_value ){

    cJSON *result = cJSON_CreateObject();

    cJSON_AddNumberToObject ( result, "type", obj->type );

    swref *ref = NULL;

    switch(obj->type){
        case INTEGER:
            cJSON_AddNumberToObject( result, "value", obj->value.integer );
            break;
        case REAL:
            cJSON_AddNumberToObject( result, "value", obj->value.real );
            break;
        case STRING:
            cJSON_AddStringToObject( result, "value", obj->value.string );
            break;
        case BINARY:
            ref = sw_save_data_to_store( NULL, NULL, obj->value.data, obj->size );
            cJSON_AddItemToObject( result, "ref", sw_serialize_ref( ref ) );
            cJSON_AddNumberToObject ( result, "size", obj->size );
            break;
        default:
            if( default_value != NULL ){
                cJSON_AddNumberToObject( result, "value", (int)default_value );
            } else {
                cJSON_AddNullToObject( result, "value" );
            }
            break;
    }

    return result;

}

cldvalue *cldvalue_deserialize( cJSON *json ){

    cldvalue *result = NULL;

    if( json != NULL ){

        swref * ref;

        result = calloc( 1, sizeof(cldvalue) );

        result->type = (enum cldthread_result_type)cJSON_GetObjectItem(json,"type")->valueint;

        switch( result->type ) {

            case INTEGER:
                result->value.integer = cJSON_GetObjectItem(json,"value")->valueint;
                break;

            case REAL:
                result->value.integer = cJSON_GetObjectItem(json,"value")->valuedouble;
                break;

            case STRING:
                result->value.string = strdup(cJSON_GetObjectItem(json,"value")->valuestring);

            case DATA:

                result->size = (size_t)cJSON_GetObjectItem(json,"size")->valueint;
                ref = sw_deserialize_ref( cJSON_GetObjectItem(json,"ref") );

                if( ref != NULL ){

                    result->value.data = sw_get_data_from_store( ref, &(result->size) );
                    sw_free_ref( ref );

                }

                break;

            default:

                result->value.data = (void *)(int)cJSON_GetObjectItem(json,"value")->valueint;
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

