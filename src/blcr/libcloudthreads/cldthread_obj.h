#include <inttypes.h>

typedef struct cldthread_obj {

    enum cldthread_result_type { NONE=0, INTEGER, REAL, STRING, BINARY } type;

    union {
        intmax_t integer;
        long double real;
        char* string;
        void* data;
    } value;

    size_t size;

} cldthread_obj;

void *cldthread_exit( cldthread_obj *exit_value );

cldthread_obj *cldthread_none( void );
cldthread_obj *cldthread_integer( intmax_t intgr );
cldthread_obj *cldthread_real( long double dbl );
cldthread_obj *cldthread_string( const char *str );
cldthread_obj *cldthread_data( void *data, size_t size );

char *cldthread_serialize_obj( cldthread_obj *obj, void *default_value );
cldthread_obj *cldthread_deserialize_obj( const char *output_id );

inline intmax_t cldthread_obj_to_intmax( cldthread_obj *obj );
inline long cldthread_obj_to_long( cldthread_obj *obj );
inline int cldthread_obj_to_int( cldthread_obj *obj );
inline double cldthread_obj_to_double( cldthread_obj *obj );
inline float cldthread_obj_to_float( cldthread_obj *obj );
inline const char * cldthread_obj_to_string( cldthread_obj *obj );
inline void * cldthread_obj_to_data( cldthread_obj *obj, size_t *size );
