#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <cldthread.h>

cldvalue *Fib(void *_index);

char *_group_id;

int disableMemoisation = 0;

int main(int argc, char *argv[])
{
    int termIndex;

    if(argc>=2 && sscanf( argv[1], "%d", &termIndex ) && termIndex >= 0){

        if( argc >= 3  && !sscanf( argv[2], "%d", &disableMemoisation ) ) disableMemoisation = 0;

        if( !cldthread_init() ){

            fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                             "instead of attempting to invoke it directly. \n" );

            exit( EXIT_FAILURE );

        }

        if( asprintf( &_group_id, "%p", Fib ) == 1 ) exit( EXIT_FAILURE );

        return cldapp_exit( Fib((void*)termIndex) );

    } else {

        printf( "\nInvalid parameters (input must be a positive integer).\n\nUsage: fib <term-index> [<disable_memoisation>]\n\n" );

    }

    return -1;

}


cldvalue *Fib(void *_index){

    uintmax_t result;
    const int index = (int)_index;

    printf("Executing Fib(%d)...\n", index );

    switch(index){
        case 0:
            result = 0;
            break;
        case 1:
            result = 1;
            break;
        default:
        {
            cldthread *thread[2];

            printf("--> Spawning Thread Fib(%d)...\n", (index - 1) );
            if(!disableMemoisation)
                thread[0] = cldthread_smart_create( _group_id, Fib, (void *)(index - 1) );
            else
                thread[0] = cldthread_create( Fib, (void *)(index - 1) );

            printf("--> Spawning Thread Fib(%d)...\n", (index - 2) );
            if(!disableMemoisation)
                thread[1] = cldthread_smart_create( _group_id, Fib, (void *)(index - 2) );
            else
                thread[1] = cldthread_create( Fib, (void *)(index - 2) );

            printf("--> Joining threads...\n" );
            cldthread_joins( thread, 2 );

            printf("--> Evaluating result...\n" );
            result = cldvalue_to_integer( cldthread_result_as_cldvalue(thread[0]) ) +
                     cldvalue_to_integer( cldthread_result_as_cldvalue(thread[1]) );
            break;
        }
    }

    printf("--> Exiting Fib(%d) with result of %" PRIuMAX "...\n", index, result );
    return cldvalue_integer(result);
}
