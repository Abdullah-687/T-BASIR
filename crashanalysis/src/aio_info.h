// File: aio_info.h
#ifndef AIO_INFO_H
#define AIO_INFO_H

#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "defs.h"

struct aio_info aio_i;

void
init_aio_info_struct();


void
handle_io_submit(int , pid_t);

void
handle_io_getevents(int , pid_t);

void
print_aio_stats_and_remove_lists();

#endif 
