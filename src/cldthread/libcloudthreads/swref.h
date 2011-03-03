#pragma once

#include <stdint.h>
#include "cldvalue.h"
#include "cJSON.h"

static const char * const swref_map_type_tuple_id[] = { "err",
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

    const char *ref_id;
    int fd;
    uintmax_t size;
    const char **loc_hints;
    uintmax_t loc_hints_size;
    const cldvalue *value;

} swref;

swref *swref_create( enum swref_type type, const char *ref_id, const cldvalue *value, uintmax_t size, const char *loc_hint );
void swref_fatal_merge( swref *receiver, swref *sender );
void swref_free( swref *ref );

cJSON *swref_serialize( const swref *ref );
swref *swref_deserialize( cJSON* json );
