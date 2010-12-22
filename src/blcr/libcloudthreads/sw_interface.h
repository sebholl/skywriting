#include <stdio.h>

#include <curl/curl.h>
#include <openssl/sha.h>

#include "curl_helper_functions.h"

void sw_init( void );


// C implementation of
//   src/python/skywriting/runtime/worker/master_proxy.py
//   spawn_tasks( self, parent_task_id, tasks )

int sw_spawntasks( const unsigned char *master_url, const unsigned char *parent_task_id, const unsigned char *handler, const unsigned char *dependencies );

const unsigned char *sw_post_file_to_worker( const unsigned char *worker_url, const unsigned char *filepath );




unsigned char *sw_get_new_task_id( const unsigned char *current_task_id );

unsigned char *sw_get_new_output_id( const unsigned char *handler, const unsigned char *task_id );


inline unsigned char* sw_get_current_worker_url( void );

inline unsigned char* sw_get_current_task_id( void );

inline unsigned char* sw_get_master_url( void );


// C implementation of
//   src/python/skywriting/runtime/task_executor.py
//   spawn_func( self, spawn_expre, args )
unsigned char *sw_create_json_task_descriptor( const unsigned char* current_task_id, const unsigned char *handler, const unsigned char *jsonenc_dependencies );

unsigned char *sw_sha1_hex_digest_from_bytes( const unsigned char *bytes, unsigned int len );


