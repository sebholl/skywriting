#include <stdio.h>

#include "common/curl_helper_functions.h"
#include "cldptr.h"

#include "blcr_interface.h"
#include "sw_interface.h"

/* private functions for cldthread.c */

static cielID *_outputstream = NULL;

static void _cldthread_submit_output( cldvalue *const value ){

    /* First of all, publish our shared memory heap */

    size_t heap_size;
    const void *const heap_data = cldptr_get_heap( &heap_size );

    if( heap_size > 0 ){

        cielID *heapid = cldptr_heap_cielID();

        sw_publish_ref( sw_get_master_loc(),
                        sw_get_current_task_id(),
                        sw_save_data_to_store( NULL, heapid->id_str, heap_data, heap_size ) );

        cielID_free( heapid );

    }


    /* And then once we know the heap is available, publish our output */

    if( _outputstream != NULL ){

        cielID_finalize_stream( _outputstream );

        cielID_free( _outputstream );

        _outputstream = NULL;

    } else {

        swref *data = swref_create( DATA, sw_get_current_output_id(), value, 0, NULL );

        cJSON *json = swref_serialize( data );

        char *tmp = cJSON_PrintUnformatted( json );

        cJSON_Delete( json );

        swref_free( data );

        sw_save_string_to_store( NULL, sw_get_current_output_id(), tmp );

        free( tmp );

    }

}

