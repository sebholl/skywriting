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
    int index = (int)_index;

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
            thread[0] = cldthread_smart_create( Fib, (void *)(index - 1), _group_id );
            thread[1] = cldthread_smart_create( Fib, (void *)(index - 2), _group_id );

            cldthread_joins( thread, 2 );
            result = cldvalue_to_intmax(thread[0]->result) + cldvalue_to_intmax(thread[1]->result);
            break;
        }
    }

    return cldthread_exit( cldvalue_integer(result) );
}
