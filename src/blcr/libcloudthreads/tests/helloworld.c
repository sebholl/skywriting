#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "../sw_blcr_glue.h"

void my_thread();

int main(int argc, char *argv[])
{
    int i;
    cldthread *threads[4];

    sw_blcr_init();

    for( i = 0; i < 4; i++ ){
        printf("Creating thread %d.\n", i );
        threads[i] = sw_blcr_spawnthread( my_thread, (void *)i );
        //printf("Waiting for thread %d.\n", i );
        //sw_blcr_wait_thread( threads[i] );
    }

    printf("Waiting for all threads.\n");
    sw_blcr_wait_threads( threads, 4 );

    printf( "Finished!\n" );

    return 0;
}


void my_thread(int thread_id)
{
    int count = 1;
    printf( "Thread ID %d\n", thread_id );
    while(count <= 10){
        printf( "%p: Hello World!\n", (void*)count++ );
        sleep(1);
    }

}
