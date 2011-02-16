#include <stdlib.h>
#include <string.h>

#include "swref.h"

swref *sw_create_ref( enum swref_type type, const char *ref_id, uintmax_t size, const char *loc_hint ){

    swref *result = calloc( 1, sizeof(swref) );
    result->loc_hints = calloc( 2, sizeof(char *) );

    result->type = type;
    result->ref_id = strdup(ref_id);
    result->size = size;
    result->loc_hints[0] = strdup(loc_hint);
    result->loc_hints_size = 1;

    return result;

}

void sw_fatal_merge_ref( swref *const receiver, swref *const sender ){

    receiver->type = sender->type;
    receiver->size = sender->size;
    receiver->loc_hints = sender->loc_hints;

    free( (char *)sender->ref_id );
    free( sender );

}

void sw_free_ref( swref *ref ){

    if(ref!=NULL){

        int i = 0;

        const char *hint;
        for( hint = ref->loc_hints[i]; hint != NULL; hint = ref->loc_hints[++i] ){
            free((char *)hint);
        }

        free((char *)ref->ref_id);
        free((swref *)ref);

    }

}

cJSON *sw_serialize_ref( const swref * const ref ){

    cJSON *result = NULL;

    if( ref != NULL ){

        cJSON *array;

        result = cJSON_CreateObject();

        array = cJSON_CreateArray();

        switch( ref->type ){
            case FUTURE:
                cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
                cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );
                break;
            case STREAMING:
                cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
                cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( ref->loc_hints, ref->loc_hints_size ) );
                break;
            case CONCRETE:
            default:
                cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
                cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );
                cJSON_AddItemToArray( array, cJSON_CreateNumber(ref->size) );
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( ref->loc_hints, ref->loc_hints_size ) );
                break;
        }

        cJSON_AddItemToObject( result, "__ref__", array );

    }

    return result;

}

swref *sw_deserialize_ref( cJSON *json ){

    cJSON *ref;
    swref *result = NULL;

    if( (json != NULL) && ( (ref = cJSON_GetObjectItem(json, "__ref__")) != NULL ) ){

        int i;

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

        switch(result->type){

            case FUTURE:
                result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);
                break;

            case STREAMING:
                result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);

                netlocs = cJSON_GetArrayItem( ref, 2 );
                result->loc_hints_size = cJSON_GetArraySize( netlocs );
                result->loc_hints = calloc( result->loc_hints_size, sizeof( const char * ) );

                for(i = 0; i < result->loc_hints_size; i++){
                    result->loc_hints[i] = strdup(cJSON_GetArrayItem( ref, i )->valuestring);
                }

                break;

            case CONCRETE:
            default:

                result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);
                result->size = cJSON_GetArrayItem( ref, 2 )->valueint;

                netlocs = cJSON_GetArrayItem( ref, 3 );
                result->loc_hints_size = cJSON_GetArraySize( netlocs );
                result->loc_hints = calloc( result->loc_hints_size, sizeof( const char * ) );

                for(i = 0; i < result->loc_hints_size; i++){
                    result->loc_hints[i] = strdup(cJSON_GetArrayItem( ref, i )->valuestring);
                }

                break;

        }

    }

    return result;

}

