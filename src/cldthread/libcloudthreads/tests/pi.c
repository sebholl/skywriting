#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "../cldthread.h"

#include "pi_halton.c"

double Pi_Reducer( int numSamples, int numSurveys );

int main(int argc, char *argv[])
{
    int numSamples, numSurveys;

    if(argc==3 && sscanf( argv[1], "%d", &numSamples ) && sscanf( argv[2], "%d", &numSurveys )){

        double result;

        cldthread_init();

        result = Pi_Reducer( numSamples, numSurveys );

        return cldapp_exit( cldvalue_real( result ) );

    } else {

        printf( "Invalid parameters.\n\nUsage: pi <num_samples> <num_surveys>\n" );

    }

    return -1;

}


void *Pi_Mapper(void *_arg){

    int i = 0L;
    int counts[] = {0L, 0L};

    int *arg = (int *)_arg;

    int numSamples = arg[0];
    int offset = arg[1];

    double x, y;
    double *point;

    HaltonSeq *seq = HaltonSeq_Create(offset);

    printf( "Sampling %d random numbers from offset %d...\n", numSamples, offset );

    for(i = 0; i < numSamples; i++){

        point = HaltonSeq_NextPoint(seq);

        x = point[0] - 0.5;
        y = point[1] - 0.5;

        counts[x*x + y*y > 0.25]++;

    }

    return cldthread_exit( cldvalue_integer(counts[0]) );
}


double Pi_Reducer(int const numSamples, int const numSurveys){

    int i;
    int thread_input[2];
    cldthread **threads;

    thread_input[0] = numSamples;
    thread_input[1] = 0;

    threads = calloc( numSurveys, sizeof( cldthread * ) );

    printf("Starting thread spawning...\n");

    for(i = 0; i < numSurveys; i++){
        thread_input[1] += numSamples;
        threads[i] = cldthread_create( Pi_Mapper, thread_input );
        printf("Spawned thread %d (%p).\n", i, threads[i] );
    }

    printf("Waiting on thread results...\n");

    cldthread_joins( threads, numSurveys );

    printf("Calculating Pi...\n");

    long numInside = 0L;

    for(i = 0; i < numSurveys; i++){
        numInside += cldthread_result_as_long( threads[i] );
    }

    free(threads);

    printf("Returning result...\n");

    return ((double)numInside) / ((numSamples * numSurveys) * 0.25);

}


