#pragma once

#include <stdint.h>

static const char * const swref_map_type_tuple_id[] = { "err",
                                          "f2",
                                          "c2",
                                          "s2",
                                          "t2",
                                          "fetch2",
                                          "urls",
                                          "val" };

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
    uintmax_t size;
    const char **loc_hints;

} swref;

swref *sw_create_ref( enum swref_type type, const char *ref_id, uintmax_t size, const char *loc_hint );
void sw_fatal_merge_ref( swref *receiver, swref *sender );
void sw_free_ref( swref *ref );

char *sw_serialize_ref( const swref *ref );
swref *sw_deserialize_ref( const char *data );
