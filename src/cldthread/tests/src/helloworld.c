#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <cldthread.h>

cldvalue *my_thread();

int main(int argc, char *argv[])
{
    int i;

    cldthread *threads[4];
    cldvalue *results[4];

    if( !cldthread_init() ){

        fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                         "instead of attempting to invoke it directly. \n" );

        exit( EXIT_FAILURE );

    }

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
        results[i] = cldthread_result_as_cldvalue(threads[i]);
    }

    printf( "Finished!\n" );

    return cldapp_exit( cldvalue_array( results, 4 ) );

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

    if( asprintf( &ret_value, "I've finished! (Thread ID: %d)", thread_id ) == -1 ) exit( EXIT_FAILURE );

    return cldvalue_vargs( 2, cldvalue_string( ret_value ), cldvalue_integer( thread_id ) );
}
