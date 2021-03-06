#include <unistd.h>

/* Bases */
#define HaltonSeq_intBaseP_sz 2
static int HaltonSeq_intBaseP[] = {2, 3};

/* Maximum number of digits allowed */
#define HaltonSeq_intBaseK_sz 2
static int HaltonSeq_intBaseK[] = {63, 40};

typedef struct {

    uintmax_t index;
    double *x;
    double **q;
    uintmax_t **d;

} HaltonSeq;

HaltonSeq *HaltonSeq_Create( uintmax_t const startindex ){

    HaltonSeq *result = calloc( 1, sizeof( HaltonSeq ) );

    result->index = startindex;

    result->x = calloc( HaltonSeq_intBaseK_sz, sizeof( double ) );
    result->q = calloc( HaltonSeq_intBaseK_sz, sizeof( double * ) );
    result->d = calloc( HaltonSeq_intBaseK_sz, sizeof( uintmax_t * ) );

    size_t i;

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        result->x[i] = 0;
        result->q[i] = calloc( HaltonSeq_intBaseK[i], sizeof( double ) );
        result->d[i] = calloc( HaltonSeq_intBaseK[i], sizeof( uintmax_t ) );
    }

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        uintmax_t k = startindex;
        size_t j;
        for(j = 0; j < HaltonSeq_intBaseK[i]; j++){
            result->q[i][j] = (j == 0 ? 1.0 : result->q[i][j-1])/HaltonSeq_intBaseP[i];
            result->d[i][j] = (uintmax_t)(k % HaltonSeq_intBaseP[i]);
            k = (k - result->d[i][j])/HaltonSeq_intBaseP[i];
            result->x[i] += result->d[i][j] * result->q[i][j];
        }
    }

    return result;

}


double *HaltonSeq_NextPoint( HaltonSeq *seq ){

    seq->index++;

    size_t i, j;

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        for(j = 0; j < HaltonSeq_intBaseK[i]; j++){
            seq->d[i][j]++;
            seq->x[i] += seq->q[i][j];
            if(seq->d[i][j] < HaltonSeq_intBaseP[i]) break;
            seq->d[i][j] = 0;
            seq->x[i] -= (j==0 ? 1.0 : seq->q[i][j-1]);
        }
    }

    return seq->x;

}

void HaltonSeq_Free( HaltonSeq *seq ){

    size_t i;

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        free( seq->q[i] );
        free( seq->d[i] );
    }

    free( seq->x );
    free( seq->q );
    free( seq->d );

    free( seq );

}
