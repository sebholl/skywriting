/* SWReference Equivalent */

#pragma once

#include <stdint.h>
#include "cldvalue.h"
#include "cJSON.h"
#include "cielID.h"

static const char *const swref_map_type_tuple_id[] = { "err",
                                          "f2",
                                          "c2",
                                          "s2",
                                          "t2",
                                          "fetch2",
                                          "urls",
                                          "val",
                                          "" };

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
    const char **loc_hints;
    uintmax_t loc_hints_size;
    cldvalue *value;

} swref;


swref *      swref_create( enum swref_type type, const char *id, cldvalue *value, uintmax_t size, const char *loc_hint );
void         swref_fatal_merge( swref *receiver, swref *sender );
void         swref_free( swref *ref );

cJSON *      swref_serialize( const swref * const ref );
swref *      swref_deserialize( cJSON *json );

intmax_t     swref_to_intmax ( const swref *ref );
double       swref_to_double ( const swref *ref );
const char * swref_to_string ( const swref *ref );

swref *      swref_at_id( cielID *id );
cielID *     cielID_of_swref( const swref *ref );
