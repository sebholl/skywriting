#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>

#include "sw_interface.h"
#include "swref.h"

swref *swref_create( enum swref_type type, const char *const id, cldvalue *const value, uintmax_t size, const char *const loc_hint ){

    swref *result = calloc( 1, sizeof(swref) );

    result->type = type;
    result->id = cielID_create( id );
    result->size = size;

    if( loc_hint != NULL ){
        result->loc_hints = calloc( 2, sizeof(char *) );
        result->loc_hints[0] = strdup(loc_hint);
        result->loc_hints_size = 1;
    } else {
        result->loc_hints_size = 0;
    }

    result->value = value;

    return result;

}

void swref_fatal_merge( swref *const receiver, swref *const sender ){

    receiver->type = sender->type;
    receiver->size = sender->size;
    receiver->loc_hints = sender->loc_hints;
    receiver->loc_hints_size = sender->loc_hints_size;
    receiver->value = sender->value;

    cielID_free( sender->id );
    free( sender );

}

void swref_free_ex( swref *const ref, int keepcldvalue ){

    if( ref != NULL ){

        if( ref->loc_hints != NULL ){

            char *hint;
            size_t i = 0;

            for( hint = ref->loc_hints[i]; (hint != NULL) && (i < ref->loc_hints_size); hint = ref->loc_hints[++i] ){
                free( hint );
            }

            free(ref->loc_hints);

        }

        if( (!keepcldvalue) && (ref->value != NULL) ){

            cldvalue_free( ref->value );
            ref->value = NULL;

        }

        cielID_free( ref->id );
        free( ref );

    }

}

void swref_free( swref *const ref ){

    swref_free_ex( ref, 0 );

}

cJSON *swref_serialize( const swref *const ref ){

    cJSON *result = NULL;

    if( ref != NULL ){

        cJSON *array = cJSON_CreateArray();

        cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
        cJSON_AddItemToArray( array, cJSON_CreateString(ref->id->id_str) );

        switch( ref->type ){
            case FUTURE:
                break;
            case STREAMING:
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( (const char **)ref->loc_hints, ref->loc_hints_size ) );
                break;
            case DATA:
            {
                const cldvalue* val = ref->value;
                cJSON_AddItemToArray( array, val != NULL ? cldvalue_to_json( val ) : cJSON_CreateNull() );

            }
                break;
            case CONCRETE:
            default:
                cJSON_AddItemToArray( array, cJSON_CreateNumber(ref->size) );
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( (const char **)ref->loc_hints, ref->loc_hints_size ) );
                break;
        }

        if(result==NULL){

            result = cJSON_CreateObject();
            cJSON_AddItemToObject( result, "__ref__", array );

        } else {

            cJSON_Delete( array );

        }

    }

    return result;

}

swref *swref_at_id( cielID *const id ){

    swref* result = NULL;

    size_t size;
    char *dump;

    dump = cielID_dump_stream( id, &size );

    if( dump != NULL && size > 0 ){

        cJSON *json = cJSON_Parse( dump );

        if( json != NULL ){

            result = swref_deserialize( json );
            cJSON_Delete( json );

        }

        free( dump );

    }

    return result;

}

swref * concrete_swref_for_cielID( cielID *const id ){

    swref * result = NULL;

    char *path = sw_generate_block_store_path( id->id_str );

    struct stat buf;

    if( stat( path, &buf ) == 0 ){

        result = swref_create( CONCRETE, id->id_str, NULL, buf.st_size, sw_get_current_worker_loc() );

    }

    free( path );

    return result;

}

swref * future_swref_for_cielID( cielID *const id ){

    return swref_create( FUTURE, id->id_str, NULL, 0, NULL );

}


swref *swref_deserialize( cJSON *json ){

    cJSON *ref;
    swref *result = NULL;

    if( (json != NULL) && ( (ref = cJSON_GetObjectItem(json, "__ref__")) != NULL ) ){

        size_t i;

        cJSON *netlocs;

        char *parse_type;

        result = calloc( 1, sizeof( swref ) );

        parse_type = cJSON_GetArrayItem( ref, 0 )->valuestring;

        for(i = SWREFTYPE_ENUMMIN; i <= SWREFTYPE_ENUMMAX; i++){
            if( strcmp( parse_type, swref_map_type_tuple_id[i] ) == 0 ){
                result->type = i;
                break;
            }
        }

        result->id = cielID_create( cJSON_GetArrayItem( ref, 1 )->valuestring );

        switch(result->type){

            case FUTURE:
                break;

            case STREAMING:
                netlocs = cJSON_GetArrayItem( ref, 2 );
                result->loc_hints_size = cJSON_GetArraySize( netlocs );
                result->loc_hints = calloc( result->loc_hints_size, sizeof( const char * ) );

                for(i = 0; i < result->loc_hints_size; i++){
                    result->loc_hints[i] = strdup(cJSON_GetArrayItem( netlocs, i )->valuestring);
                }

                break;

            case DATA:
                result->value = cldvalue_from_json( cJSON_GetArrayItem( ref, 2 ) );
                break;

            case CONCRETE:
            default:
                result->size = cJSON_GetArrayItem( ref, 2 )->valueint;

                netlocs = cJSON_GetArrayItem( ref, 3 );
                result->loc_hints_size = cJSON_GetArraySize( netlocs );
                result->loc_hints = calloc( result->loc_hints_size, sizeof( const char * ) );

                for(i = 0; i < result->loc_hints_size; i++){
                    result->loc_hints[i] = strdup(cJSON_GetArrayItem( netlocs, i )->valuestring);
                }

                break;

        }

    }

    return result;

}


const cldvalue *swref_to_cldvalue( const swref *const ref ){
    if( ref->type != DATA ){
        fprintf(stderr,"swref %p invalid cast to cldvalue\n", ref);
        exit( EXIT_FAILURE );
    }
    return ref->value;
}

char * swref_to_data( const swref *const ref, size_t *const size_out ){
    if( ref->type != CONCRETE ){
        fprintf(stderr,"swref %p invalid cast to data\n", ref);
        exit( EXIT_FAILURE );
    }
    return cielID_dump_stream( ref->id, size_out );
}


inline cielID * cielID_of_swref( const swref *ref ){
    return ref->id;
}
