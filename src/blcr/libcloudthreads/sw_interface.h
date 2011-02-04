#pragma once

#define VERBOSE 1

int sw_init( void );


/* C implementation of
 *   src/python/skywriting/runtime/worker/master_proxy.py
 *   spawn_tasks( self, parent_task_id, tasks )
 */

int sw_spawntask( const char *new_task_id,
                  const char *output_task_id,
                  const char *master_url,
                  const char *parent_task_id,
                  const char *handler,
                  const char *jsonenc_dependencies,
                  int const is_continuation );

int sw_abort_task( const char *master_url, const char *task_id );

char *sw_post_string_to_worker( const char *worker_url, const char *id, const char *str );
char *sw_post_data_to_worker( const char *worker_url, const char *id, const void *data, size_t size );
char *sw_post_file_to_worker( const char *worker_url, const char *filepath );

char *sw_get_data_from_store( const char *worker_url, const char *id, size_t *size_out );

char *sw_get_new_task_id( const char *current_task_id, const char *task_type );

char *sw_get_new_output_id( const char *handler, const char *task_id );


inline const char* sw_get_current_worker_url( void );

inline int sw_set_current_task_id( const char *taskid );
inline const char* sw_get_current_task_id( void );

inline const char* sw_get_current_output_id( void );

inline const char* sw_get_master_url( void );


/* C implementation of
 *   src/python/skywriting/runtime/task_executor.py
 *   spawn_func( self, spawn_expre, args )
 */
char *sw_create_json_task_descriptor( const char *new_task_id,
                                      const char *output_task_id,
                                      const char *current_task_id,
                                      const char *handler,
                                      const char *jsonenc_dependencies,
                                      int const is_continuation );

char *sw_sha1_hex_digest_from_bytes( const char *bytes, unsigned int len, int shouldFreeInput );


