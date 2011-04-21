#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <cldthread.h>

#include "pi_halton.c"

double Pi_Reducer( long numSamples, long numSurveys );

int main(int argc, char *argv[])
{
    long numSamples, numSurveys;

    if( (argc==3) && sscanf( argv[1], "%ld", &numSamples ) && sscanf( argv[2], "%ld", &numSurveys )){

        double result;

        if( !cldthread_init() ){

            fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                             "instead of attempting to invoke it directly. \n" );

            exit( EXIT_FAILURE );

        }

        result = Pi_Reducer( numSamples, numSurveys );

        return cldapp_exit( cldvalue_real( result ) );

    } else {

        printf( "\nInvalid parameters.\n\nUsage: pi <num_samples> <num_surveys>\n\n" );

    }

    return -1;

}


cldvalue *Pi_Mapper(void *const _arg){

    const long *const arg = (long *)_arg;

    long const numSamples = arg[0];
    long const offset = arg[1];

    HaltonSeq *seq = HaltonSeq_Create(offset);

    printf( "Sampling %ld random numbers from offset %ld...\n", numSamples, offset );

    double x, y;
    double *point;

    long counts[] = {0L, 0L};

    long i = 0L;
    for(i = 0; i < numSamples; i++){

        point = HaltonSeq_NextPoint(seq);

        x = point[0] - 0.5;
        y = point[1] - 0.5;

        counts[x*x + y*y > 0.25]++;

    }

    return cldvalue_integer( counts[0] );
}


double Pi_Reducer(long const numSamples, long const numSurveys){

    cldthread **threads;

    threads = calloc( numSurveys, sizeof( cldthread * ) );

    printf("Starting thread spawning...\n");

    long i;

    long thread_input[2];

    thread_input[0] = numSamples;
    thread_input[1] = 0L;

    for(i = 0; i < numSurveys; i++){
        thread_input[1] += numSamples;
        threads[i] = cldthread_create( Pi_Mapper, thread_input );
        if( threads[i] == NULL ){
            fprintf( stderr, "Failed to spawn thread %ld.\n", i );
            fflush( stdout );
        }
        printf("Spawned thread %ld (%p).\n", i, threads[i] );
    }

    printf("Waiting on thread results...\n");

    cldthread_joins( threads, numSurveys );

    printf("Calculating Pi...\n");

    uintmax_t numInside = 0L;

    for(i = 0; i < numSurveys; i++){
        numInside += cldvalue_to_integer( cldthread_result_as_cldvalue( threads[i] ) );
    }

    free(threads);

    printf("Returning result...\n");

    return ( numInside / ((numSamples * numSurveys) * 0.25) );

}


