#ifdef PROFILE

#include <stdio.h>
#include <sys/time.h>

#include "timer.h"

#define STACK_SIZE 200

FILE* _timer_fp = NULL;

static const char * string_stack[STACK_SIZE];
static struct timeval timer_stack[STACK_SIZE];
static size_t timer_stack_index = 0;

static char _prefix[ STACK_SIZE * 4 + 2 ];

static const char * __attribute__ ((no_instrument_function)) prefix( size_t const level ){

    if( level <= 0 ){

        _prefix[0] = '\0';

    } else {

        size_t i, p = 0;

        _prefix[p++] = ' ';
        _prefix[p++] = ' ';
        _prefix[p++] = '|';

        for( i = 1; i < timer_stack_index; i++ ){
            _prefix[p++] = ' ';
            _prefix[p++] = ' ';
            _prefix[p++] = ' ';
            _prefix[p++] = '|';
        }

        _prefix[ p++ ] = ' ';
        _prefix[ p ] = '\0';

    }

    return _prefix;


}

void __attribute__ ((no_instrument_function)) _timer_start( const char *name ){

    string_stack[ timer_stack_index ] = name;

    timer_stack_index++;

}

void __attribute__ ((no_instrument_function)) _timer_setlabel( const char *name ){

    timer_stack_index--;

    string_stack[ timer_stack_index ] = name;

    gettimeofday( &timer_stack[ timer_stack_index ], NULL );

    fprintf( _timer_fp ? _timer_fp : stderr, "%s- %s,,,,,\n", prefix( timer_stack_index ), name );

    timer_stack_index++;

}

void __attribute__ ((no_instrument_function)) _timer_end( void ){

    --timer_stack_index;

    if( string_stack[ timer_stack_index ] == NULL ) return;

	struct timeval start, end;

	start = timer_stack[ timer_stack_index ];
	gettimeofday( &end, NULL );

    long const first = ( start.tv_sec * 1000000 ) + start.tv_usec;
    long const last = ( end.tv_sec * 1000000 ) + end.tv_usec;

	long const diff = last - first;

	fprintf( _timer_fp ? _timer_fp : stderr, "%s,Total:,%ld,%ld,%ld,%ld,%ld,%s\n", prefix( timer_stack_index ), (long)start.tv_sec, (long)start.tv_usec, (long)end.tv_sec, (long)end.tv_usec, diff, string_stack[ timer_stack_index ] );

}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_enter(void *func,  void *caller) {
    (void)func;(void)caller;
    _timer_start( NULL );
}

void __attribute__ ((no_instrument_function)) __cyg_profile_func_exit(void *func, void *caller) {
    (void)func;(void)caller;
    _timer_end();
}

#endif
