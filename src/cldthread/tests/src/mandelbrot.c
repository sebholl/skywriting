#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

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
    int offx, offy;

    int maxit = 250;
    double scale = 0.005;

    if(( argc >= 7 ) && sscanf( argv[1], "%d", &sizex )
                     && sscanf( argv[2], "%d", &sizey )
                     && sscanf( argv[3], "%d", &countx )
                     && sscanf( argv[4], "%d", &county )
                     && sscanf( argv[5], "%d", &offx )
                     && sscanf( argv[6], "%d", &offy ) ) {

        if( argc >= 8 ) sscanf( argv[7], "%d", &maxit );

        if( argc >= 9 ) sscanf( argv[8], "%lf", &scale );


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
        input->sWidth = 800;
        input->sHeight = 800;
        input->maxit = maxit;
        input->scale = scale;

        size_t y, x;

        for( y = 0; y < county; ++y ) {
            for( x = 0; x < countx; ++x ) {
                input->tOffX = x + offx;
                input->tOffY = y + offy;
                tiles[( y*countx ) + x] = cldthread_create( Mandelbrot_Tile_Generator, input );
            }
        }

        cldthread_joins( tiles, countx * county );

        unsigned char *big_data = Tiler( tiles, countx, county, input->tWidth, input->tHeight );

        if( big_data != NULL ) {

            png_t pngInfo;

            png_open_write( &pngInfo, 0, fdopen( cldthread_write_result(), "ab" ) );

            png_set_data( &pngInfo, countx * input->tWidth, county * input->tHeight, 8, PNG_TRUECOLOR_ALPHA, big_data );

            png_close_file( &pngInfo );

        }

        free( tiles );

        /* We are streaming our result, so this return value is ignore anyway. */
        return cldapp_exit( cldvalue_none() );

    } else {

        printf( "\nInvalid parameters.\n\nUsage: mandelbrot sizeX sizeY tilesX tilesY myX myY [max iterations] [scale]\n\n" );

    }

    return -1;

}

/*
  Adapted from http://mjijackson.com/2008/02/rgb-to-hsl-and-rgb-to-hsv-color-model-conversion-algorithms-in-javascript,
  which was adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 */
int hsvToRgb( double const h, double const s, double const v_in ){

    double const i = floor( h * 6 );
    double const f = ( h * 6 ) - i;

    char const v = (char)( v_in * 255 );

    char const p = (char) ((1.0 - s) * v);
    char const q = (char) ((1.0 - f * s) * v);
    char const t = (char) ((1.0 - (1.0 - f) * s) * v);

    char r = 255, g = 255, b = 255;

    switch( ((int)i) % 6 ){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    return ( b << 16 ) | ( g << 8 ) | r;
}


int getPixel( double x0, double y0, int maxit ) {

    double x = 0.0;
    double y = 0.0;

    double x2 = x * x;
    double y2 = y * y;

    int it = 0;

    while( ( x2 + y2 < 4.0 ) && ( it < maxit ) ) {

        y = 2 * x * y + y0;
        x = x2 - y2 + x0;

        x2 = x * x;
        y2 = y * y;

        it++;

    }

    return hsvToRgb( ((double)it)/maxit, (0.6+0.4*cos( it / 40.0 )), 1.0 ) | 0xFF000000;
    //return ((( it * 0xFF ) / maxit ) << 24 ) | 0x004080FF;
    //return 0xFF000000 | it;

}

cldvalue *Mandelbrot_Tile_Generator( void *arg ) {

    const mtginput *input = ( const mtginput * )arg;

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
            data[i++] = getPixel( iLoopConst + localI * scale, jLoopConst + localJ * scale, maxit );
        }
    }

    png_t pngInfo;

    png_open_write( &pngInfo, 0, fdopen( cldthread_write_result(), "ab" ) );

    png_set_data( &pngInfo, input->tWidth, input->tHeight, 8, PNG_TRUECOLOR_ALPHA, ( unsigned char * )data );

    png_close_file( &pngInfo );

    /* We are streaming our result, so this return value is ignore anyway. */
    return cldvalue_none();

}

static size_t read_offset = 0, read_total = 0;

/* Function that should mimic fread() but for memory pointers. */

size_t read_mem(void* output, size_t size, size_t numel, void* user_pointer){

    size_t const remaining = ( read_total - read_offset ) / size;

    numel = remaining < numel ? remaining : numel;

    if( output != NULL ) memcpy( output, user_pointer+read_offset, size*numel );

    read_offset += size * numel;

    return numel;

}

unsigned char *Tiler( cldthread **tiles, size_t const count_x, size_t const count_y, size_t const tile_width, size_t const tile_height ) {

    png_t pnginfo;

    void *png_data;
    unsigned char *big_data = NULL, *tile_data = NULL;

    size_t tile_x, tile_y;

    for( tile_y = 0; tile_y < count_y; ++tile_y ) for( tile_x = 0; tile_x < count_x; ++tile_x ) {

            read_offset = 0;

            /* libPngLite doesn't like streaming files, so let's fully load it into memory first */
            int retcode = png_open_read( &pnginfo, read_mem, png_data = cielID_dump_stream( tiles[tile_x + ( count_x * tile_y )], &read_total ) );

            if( retcode != PNG_NO_ERROR ) {
                fprintf( stderr, "Error while attempting to decode PNG (png_data: %p).\n%s\n", png_data, png_error_string(retcode) );
                exit( EXIT_FAILURE );
            }

            int const rowbytecount = tile_width * pnginfo.bpp;

            if( big_data == NULL ) big_data = malloc(( tile_height * count_y ) * ( count_x * rowbytecount ) );

            tile_data = realloc( tile_data, tile_height * rowbytecount );

            retcode = png_get_data( &pnginfo, tile_data );

            if ( retcode != PNG_NO_ERROR ) {
                fprintf( stderr, "Error while attempting to dump PNG (file size: %d, error code: %d).\n%s\n", (int)read_total, retcode, png_error_string(retcode) );
                exit( EXIT_FAILURE );
            }

            free( png_data );

            size_t y;

            for( y = 0; y < tile_height; ++y )

                memcpy( &big_data[(((( tile_y * tile_height ) + y ) * count_x ) + tile_x ) * rowbytecount ],
                        &tile_data[ y * rowbytecount ],
                        rowbytecount );

        }

    free( tile_data );

    return big_data;

}

