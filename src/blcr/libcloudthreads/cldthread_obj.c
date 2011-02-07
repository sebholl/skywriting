#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cldthread_obj.h"
#include "sw_interface.h"

cldthread_obj *cldthread_none( void ){
    cldthread_obj *result = malloc( sizeof(cldthread_obj) );
    result->type = NONE;
    result->size = 0;
    result->value.data = NULL;
    return result;
}

cldthread_obj *cldthread_integer( intmax_t intgr ){
    cldthread_obj *result = malloc( sizeof(cldthread_obj) );
    result->type = INTEGER;
    result->value.integer = intgr;
    result->size = 0;
    return result;
}

cldthread_obj *cldthread_real( long double dbl ){
    cldthread_obj *result = malloc( sizeof(cldthread_obj) );
    result->type = REAL;
    result->value.real = dbl;
    result->size = 0;
    return result;
}

cldthread_obj *cldthread_string( const char *str ){
    cldthread_obj *result = malloc( sizeof(cldthread_obj) );
    result->type = STRING;
    result->value.string = strdup( str );
    result->size = 0;
    return result;
}

cldthread_obj *cldthread_data( void *data, size_t size ){
    cldthread_obj *result = malloc( sizeof(cldthread_obj) );
    result->type = BINARY;
    result->size = size;
    result->value.data = memcpy( malloc( size ), &data, size );
    return result;
}

inline intmax_t cldthread_obj_to_intmax( cldthread_obj *obj ){
    if( obj->type != INTEGER ) perror("cldthread_obj invalid cast");
    return obj->value.integer;
}

inline long cldthread_obj_to_long( cldthread_obj *obj ){
    if( obj->type != INTEGER ) perror("cldthread_obj invalid cast");
    return (long)obj->value.integer;
}

inline int cldthread_obj_to_int( cldthread_obj *obj ){
    if( obj->type != INTEGER ) perror("cldthread_obj invalid cast");
    return (int)obj->value.integer;
}

inline double cldthread_obj_to_double( cldthread_obj *obj ){
    if( obj->type != REAL ) perror("cldthread_obj invalid cast");
    return (double)obj->value.real;
}

inline float cldthread_obj_to_float( cldthread_obj *obj ){
    if( obj->type != REAL ) perror("cldthread_obj invalid cast");
    return (float)obj->value.real;
}

inline const char * cldthread_obj_to_string( cldthread_obj *obj ){
    if( obj->type != STRING ) perror("cldthread_obj invalid cast");
    return obj->value.string;
}

inline void * cldthread_obj_to_data( cldthread_obj *obj, size_t *size ){
    if( obj->type != BINARY ) perror("cldthread_obj invalid cast");
    *size = obj->size;
    return obj->value.data;
}

char *cldthread_serialize_obj( cldthread_obj *obj, void *default_value ){

    char *value;
    char *result;

    switch(obj->type){
        case BINARY:
            value = sw_post_data_to_worker( sw_get_current_worker_url(), NULL, obj->value.data, obj->size );
            break;
        case STRING:
            value = sw_post_string_to_worker( sw_get_current_worker_url(), NULL, obj->value.string );
            break;
        case INTEGER:
            asprintf( &value, "%" PRIdMAX, obj->value.integer );
            break;
        case REAL:
            asprintf( &value, "%Lf", (long double)obj->value.real );
            break;
        default:
            if( default_value != NULL ){
                asprintf( &value, "%p", default_value );
            } else {
                value = "NULL";
            }
            break;
    }

    asprintf( &result, "{ \"type\": %d, \"size\": %" PRIdMAX ", \"value\": \"%s\" }", (int)obj->type, (intmax_t)obj->size, value );

    return result;

}

cldthread_obj *cldthread_deserialize_obj( const char *data ){

    char *ref;

    cldthread_obj *result;

    if( data != NULL ){

        int completeness_mask;
        char *str, *str_again, *token, *saveptr;
        char *tmp_value;
        intmax_t tmp_int;

        str = str_again = strdup( data );
        completeness_mask = 0;
        result = malloc( sizeof(cldthread_obj) );

        while(1){

            if( ( token = strtok_r(str, ",", &saveptr) ) == NULL ) break;

            str = NULL;

            printf("::TOKEN: %s\n", token );

            if( sscanf( token, "%*[^\"]\"type\" : %" SCNdMAX, &(tmp_int) ) == 1 ){

                result->type = (enum cldthread_result_type)tmp_int;
                completeness_mask |= 1;

            } else if( sscanf( token, "%*[^\"]\"size\" : %" SCNdMAX, &(tmp_int) ) == 1 ){

                result->size = (size_t)tmp_int;
                completeness_mask |= 2;

            } else if( sscanf( token, "%*[^\"]\"value\" : \"%as", &(tmp_value) ) == 1 ) {

                switch( result->type ) {

                    case INTEGER:

                        if( sscanf( tmp_value, "%"SCNdMAX, &(result->value.integer) ) == 1 )
                            completeness_mask |= 4;
                        break;

                    case REAL:

                        if( sscanf( tmp_value, "%Lf", &(result->value.real) ) == 1 )
                            completeness_mask |= 4;
                        break;

                    case STRING:
                    case BINARY:

                        if( sscanf( tmp_value, "%a[^\"]", &ref ) == 1 ){

                            completeness_mask |= 4;

                            #if VERBOSE
                            printf( "Parsing as string/binary, about to download from \"%s\".\n", ref );
                            #endif

                            result->value.data = sw_get_data_from_store( sw_get_current_worker_url(), ref, &(result->size) );
                            free(ref);

                            if( result->type == STRING ) result->value.string = (char *)result->value.data;

                        }

                        break;

                    default:

                        if( sscanf( tmp_value, "%p", &(result->value.data) ) == 1)
                            completeness_mask |= 4;
                        break;
                }

                free( tmp_value );

            }

        }

        free(str_again);

        if(completeness_mask != (1|2|4)){
            #if VERBOSE
            printf( "!Error while parsing CloudThread result \"%s\" (%d).\n", data, completeness_mask );
            #endif
            free( result );
            result = NULL;
        }

        return result;

    }

    return NULL;

}


