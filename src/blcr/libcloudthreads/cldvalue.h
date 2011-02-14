#include <inttypes.h>

typedef struct cldvalue {

    enum cldthread_result_type { NONE=0, INTEGER, REAL, STRING, BINARY } type;

    union {
        intmax_t integer;
        long double real;
        char* string;
        void* data;
    } value;

    size_t size;

} cldvalue;

void *cldthread_exit( cldvalue *exit_value );

cldvalue *cldvalue_none( void );
cldvalue *cldvalue_integer( intmax_t intgr );
cldvalue *cldvalue_real( long double dbl );
cldvalue *cldvalue_string( const char *str );
cldvalue *cldvalue_data( void *data, size_t size );

char *cldvalue_serialize( cldvalue *obj, void *default_value );
cldvalue *cldvalue_deserialize( const char *output_id );

inline intmax_t cldvalue_to_intmax( const cldvalue *obj );
inline long cldvalue_to_long( const cldvalue *obj );
inline int cldvalue_to_int( const cldvalue *obj );
inline double cldvalue_to_double( const cldvalue *obj );
inline float cldvalue_to_float( const cldvalue *obj );
inline const char * cldvalue_to_string( const cldvalue *obj );
inline void * cldvalue_to_data( const cldvalue *obj, size_t *size );
