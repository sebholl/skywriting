#pragma once

#include "swref.h"

//#define VERBOSE 1

/* Initialization */

int sw_init( void );


/* Managing tasks */

int sw_spawntask( const char *new_task_id,
                  const char *output_task_id,
                  const char *parent_task_id,
                  const char *handler,
                  cJSON *jsonenc_dependencies,
                  int const is_continuation );

cJSON *sw_query_info_for_output_id( const char *output_id );

int sw_abort_task( const char *master_url, const char *task_id );


/* Block-store access */

char *sw_dereference( const swref *ref, size_t *size_out );

cJSON *sw_get_json_from_store( const swref *ref );

swref *sw_save_data_to_store( const char *worker_loc, const char *id, const void *data, size_t size );

#define sw_save_string_to_store( worker_loc, id, str ) sw_save_data_to_store( worker_loc, id, str, strlen(str) )

swref *sw_move_file_to_store( const char *worker_loc, const char *filepath, const char *id );


/* Environment querying */

const char *sw_get_current_task_id( void );

const char *sw_get_current_output_id( void );

const char* sw_get_master_loc( void );

const char* sw_get_current_worker_loc( void );

const char *sw_get_block_store_path( void );


/* Environment setting */

int sw_set_current_task_id( const char *taskid );


/* Miscellaneous helper functions */

char *sw_generate_new_task_id( const char *task_type );

char *sw_generate_output_id( const char *task_id, const void* unique_id, const char *handler );

cJSON *sw_create_json_task_descriptor( const char *new_task_id,
                                       const char *output_task_id,
                                       const char *current_task_id,
                                       const char *handler,
                                       cJSON *jsonenc_dependencies,
                                       int const is_continuation );

