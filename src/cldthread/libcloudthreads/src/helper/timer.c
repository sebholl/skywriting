#ifdef PROFILE

#include <stdio.h>
#include <time.h>

#include "timer.h"

#define STACK_SIZE 200

FILE* _timer_fp = NULL;

static const char * string_stack[STACK_SIZE];
static struct timespec timer_stack[STACK_SIZE];
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

    clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &timer_stack[ timer_stack_index ] );

    fprintf( _timer_fp ? _timer_fp : stderr, "%s- %s,,,,,\n", prefix( timer_stack_index ), name );

    timer_stack_index++;

}

void __attribute__ ((no_instrument_function)) _timer_end( void ){

    --timer_stack_index;

    if( string_stack[ timer_stack_index ] == NULL ) return;

	struct timespec start, end;

	start = timer_stack[ timer_stack_index ];
	clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &end);

    long const first = ( start.tv_sec * 1000000000 ) + start.tv_nsec;
    long const last = ( end.tv_sec * 1000000000 ) + end.tv_nsec;

	long const diff = last - first;

	long const sec = diff / 1000000000;
	long const nano = diff % 1000000000;

	fprintf( _timer_fp ? _timer_fp : stderr, "%s,Total:,%ld,%ld,%ld,%s\n", prefix( timer_stack_index ), sec, nano, diff, string_stack[ timer_stack_index ] );

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
