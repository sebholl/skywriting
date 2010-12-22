#define _GNU_SOURCE
#include <stdio.h>

#include "blcr_interface.h"
#include "sw_interface.h"

int sw_blcr_init( void ){

    blcr_init_framework();
    sw_init();

    return 1;

}

void sw_blcr_update_env( void ){

    FILE *named_pipe;
    unsigned char env_name[2048], env_value[2048];

    named_pipe = fopen( "/special/skywriting/path/worker_comm", "r" );
    if(named_pipe == NULL) exit(-1000);

    while(!feof(named_pipe)){
        fgets( env_name, 2048, named_pipe );
        fgets( env_value, 2048, named_pipe );
        printf( "Setting %s to \"%s\".\n", env_name, env_value );
        setenv( env_name, env_value, 1 );
    }


}

int sw_blcr_spawnthread( void(*fptr)(void) ){

    int result;

    unsigned char *checkpoint_resource;
    unsigned char *dependencies;
    unsigned char *path;

    path = blcr_generate_context_filename();

    blcr_spawn_function( path, sw_blcr_update_env, fptr );

    checkpoint_resource = sw_post_file_to_worker( sw_get_current_worker_url(), path );

    asprintf( &dependencies, "\"checkpoint\": \"%s\"", checkpoint_resource );

    free(checkpoint_resource);

    result =  sw_spawntasks( sw_get_master_url(), sw_get_current_task_id(), "blcr", dependencies );

    free( dependencies );

    return result;

}
