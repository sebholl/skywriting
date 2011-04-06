#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <cldthread.h>

cldvalue *ListLinker( void *arg );

typedef struct cldlink_int{

    int payload;
    cldptr next_ptr;

} cldlink_int;


cldptr cldlink_create( int value, cldptr next ){

    cldptr cptr = cldptr_malloc( sizeof(cldlink_int) );
    cldlink_int *link = cldptr_deref( cptr );

    link->payload = value;
    link->next_ptr = next;

    return cptr;

}

void cldlink_print( cldptr head ){

    int i = 0;
    while (!cldptr_is_null(head)){

        cldlink_int *link = ((cldlink_int *)cldptr_deref(head));

        printf( "Item %d: %d\n", ++i, link->payload );

        head = link->next_ptr;

    }

}


int main(int argc, char *argv[])
{

    if( !cldthread_init() ){

        fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                         "instead of attempting to invoke it directly. \n" );

        exit( EXIT_FAILURE );

    }

    //if(argc==1){

        cldlink_print( ListLinker( (void *)10 )->value.ptr );

        return cldapp_exit( cldvalue_none() );

    //} else {

    //    printf( "Invalid parameters.\n\nUsage: sha_stream <filepath>\n" );

    //}

    //return -1;

}


cldvalue *ListLinker( void *arg ){

    cldvalue *result;

    int count = (int)arg;

    if( count ){

        cldthread *thread = cldthread_create( ListLinker, (void *)(--count) );

        cldthread_join( thread );

        result = cldvalue_ptr( cldlink_create( (count + 1), cldvalue_to_cldptr( cldthread_result_as_cldvalue( thread ) ) ) );

    } else {

        result = cldvalue_ptr( cldptr_null() );

    }

    return result;

}

