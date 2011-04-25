#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

#include <cldthread.h>

cldvalue *ListLinker( void *arg );

typedef struct {

    int payload;
    cldptr next_ptr;

} cldlink_int;


cldptr cldlink_create( int value, cldptr next ){

    /* Allocate memory in the cldptr heap */
    cldptr cptr = cldptr_malloc( sizeof(cldlink_int) );

    /* Retrieve the corresponding memory address for this ptr */
    cldlink_int *link = cldptr_deref( cptr );

    /* Set the values of the link */
    link->payload = value;
    link->next_ptr = next;

    /* And return */
    return cptr;

}

cldvalue *cldlink_iterate( cldptr head ){

    size_t i = 0;

    /* Allocate a temporary array for holding all the link values */
    size_t arr_size = 4;
    cldvalue **values = calloc( arr_size, sizeof(cldvalue*) );

    while (!cldptr_is_null(head)){

        /* Dereference Cloud Pointer to Link */
        cldlink_int *link = ((cldlink_int *)cldptr_deref(head));

        /* Resize the array where we are collecting item values together (if necessary). */
        if( arr_size <= i ) values = realloc( values, (arr_size<<=2) * sizeof(cldvalue*) );

        /* Store link value in array */
        values[i++] = cldvalue_integer( link->payload );

        /* Print out the value on the current worker (debugging purposes) */
        printf( "Item %d: %d\n", (int)i, link->payload );

        /* Advance to the next link in the list */
        head = link->next_ptr;

    }

    /* Set the result to an array of cldvalue s */
    cldvalue *result = cldvalue_array( values, i );

    /* Free the temporary allocated array. */
    free( values );

    return result;

}


int main(int argc, char *argv[])
{

    if( !cldthread_init() ){

        fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                         "instead of attempting to invoke it directly. \n" );

        exit( EXIT_FAILURE );

    }

    int count = 10;

    if( (argc == 1) || ( argc == 2 && sscanf( argv[1], "%d", &count ) ) ){

        cldvalue *output = cldlink_iterate( ListLinker( (void *)count )->value.ptr );

        return cldapp_exit( output );

    } else {

        printf( "Invalid parameters.\n\nUsage: linkedlist [<no of links>|default:10]\n" );

    }

    return -1;

}


cldvalue *ListLinker( void *arg ){

    cldvalue *result;

    int const count = (int)arg;

    if( count ){

        /* Spawn a thread */
        cldthread *thread = cldthread_create( ListLinker, (void *)(count - 1) );

        /* Request a value for it */
        cldthread_join( thread );

        /* Append a link to head of the thread result */
        cldptr link = cldlink_create( count, cldvalue_to_cldptr( cldthread_result_as_cldvalue( thread ) ) );

        /* Return the cldptr as the new thread result */
        result = cldvalue_ptr( link );

    } else {

        /* If the last item in the list, return a NULL cldptr. */
        result = cldvalue_ptr( cldptr_null() );

    }

    return result;

}

