// File: futex_info.h

#ifndef FUTEX_INFO_H
#define FUTEX_INFO_H

#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "defs.h"

struct list_element *fi_list;

void
init_futex_info_list();

struct futex_info *
init_futex_info();

struct futex_info *
add_futex_info (const void *);

struct futex_info *
get_futex_info (struct list_element *, const void *);

void
inc_num_times_acquired (struct futex_info *);

void
inc_num_times_waited (struct futex_info *);

void
update_acquired_duration (struct futex_info *);

void
update_waited_duration (struct futex_info *, int);

void
update_waited_duration_for_pid (struct futex_info *, pid_t);

void
set_lock_start_time (struct futex_info *);

void
set_lock_stop_time (struct futex_info *);

void
set_wait_start_time (struct futex_info *, pid_t);

void
set_wait_stop_time (struct futex_info *, int);

void
set_wait_stop_time_for_pid (struct futex_info *, pid_t);

void
handle_futex(int, pid_t, int, int, void *, unsigned long, unsigned long);
void
print_futex_stats();

#endif 
