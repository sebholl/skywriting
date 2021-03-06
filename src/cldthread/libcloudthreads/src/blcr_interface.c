#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "helper/timer.h"

#include <libcr.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "blcr_interface.h"

/* Checkpointing Flags */
static int BLCR_CRFLAGS = CR_CHKPT_PTRACED_ALLOW|CR_CHKPT_DUMP_EXEC|CR_CHKPT_DUMP_PRIVATE;
static int BLCR_CRSCOPE = CR_SCOPE_PROC; /* See blcr_common.h */

/* Enums and global variables */

static int checkpoint_count = 0;
static enum blcr_state blcr_checkpoint_status = blcr_error;

static int blcr_callback();

/* API */

int blcr_init_framework( void ){

    int client_id, cb_id;

    client_id = cr_init();

    if (client_id < 0) {
      if (errno == ENOSYS) {
          fprintf(stderr, "<FATAL ERROR> blcr_init_framework(): libcr kernel module not installed\n");
      } else {
          fprintf(stderr, "<FATAL ERROR> blcr_init_framework(): cr_init() failed: %s\n", cr_strerror(errno));
      }
      return 0;
    }

    cb_id = cr_register_callback(blcr_callback, NULL, CR_SIGNAL_CONTEXT);

    if (cb_id < 0) {
      fprintf(stderr, "blcr_init_framework(): failed cr_register_callback(): %s\n", cr_strerror(errno));
      return 0;
    }

    return 1;
}

int blcr_fork( const char *filepath ){

    TIMER_LABEL( blcr_fork() )

    int result;

    cr_checkpoint_args_t args;
    cr_checkpoint_handle_t hndl;

    cr_initialize_checkpoint_args_t( &args );

	args.cr_flags |= BLCR_CRFLAGS;

    args.cr_scope = BLCR_CRSCOPE;

    args.cr_fd = open( filepath, O_WRONLY | O_CREAT | O_TRUNC , 0600);

    blcr_checkpoint_status = blcr_error;

    /* Flush any data waiting in STDOUT or STDERR because
     * we don't really want 2 copies after checkpoint. */
    fflush(stdout);fflush(stderr);

    result = cr_request_checkpoint( &args, &hndl );

    close(args.cr_fd);

    switch(blcr_checkpoint_status){
        case blcr_continue:
            cr_wait_checkpoint( &hndl, NULL );
            result = 1;
            break;
        case blcr_restart:
            result = 0;
            break;
        case blcr_error:
            result = -1;
            break;
    }

    /* Error displaying code from cr_checkpoint.c */

    if (result < 0) {

        fprintf( stderr, "blcr_fork(): cr_request_checkpoint(): %s\n", cr_strerror(errno) );

    } else if ( blcr_checkpoint_status == blcr_error ) {

        result = -1;

    }

    /*
    printf("Checkpointed with return value: %d (errorno: %d).\n", blcr_checkpoint_status, errno );
    */

    return result;

}

/* Helper Functions */

static int blcr_callback()
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

