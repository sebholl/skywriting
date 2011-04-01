#pragma once

#include "swref.h"

#define VERBOSE 0
#define ALLOW_NONDETERMINISM 0

/* Initialization */

int sw_init( void );


/* Managing tasks */

int sw_spawntask( const char *new_task_id,
                  const char *output_task_id,
                  const char *parent_task_id,
                  const char *handler,
                  cJSON *jsonenc_dependencies,
                  int const is_continuation );


int sw_publish_ref( const char *master_loc, const char *task_id, const swref *ref );

#if ALLOW_NONDETERMINISM

cJSON *sw_query_info_for_output_id( const char *output_id );

int sw_abort_task( const char *master_url, const char *task_id );

#endif


/* Block-store access */

swref *sw_save_data_to_store( const char *worker_loc, const char *id, const void *data, size_t size );

#define sw_save_string_to_store( worker_loc, id, str ) sw_save_data_to_store( (worker_loc), (id), (str), strlen(str) )

swref *sw_move_file_to_store( const char *worker_loc, const char *filepath, const char *id );

char *sw_generate_block_store_path( const char *id );

int sw_open_fd_for_id( const char *id );

/* Environment querying */

const char *sw_get_current_task_id( void );

const char *sw_get_current_output_id( void );

const char* sw_get_master_loc( void );

const char* sw_get_current_worker_loc( void );

const char *sw_get_block_store_path( void );


/* Environment setting */

int sw_set_current_task_id( const char *taskid );


/* Miscellaneous helper functions */

char *sw_generate_new_task_id( const char *handler, const char *group_id, const char* desc );

char *sw_generate_task_id( const char *handler, const char *group_id, const void* unique_id );

char *sw_generate_suffixed_id( const char *task_id, const char *suffix );

#define sw_generate_output_id( task_id ) sw_generate_suffixed_id( (task_id), "output" )

cJSON *sw_create_json_task_descriptor( const char *new_task_id,
                                       const char *output_task_id,
                                       const char *current_task_id,
                                       const char *handler,
                                       cJSON *jsonenc_dependencies,
                                       int const is_continuation );

