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

char *cldvalue_serialize( cldvalue *obj, void *default_value ){

    char *result;

    swref *ref = NULL;
    char *value = NULL;

    switch(obj->type){
        case BINARY:
            ref = sw_save_data_to_worker( NULL, NULL, obj->value.data, obj->size );
            value = sw_serialize_ref( ref );
            break;
        case STRING:
            ref = sw_save_string_to_worker( NULL, NULL, obj->value.string );
            value = sw_serialize_ref( ref );
            break;
        case INTEGER:
            asprintf( &value, "\"%" PRIdMAX "\"", obj->value.integer );
            break;
        case REAL:
            asprintf( &value, "\"%Lf\"", (long double)obj->value.real );
            break;
        default:
            if( default_value != NULL ){
                asprintf( &value, "\"%p\"", default_value );
            } else {
                value = "\"NULL\"";
            }
            break;
    }

    asprintf( &result, "{ \"type\": %d, \"size\": %" PRIuMAX ", \"%s\": %s }",
              (int)obj->type, (uintmax_t)obj->size, (ref != NULL ? "ref" : "value"), value );

    if(value!=NULL) free(value);
    if(ref!=NULL) sw_free_ref(ref);

    return result;

}

cldvalue *cldvalue_deserialize( const char *data ){

    cldvalue *result;

    if( data != NULL ){

        int completeness_mask;
        char *str, *str_again, *token, *saveptr;
        char *tmp_value;
        int tmp_i;
        intmax_t tmp_int;
        uintmax_t tmp_uint;

        size_t char_count = 0;

        str = str_again = strdup( data );
        completeness_mask = 0;
        result = malloc( sizeof(cldvalue) );

        while(1){

            if( ( token = strtok_r(str, ",", &saveptr) ) == NULL ) break;

            str = NULL;

            if( sscanf( token, "%*[^\"]\"type\" : %" SCNdMAX, &(tmp_int) ) == 1 ){

                result->type = (enum cldthread_result_type)tmp_int;
                completeness_mask |= 1;

            } else if( sscanf( token, "%*[^\"]\"size\" : %" SCNuMAX, &(tmp_uint) ) == 1 ){

                result->size = (size_t)tmp_uint;
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

                    default:

                        if( sscanf( tmp_value, "%p", &(result->value.data) ) == 1)
                            completeness_mask |= 4;
                        break;
                }

                free( tmp_value );

            } else if( sscanf( token, "%*[^\"]\"ref\" : %n%as", &tmp_i, &tmp_value ) > 0 ) {

                swref *ref;

                free(tmp_value);
                sscanf( &data[char_count+tmp_i], "%a[^}]", &tmp_value );

                ref = sw_deserialize_ref( tmp_value );
                free( tmp_value );

                if( ref != NULL ){

                    result->value.data = sw_get_data_from_store( ref, &(result->size) );

                    sw_free_ref( ref );

                    completeness_mask |= 4;

                }

            }

            char_count += strlen( token )+1;

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

