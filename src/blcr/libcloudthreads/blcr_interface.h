/* Enums and global variables */

enum blcr_state {
    blcr_error=0,
    blcr_continue,
    blcr_restart,
};

void blcr_init_framework( void );
int blcr_spawn_function( const unsigned char *filepath, void(*fptr)(void), void(*fptr2)(void) );
unsigned char* blcr_generate_context_filename(void);


