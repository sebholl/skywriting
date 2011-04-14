/* Enums and global variables */

enum blcr_state {
    blcr_error=0,
    blcr_continue,
    blcr_restart
};

int blcr_init_framework( void );
int blcr_fork( const char *filepath );
char* blcr_generate_context_filename(void);
