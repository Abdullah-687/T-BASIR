// File: task_info.h

#ifndef TASK_INFO_H
#define TASK_INFO_H

#include <stdlib.h>
#include <stdbool.h>
#include "list.h"
#include "defs.h"

struct list_element *ti_list;

void
init_task_info_list();

struct task_info *
init_task_info();


void
add_task_info (const pid_t);

bool
get_started_task_info (const pid_t);

void
set_started_task_info (const pid_t);

struct task_info *
get_task_info (const pid_t);


pid_t
get_first_pid ();


int
remove_task_info (const pid_t);


int
check_last_task_info_removed();

void
remove_task_info_list();
#endif

