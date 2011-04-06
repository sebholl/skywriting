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

void cielID_close_fd( cielID *id );

int cielID_publish_stream( cielID *id );

int cielID_finalize_stream( cielID *id );


int cielID_read_stream( cielID *id );

size_t cielID_read_streams( cielID *id[], size_t const count );


char *cielID_dump_stream( cielID *id, size_t *size_out );
