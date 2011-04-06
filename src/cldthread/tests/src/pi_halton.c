/* Bases */
#define HaltonSeq_intBaseP_sz 2
static int HaltonSeq_intBaseP[] = {2, 3};

/* Maximum number of digits allowed */
#define HaltonSeq_intBaseK_sz 2
static int HaltonSeq_intBaseK[] = {63, 40};

typedef struct HaltonSeq {

    long index;
    double *x;
    double ** q;
    int **d;

} HaltonSeq;

HaltonSeq *HaltonSeq_Create( long const startindex ){

    HaltonSeq *result = calloc( 1, sizeof( HaltonSeq ) );

    result->index = startindex;

    result->x = calloc( HaltonSeq_intBaseK_sz, sizeof( double ) );
    result->q = calloc( HaltonSeq_intBaseK_sz, sizeof( double * ) );
    result->d = calloc( HaltonSeq_intBaseK_sz, sizeof( int * ) );

    int i;

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        result->x[i] = 0;
        result->q[i] = calloc( HaltonSeq_intBaseK[i], sizeof( double ) );
        result->d[i] = calloc( HaltonSeq_intBaseK[i], sizeof( int ) );
    }

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        long k = startindex;
        int j;
        for(j = 0; j < HaltonSeq_intBaseK[i]; j++){
            result->q[i][j] = (j == 0 ? 1.0 : result->q[i][j-1])/HaltonSeq_intBaseP[i];
            result->d[i][j] = (int)(k % HaltonSeq_intBaseP[i]);
            k = (k - result->d[i][j])/HaltonSeq_intBaseP[i];
            result->x[i] += result->d[i][j] * result->q[i][j];
        }
    }

    return result;

}


double *HaltonSeq_NextPoint( HaltonSeq *seq ){

    int i, j;

    seq->index++;

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

    int i = 0;

    for(i = 0; i < HaltonSeq_intBaseK_sz; i++){
        free( seq->q[i] );
        free( seq->d[i] );
    }

    free( seq->x );
    free( seq->q );
    free( seq->d );

    free( seq );

}
