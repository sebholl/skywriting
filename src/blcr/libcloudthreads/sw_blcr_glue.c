#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "blcr_interface.h"
#include "sw_interface.h"

int sw_blcr_init( void ){

    return blcr_init_framework() &&
           sw_init();

    return 1;

}

void sw_blcr_update_env( void ){

    FILE *named_pipe;
    const char *task_id;
    char *path;
    char env_name[2048], env_value[2048];

    task_id = sw_get_current_task_id();

    asprintf( &path, "/tmp/%s", task_id ); //Actually the parent task ID at this point

    printf( "Opening named FIFO \"%s\".\n", path );

    named_pipe = fopen( path, "r" );
    free(path);

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

    int chkpt_size;

    char *checkpoint_resource;
    char *dependencies;
    char *path;

    path = blcr_generate_context_filename();

    result = -1;

    if( blcr_spawn_function( path, sw_blcr_update_env, fptr ) ){

        checkpoint_resource = sw_post_file_to_worker( sw_get_current_worker_url(), path );

        {
            struct stat buf;

            if( stat(path, &buf)==-1 ){
                perror("sw_blcr_spawnthread: cannot retrieve checkpoint filesize");
                return result;
            }

            chkpt_size = buf.st_size;

        }

        asprintf( &dependencies, "\"checkpoint\": {\"__ref__\": [\"c2\", \"%s\", %d, [\"%s\"]]}", checkpoint_resource, chkpt_size, sw_get_current_worker_url() );

        free(checkpoint_resource);

        result = sw_spawntasks( sw_get_master_url(), sw_get_current_task_id(), "blcr", dependencies );

        free( dependencies );

    } else {

        perror( "Couldn't checkpoint process so we should perhaps resort to local threading.\n" );

    }

    return result;

}
