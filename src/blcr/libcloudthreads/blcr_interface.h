/* Enums and global variables */

enum blcr_state {
    blcr_error=0,
    blcr_continue,
    blcr_restart
};

int blcr_init_framework( void );
int blcr_checkpoint( const char *filepath, void(*fptr)(void), void(*fptr2)(void) );
char* blcr_generate_context_filename(void);

int BLCR_CRFLAGS;
int BLCR_CRSCOPE;

