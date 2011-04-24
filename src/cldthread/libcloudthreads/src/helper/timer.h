#ifdef PROFILE

#include <stdio.h>

/*

void _timer_start( const char *name );
void _timer_end( void );

#define TIMER_START( name ) _timer_start( #name );
#define TIMER_END() _timer_end();

*/

void _timer_setlabel( const char *name );
FILE * _timer_fp;

#define TIMER_LABEL( name ) _timer_setlabel( #name );

#else

/*
#define TIMER_START( name )
#define TIMER_END()
*/

#define TIMER_LABEL( name )

#endif

