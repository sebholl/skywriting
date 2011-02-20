#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include "../sw_interface.h"
#include "../cldthread.h"

void *Fib(void *_index);

char *_group_id;

int main(int argc, char *argv[])
{
    int termIndex;

    if(argc==2 && sscanf( argv[1], "%d", &termIndex ) && termIndex >= 0){

        cldthread_init();

        asprintf( &_group_id, "%p", Fib );

        Fib((void*)termIndex);

    } else {

        printf( "Invalid parameters (input must be a positive integer).\n\nUsage: fib <term-index>\n" );

    }

    return -1;

}


void *Fib(void *_index){

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
            printf("--> Creating SmartThread Fib(%d)...\n", (index - 1) );
            thread[0] = cldthread_smart_create( Fib, (void *)(index - 1), _group_id );
            printf("--> Creating SmartThread Fib(%d)...\n", (index - 2) );
            thread[1] = cldthread_smart_create( Fib, (void *)(index - 2), _group_id );
            printf("--> Joining threads...\n" );
            cldthread_joins( thread, 2 );
            printf("--> Evaluating result...\n" );
            result = cldvalue_to_intmax(thread[0]->result) + cldvalue_to_intmax(thread[1]->result);
            break;
        }
    }

    printf("--> Exiting Fib(%d) with result of %ld...\n", index, (long)result );
    return cldthread_exit( cldvalue_integer(result) );
}
