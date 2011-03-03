#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "sw_interface.h"
#include "swref.h"

swref *swref_create( enum swref_type type, const char *ref_id, const cldvalue *value, uintmax_t size, const char *loc_hint ){

    swref *result = calloc( 1, sizeof(swref) );

    result->type = type;
    result->ref_id = strdup(ref_id);
    result->size = size;
    result->fd = -1;

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

    free( (char *)sender->ref_id );
    free( sender );

}

void swref_free( swref *ref ){

    if(ref!=NULL){

        if(ref->loc_hints != NULL){
            int i = 0;

            const char *hint;
            for( hint = ref->loc_hints[i]; (hint != NULL) && (i < ref->loc_hints_size); hint = ref->loc_hints[++i] ){
                free((char *)hint);
            }

            free(ref->loc_hints);

        }

        free((char *)ref->ref_id);
        free((swref *)ref);

    }

}

cJSON *swref_serialize( const swref * const ref ){

    cJSON *result = NULL;

    if( ref != NULL ){

        cJSON *array = cJSON_CreateArray();

        cJSON_AddItemToArray( array, cJSON_CreateString(swref_map_type_tuple_id[ref->type]) );
        cJSON_AddItemToArray( array, cJSON_CreateString(ref->ref_id) );

        switch( ref->type ){
            case FUTURE:
                break;
            case STREAMING:
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( ref->loc_hints, ref->loc_hints_size ) );
                break;
            case DATA:
            {
                const cldvalue* val = ref->value;
                if(val!=NULL){

                    switch(val->type){
                        case INTEGER:
                            cJSON_AddItemToArray( array, cJSON_CreateNumber( val->value.integer ) );
                            break;
                        case REAL:
                            cJSON_AddItemToArray( array, cJSON_CreateNumber( val->value.real ) );
                            break;
                        case STRING:
                            cJSON_AddItemToArray( array, cJSON_CreateString( val->value.string ) );
                            break;
                        case BINARY:
                        {
                            swref *ref = sw_save_data_to_store( NULL, NULL, val->value.data, val->size );
                            result = swref_serialize( ref );
                            swref_free(ref);
                        }
                            break;
                        default:
                            cJSON_AddItemToArray( array, cJSON_CreateNull() );
                            break;
                    }

                }
            }
                break;
            case CONCRETE:
            default:
                cJSON_AddItemToArray( array, cJSON_CreateNumber(ref->size) );
                cJSON_AddItemToArray( array, cJSON_CreateStringArray( ref->loc_hints, ref->loc_hints_size ) );
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

swref *swref_deserialize( cJSON *json ){

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

        result->ref_id = strdup(cJSON_GetArrayItem( ref, 1 )->valuestring);

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

