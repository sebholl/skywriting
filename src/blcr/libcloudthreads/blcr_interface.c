#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libcr.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "blcr_interface.h"

/* Enums and global variables */

int checkpoint_count = 0;
static enum blcr_state blcr_checkpoint_status = blcr_error;

static int blcr_callback(void *arg);

/* API */

void blcr_init_framework( void ){
    cr_init();
    cr_register_callback(blcr_callback, NULL, CR_SIGNAL_CONTEXT);
}

int blcr_spawn_function( const unsigned char *filepath, void(*fptr)(void), void(*fptr2)(void) ){

    cr_checkpoint_args_t args;
    cr_checkpoint_handle_t hndl;

    cr_initialize_checkpoint_args_t( &args );

    args.cr_scope = CR_SCOPE_PROC;  // See blcr_common.h

    //args.cr_fd = fopen(filepath, "wb");
    args.cr_fd = open( (const char *) filepath, O_WRONLY | O_CREAT | O_TRUNC , 0600);

    blcr_checkpoint_status = blcr_error;
    cr_request_checkpoint( &args, &hndl );

    switch(blcr_checkpoint_status){
        case blcr_continue:
            cr_wait_checkpoint( &hndl, NULL );
            close(args.cr_fd);
            break;
        case blcr_restart:
            fptr();
            fptr2();
            exit(0);
            break;
        case blcr_error:
            break;
    }

    return 0;

}

/* Helper Functions */

static int blcr_callback(void *arg)
{
    int ret;

    checkpoint_count++;

    ret = cr_checkpoint(0);

    if (ret > 0) {
        blcr_checkpoint_status = blcr_restart;
    } else if (ret == 0) {
        blcr_checkpoint_status = blcr_continue;
    } else {
        blcr_checkpoint_status = blcr_error;
    }

    return 0;
}

unsigned char* blcr_generate_context_filename(void)
{
    unsigned char *p, *context_filename;

    if ( (p = (unsigned char *) getcwd(NULL, 0)) == NULL) return NULL;

    asprintf( &context_filename, "%s/context.%d.%d", p, getpid(), checkpoint_count );

    free(p);

    return context_filename;
}

