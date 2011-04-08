#include "cielID.h"

cielID * _ciel_get_task_id( void );

void _ciel_set_task_id( cielID *const new_id );

int _ciel_spawn_chkpt_task( cielID *new_task_id, cielID *output_task_id,
                            cielID *input_id[], size_t input_count,
                            int is_continuation );


