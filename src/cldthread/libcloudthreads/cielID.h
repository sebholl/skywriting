#pragma once

#include <stdlib.h>

typedef struct cielID {

   char *id_str;
   int fd;

} cielID;


/* General API */

cielID *cielID_create( const char *id_str );
cielID *cielID_create2( char *id_str );

void cielID_free( cielID *id );


/* Reading from streams */

int cielID_read_stream( cielID *id );

char *cielID_dump_stream( cielID *id, size_t *size_out );

void cielID_close_stream( cielID *id );


/* API for creating streams */

int cielID_publish_stream( cielID *id );

int cielID_finalize_stream( cielID *id );
