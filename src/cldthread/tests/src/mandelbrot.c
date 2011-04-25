#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <math.h>

#include <pnglite.h>
#include <cldthread.h>

cldvalue *Mandelbrot_Tile_Generator( void *arg );
unsigned char *Tiler( cldthread **tiles, size_t const count_x, size_t const count_y, size_t const tile_width, size_t const tile_height );

typedef struct {

    double originX;
    double originY;
    int tWidth;
    int tHeight;
    int maxit;
    double scale;

} mtginput;


int main( int argc, char *argv[] ) {

    int sizex, sizey;

    if(( argc >= 3 ) && sscanf( argv[1], "%d", &sizex )
                     && sscanf( argv[2], "%d", &sizey ) ) {

        double offx = 0.0;
        if( argc > 3 ) sscanf( argv[3], "%lf", &offx );

        double offy = 0.0;
        if( argc > 4 ) sscanf( argv[4], "%lf", &offy );

        int countx = 2;
        if( argc > 5 ) sscanf( argv[5], "%d", &countx );

        int county = 2;
        if( argc > 6 ) sscanf( argv[6], "%d", &county );

        int maxit = 250;
        if( argc > 7 ) sscanf( argv[7], "%d", &maxit );

        double scale = 3.0;
        if( argc > 8 ) sscanf( argv[8], "%lf", &scale );

        scale /= sizex;

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
        input->maxit = maxit;
        input->scale = scale;

        double const stepWidth = (sizex / countx) * scale;
        double const stepHeight = (sizey / county) * scale;

        size_t y, x;

        /* Start at the top */
        input->originY = (sizey * scale)/2 - (stepHeight / 2) + offy;

        for( y = 0; y < county; ++y ) {

            /* Start on the left */
            input->originX = (stepWidth / 2) - (sizex * scale)/2 + offx;

            for( x = 0; x < countx; ++x ) {

                /* Spawn a thread to calculate that particular tile */
                tiles[( y*countx ) + x] = cldthread_create( Mandelbrot_Tile_Generator, input );

                /* Move right */
                input->originX += stepWidth;

            }

            /* Move down */
            input->originY -= stepHeight;

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

        /* We are writing the result directly to a fd so this return value is ignored anyway. */
        return cldapp_exit( cldvalue_none() );

    } else {

        printf( "\nInvalid parameters.\n\nUsage: mandelbrot imgWidth imgHeight [offsetX=0.0] [offsetY=0.0] "
                "[tileCountX=2] [tileCountY=2] [max iterations=250] [scale=3.0]\n\n"
                "Note \"scale\" is the multiple of the image width that represent 0-1.\n\n" );

    }

    return -1;

}

/*
  Adapted from http://mjijackson.com/2008/02/rgb-to-hsl-and-rgb-to-hsv-color-model-conversion-algorithms-in-javascript,
  which was adapted from http://en.wikipedia.org/wiki/HSV_color_space.
 */
static int hsvToRgb( double const h, double const s, double const v_in ){

    double const i = floor( h * 6 );
    double const f = ( h * 6 ) - i;

    unsigned char const p = (char) ((1.0 - s) * v_in);
    unsigned char const q = (char) ((1.0 - f * s) * v_in);
    unsigned char const t = (char) ((1.0 - (1.0 - f) * s) * v_in);

    unsigned char const v = (char) (v_in * 255);

    unsigned char r = 255, g = 255, b = 255;

    switch( ((unsigned int)i) % 6 ){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }

    return ( b << 16 ) | ( g << 8 ) | r;
}

static inline double stretch( double const x ){

    /* [(2x-1)^2 + 1] / 2 */
    long double const value = (2*x-1);
    return value*value*(x-0.5)+0.5;

}

/* Mandelbrot Set Algorithm */

static int getPixel( double x0, double y0, int maxit ) {

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

    double const proportion = stretch( ((double)it)/maxit );

    return hsvToRgb( (1.0-proportion)*0.66, 0.6 + (0.4 * proportion), proportion ) | 0xFF000000;
    //return ((( it * 0xFF ) / maxit ) << 24 ) | 0x004080FF;
    //return 0xFF000000 | it;

}

cldvalue *Mandelbrot_Tile_Generator( void *arg ) {

    const mtginput *input = ( const mtginput * )arg;

    unsigned int *data = calloc( input->tWidth * input->tHeight, sizeof( int ) );

    double const originX = input->originX;
    double const originY = input->originY;

    int const maxit = input->maxit;
    double const scale = input->scale;

    int const xMax = input->tWidth/2;
    int const yMax = input->tHeight/2;

    printf( "Calculating mandelbrot set for a %dx%d block centered on (%lf,%lf).\n",
            xMax*2, yMax*2, originX, originY );

    size_t i = 0;
    int localI, localJ;

    for( localJ = yMax; localJ > -yMax; --localJ ) {
        for( localI = -xMax; localI < xMax; ++localI ) {
            data[i++] = getPixel( originX + (localI * scale), originY + (localJ * scale), maxit );
        }
    }

    png_t pngInfo;

    png_open_write( &pngInfo, 0, fdopen( cldthread_write_result(), "ab" ) );

    png_set_data( &pngInfo, input->tWidth, input->tHeight, 8, PNG_TRUECOLOR_ALPHA, ( unsigned char * )data );

    png_close_file( &pngInfo );

    /* We are writing the result directly to a fd so this return value is ignored anyway. */
    return cldvalue_none();

}

/* Function that should mimic fread() but for memory pointers. */

static size_t read_offset = 0, read_total = 0;

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
            png_data = cielID_dump_stream( tiles[tile_x + ( count_x * tile_y )], &read_total );

            /* and now let's decode it */
            int retcode = png_open_read( &pnginfo, read_mem, png_data );

            if( retcode != PNG_NO_ERROR ) {

                fprintf( stderr, "Error while attempting to decode PNG (png_data: %p).\n%s\n", png_data, png_error_string(retcode) );

                exit( EXIT_FAILURE );

            }

            int const rowbytecount = tile_width * pnginfo.bpp;

            if( big_data == NULL ) big_data = malloc(( tile_height * count_y ) * ( count_x * rowbytecount ) );

            tile_data = realloc( tile_data, tile_height * rowbytecount );

            /* decompress the png data into an array of pixels */
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

