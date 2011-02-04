#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../sw_blcr_glue.h"

void *my_thread();

int main(int argc, char *argv[])
{
    int i;

    char *tmp = strdup("");
    char *ret_value;


    cldthread *threads[4];

    sw_blcr_init();

    for( i = 0; i < 4; i++ ){
        printf("Creating thread %d.\n", i );
        threads[i] = sw_blcr_spawnthread( my_thread, (void *)i );
        /*
        printf("Waiting for thread %d.\n", i );
        sw_blcr_wait_thread( threads[i] );
        */
    }

    printf("Waiting for all threads.\n");
    sw_blcr_wait_threads( threads, 4 );

    for( i = 0; i < 4; i++ ){
        asprintf( &ret_value, "%sThread %d Output: \"%s\"\n", tmp, i, threads[i]->result->value.string );
        free(tmp);
        tmp = ret_value;
    }

    printf( "Finished!\n" );

    sw_blcr_submit_output( cldthread_result_string( ret_value ) );

    return 0;
}


void *my_thread(int thread_id)
{
    char *ret_value;

    int count = 1;
    printf( "Thread ID %d\n", thread_id );
    while(count <= 10){
        printf( "%p: Hello World!\n", (void*)count++ );
        sleep(1);
    }

    asprintf( &ret_value, "I've finished! (Thread ID: %d)", thread_id );

    return cldthread_result_string( ret_value );
}
