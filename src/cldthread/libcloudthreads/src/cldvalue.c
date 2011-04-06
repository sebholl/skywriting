#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#include <math.h>

#include "cldvalue.h"

static const char *cldvalue_type_names[] = { "NONE", "INTEGER", "REAL", "STRING",
                                             "CLDPTR", "CLDREF", "ARRAY" };

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

cldvalue *cldvalue_ref( swref *const ref ){
    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = CLDREF;
    result->value.ref = ref;
    return result;
}

cldvalue *cldvalue_array( cldvalue **const values, size_t const count ){

    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = ARRAY;
    result->value.array.size = count;
    result->value.array.values = calloc( count, sizeof( cldvalue * ) );

    size_t i;
    for( i = 0; i < count; ++i ) result->value.array.values[i] = values[i];

    return result;

}

cldvalue *cldvalue_vargs( size_t const count, ... ){

    va_list argp;
    va_start( argp, count );

    cldvalue *result = malloc( sizeof(cldvalue) );
    result->type = ARRAY;
    result->value.array.size = count;
    result->value.array.values = calloc( count, sizeof( cldvalue * ) );

    size_t i;

    for( i = 0; i < count; ++i )
        result->value.array.values[i] = va_arg( argp, cldvalue * );

    va_end( argp );

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
        case CLDREF:
            result = swref_serialize( val->value.ref );
            break;
        case ARRAY:
        {
            size_t i;
            result = cJSON_CreateArray();
            for( i = 0; i < val->value.array.size; i++ )
                cJSON_AddItemToArray( result, cldvalue_to_json(val->value.array.values[i]) );
        }
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
                size_t i;
                cJSON *iter;

                result->type = ARRAY;
                result->value.array.size = (size_t)cJSON_GetArraySize( json );
                result->value.array.values = calloc( result->value.array.size, sizeof( cldvalue * ) );

                for( iter = json->child, i = 0; iter != NULL; iter = iter->next )
                    result->value.array.values[i++] = cldvalue_from_json( iter );
            }
                break;

            case cJSON_Object:
            {
                if( (result->value.ref = swref_deserialize( json )) != NULL )
                    result->type = CLDREF;
                else {
                    result->value.ptr = cldptr_from_json( json );
                    result->type = CLDPTR;
                }
            }
                break;

            case cJSON_NULL:
                result->type = NONE;
                break;

        }

    }

    return result;

}

#define IMPLEMENT_CAST( TYPEENUM, UNIONFIELD, RETURNTYPE, TONAME ) \
RETURNTYPE cldvalue_to_##TONAME( const cldvalue *const c ){ \
    if( c->type != TYPEENUM ){ \
        fprintf( stderr, "<FATAL ERROR> invalid cldvalue cast from %s to " #TONAME " (%p)\n", cldvalue_type_names[c->type], c ); \
        exit( EXIT_FAILURE ); \
    } \
    return c->value.UNIONFIELD; \
}

IMPLEMENT_CAST( INTEGER, integer, intmax_t, integer )
IMPLEMENT_CAST( REAL, real, double, real )
IMPLEMENT_CAST( STRING, string, const char *, string )
IMPLEMENT_CAST( CLDPTR, ptr, cldptr, cldptr )
IMPLEMENT_CAST( CLDREF, ref, const swref *, swref )


cldvalue **cldvalue_to_array( const cldvalue *const c, size_t *const size_out ){
    if( c->type != ARRAY ){
        fprintf(stderr,"cldvalue %p invalid cast to array\n", c);
        exit( EXIT_FAILURE );
    }
    if(size_out != NULL) *size_out = c->value.array.size;
    return c->value.array.values;
}


void cldvalue_free( cldvalue *const obj ){

    /* Catch endless recursion if pointers are cyclic */
    int oldtype = obj->type;
    obj->type = NONE;

    switch(oldtype){
        case STRING:
            free( obj->value.string );
            break;
        case CLDREF:
            swref_free( obj->value.ref );
            break;
        case ARRAY:
        {
            size_t i;
            for( i = 0; i < obj->value.array.size; ++i )
                cldvalue_free(obj->value.array.values[i]);
            free( obj->value.array.values );
        }
            break;
        default:
            break;
    }

    free(obj);

}

