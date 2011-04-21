/* SWReference Equivalent */

#pragma once

#include <stdint.h>

#include "../lib/cJSON/cJSON.h"

#include "cielID.h"

#include "cldvalue.h"

struct cldvalue;

typedef struct swref {

    enum swref_type { SWREFTYPE_ENUMMIN = 0,
                      ERROR = SWREFTYPE_ENUMMIN,
                      FUTURE = 1,
                      CONCRETE = 2,
                      STREAMING = 3,
                      TOMBSTONE = 4,
                      FETCH = 5,
                      URL = 6,
                      DATA  = 7,
                      OTHER,
                      SWREFTYPE_ENUMMAX = OTHER} type;

    cielID *id;
    uintmax_t size;
    char **loc_hints;
    size_t loc_hints_size;
    struct cldvalue *value;

} swref;

static const char *const swref_map_type_tuple_id[] = {    "err",
                                                          "f2",
                                                          "c2",
                                                          "s2",
                                                          "t2",
                                                          "fetch2",
                                                          "urls",
                                                          "val",
                                                          "" };


swref *                  swref_create( enum swref_type type,
                                       const char *id,
                                       struct cldvalue *value,
                                       uintmax_t size,
                                       const char *loc_hint );

void                     swref_fatal_merge( swref *receiver, swref *sender );

void                     swref_free( swref *ref );
void                     swref_free_ex( swref *ref, int keepcldvalue );

cJSON *                  swref_serialize( const swref *ref );
swref *                  swref_deserialize( cJSON *json );

const struct cldvalue *  swref_to_cldvalue( const swref *ref );
char *                   swref_to_data   ( const swref *ref, size_t *size_out );

swref *                  swref_at_id( cielID *id );
cielID *                 cielID_of_swref( const swref *ref );

swref *                  concrete_swref_for_cielID( cielID *id );
swref *                  future_swref_for_cielID( cielID *id );

#define ASPRINTF_ORNULL( checkpoint, args... ) if( asprintf( checkpoint, args ) == -1 ) *checkpoint = NULL
#define ASPRINTF_ORDIE( function_name, checkpoint, args... ) \
    if( asprintf( checkpoint, args ) == -1 ) { \
        perror( #function_name ": could not allocate string.\n" ); \
        exit( EXIT_FAILURE ); \
    } \



