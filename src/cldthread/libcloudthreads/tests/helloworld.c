#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../cldthread.h"

cldvalue *my_thread();

int main(int argc, char *argv[])
{
    int i;

    char *tmp = strdup("");
    char *ret_value;


    cldthread *threads[4];

    cldthread_init();

    for( i = 0; i < 4; i++ ){
        printf("Creating thread %d.\n", i );
        threads[i] = cldthread_create( my_thread, (void *)i );
        printf("Created thread %p.\n", (void *)threads[i] );
        /*
        printf("Waiting for thread %d.\n", i );
        cldthread_join( threads[i] );
        */
    }

    printf("Waiting for all threads.\n");
    cldthread_joins( threads, 4 );

    for( i = 0; i < 4; i++ ){
        asprintf( &ret_value, "%sThread %d Output: \"%s\"\n", tmp, i, cldthread_result_as_string(threads[i]) );
        free(tmp);
        tmp = ret_value;
    }

    printf( "Finished!\n" );

    return cldapp_exit( cldvalue_string( ret_value ) );

}


cldvalue *my_thread(int thread_id)
{
    char *ret_value;

    int count = 1;
    printf( "Thread ID %d\n", thread_id );
    while(count <= 5){
        printf( "%d: Hello World!\n", count++ );
        sleep(1);
    }

    asprintf( &ret_value, "I've finished! (Thread ID: %d)", thread_id );

    return cldvalue_string( ret_value );
}
