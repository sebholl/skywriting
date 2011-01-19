#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "../sw_blcr_glue.h"

void my_thread(void);

int main(int argc, char *argv[])
{
    int i;

    sw_blcr_init();

    for( i = 0; i < 4; i++ ) sw_blcr_spawnthread( my_thread );

    return 0;
}


void my_thread(void)
{
    int count = 1;

    while(count < 10){
        printf( "%p: Hello World!\n", (void*)count++ );
        sleep(1);
    }

}
