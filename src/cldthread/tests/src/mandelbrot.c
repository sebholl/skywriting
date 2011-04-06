#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pnglite.h>
#include <cldthread.h>

cldvalue *Mandelbrot_Tile_Generator( void *arg );
unsigned char *Tiler( cldthread **tiles, size_t const count_x, size_t const count_y, size_t const tile_width, size_t const tile_height );

typedef struct {

    int tWidth;
    int tHeight;
    int sWidth;
    int sHeight;
    int tOffX;
    int tOffY;
    int maxit;
    double scale;

} mtginput;


int main( int argc, char *argv[] ) {

    int countx, county;
    int sizex, sizey;

    int maxit = 250;
    double scale = 0.005;

    if(( argc >= 5 ) && sscanf( argv[1], "%d", &sizex )
                     && sscanf( argv[2], "%d", &sizey )
                     && sscanf( argv[3], "%d", &countx )
                     && sscanf( argv[4], "%d", &county ) ) {

        if( argc >= 6 ) sscanf( argv[5], "%d", &maxit );

        if( argc >= 7 ) sscanf( argv[6], "%lf", &scale );


        if( !cldthread_init() ) {

            fprintf( stderr, "Please schedule this application using the CloudApp CIEL executor "
                     "instead of attempting to invoke it directly. \n" );

            exit( EXIT_FAILURE );

        }

        png_init( 0, 0 );

        cldthread **tiles = calloc( countx * county, sizeof( cldthread * ) );

        mtginput *input = calloc( 1, sizeof( mtginput ) );

        input->tWidth = sizex / countx;
        input->tHeight = sizey / county;
        input->tOffX = 0;
        input->tOffY = 0;
        input->sWidth = 800;
        input->sHeight = 800;
        input->maxit = maxit;
        input->scale = scale;

        size_t y, x;

        for( y = 0; y < county; ++y ) {
            for( x = 0; x < countx; ++x ) {
                input->tOffX = x;
                input->tOffY = y;
                tiles[( y*countx ) + x] = cldthread_create( Mandelbrot_Tile_Generator, input );
            }
        }

        cldthread_joins( tiles, countx * county );

        unsigned char *big_data = Tiler( tiles, countx, county, input->tWidth, input->tHeight );

        if( big_data != NULL ) {

            png_t pngInfo;

            png_open_write( &pngInfo, 0, fdopen( cldthread_open_result_as_stream(), "ab" ) );

            png_set_data( &pngInfo, countx * input->tWidth, county * input->tHeight, 8, PNG_TRUECOLOR_ALPHA, big_data );

            png_close_file( &pngInfo );

        }

        free( tiles );

        return cldapp_exit( cldvalue_none() );

    } else {

        printf( "\nInvalid parameters.\n\nUsage: mandelbrot sizeX sizeY tilesX tilesY myX myY [max iterations] [scale]\n\n" );

    }

    return -1;

}

int getPixel( double x0, double y0, int maxit ) {

    double x = 0.0;
    double y = 0.0;

    double x2 = x * x;
    double y2 = y * y;

    int it = 0;

    while( x2 + y2 < 4.0  &&  it < maxit ) {
        y = 2 * x * y + y0;
        x = x2 - y2 + x0;

        x2 = x * x;
        y2 = y * y;

        it++;
    }

    return ((( it * 0xFF ) / maxit ) << 24 ) | 0x004080FF;

}

cldvalue *Mandelbrot_Tile_Generator( void *arg ) {

    const mtginput *input = ( const mtginput * )arg;

    png_t pngInfo;

    png_open_write( &pngInfo, 0, fdopen( cldthread_open_result_as_stream(), "ab" ) );

    unsigned int *data = calloc( input->tWidth * input->tHeight, sizeof( int ) );

    int const maxit = input->maxit;
    double const scale = input->scale;

    int const tWidth = input->tWidth;
    int const tHeight = input->tHeight;

    double const iLoopConst = (( input->tWidth * input->tOffX ) - ( input->sWidth >> 1 ) ) * input->scale;
    double const jLoopConst = (( input->tHeight * input->tOffY ) - ( input->sHeight >> 1 ) ) * input->scale;

    size_t i = 0;
    int localI, localJ;

    for( localJ = 0; localJ < tHeight; ++localJ ) {
        for( localI = 0; localI < tWidth; ++localI ) {
            data[i++] = getPixel( iLoopConst + localI * scale,
                                  jLoopConst + localJ * scale,
                                  maxit );
        }
    }

    png_set_data( &pngInfo, input->tWidth, input->tHeight, 8, PNG_TRUECOLOR_ALPHA, ( unsigned char * )data );

    png_close_file( &pngInfo );

    return cldvalue_none();

}


unsigned char *Tiler( cldthread **tiles, size_t const count_x, size_t const count_y, size_t const tile_width, size_t const tile_height ) {

    png_t pnginfo;

    unsigned char *big_data = NULL, *tile_data = NULL;

    size_t tile_x, tile_y;

    for( tile_y = 0; tile_y < count_y; ++tile_y ) for( tile_x = 0; tile_x < count_x; ++tile_x ) {

            png_open_read( &pnginfo, 0, fdopen( cldthread_result_as_fd( tiles[tile_x + ( count_x * tile_y )] ), "rb" ) );

            int const rowbytecount = tile_width * pnginfo.bpp;

            if( big_data == NULL ) big_data = malloc(( tile_height * count_y ) * ( count_x * rowbytecount ) );

            tile_data = realloc( tile_data, tile_height * rowbytecount );

            png_get_data( &pnginfo, tile_data );

            png_close_file( &pnginfo );

            size_t y;

            for( y = 0; y < tile_height; ++y )

                memcpy( &big_data[(((( tile_y * tile_height ) + y ) * count_x ) + tile_x ) * rowbytecount ],
                        &tile_data[ y * rowbytecount ],
                        rowbytecount );

        }

    free( tile_data );

    return big_data;

}

