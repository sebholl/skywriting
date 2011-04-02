#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <cldthread.h>

void CloudConsumer(int fd);
cldvalue *CloudProducer(void *_arg);

int main(int argc, char *argv[])
{

    if( !cldthread_init() ){

        fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                         "instead of attempting to invoke it directly. \n" );

        exit( EXIT_FAILURE );

    }

    //if(argc==1){

        cldthread *thread = cldthread_create( CloudProducer, NULL );

        cldthread_join( thread );

        CloudConsumer( cldthread_result_as_fd( thread ) );

        return cldapp_exit( cldvalue_none() );

    //} else {

    //    printf( "Invalid parameters.\n\nUsage: sha_stream <filepath>\n" );

    //}

    //return -1;

}


cldvalue *CloudProducer(void *_arg){

    int fd = cldthread_open_result_as_stream();

    int i;

    int len;
    char *str;

    for( i = 10; i > 0; i-- ){
        len = asprintf( &str, "%d --> I can count!!!! (%d)\n", i, fd );
        write( 1, str, len );
        write( fd, str, len );
        sleep(1);
    }

    return cldvalue_none();

}


void CloudConsumer( int fd ){

    char byte;

    printf("Printing consumed characters from fd (%d)...\n", fd);

    while( read( fd, &byte, sizeof( char ) ) != 0 ) printf("%c", byte);

    printf("Finished consuming characters...\n");

}


