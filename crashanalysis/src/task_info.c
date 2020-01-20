// File: task_info.c

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <libunwind-ptrace.h>
#include "task_info.h"
#include "list.h"
void
init_task_info_list() {
	ti_list = (struct list_element *)list_init();
}

struct task_info *
init_task_info() {
	return (struct task_info *)malloc(sizeof(struct task_info));
}

static struct task_info *
pid_to_task_info (struct list_element *list, const pid_t pid) {
	if (pid == 0)
		return NULL;

	struct list_element *temp;
	struct task_info *ti;
	temp=list->next;
	while (temp != NULL)
	{
		ti=(struct task_info *)temp->element;
		if (ti->pid == pid)
		{
			return ti;
		}
		temp=temp->next;
	}
	return NULL;
}

static struct list_element *
pid_to_list_element (struct list_element *list, const pid_t pid) {
	if (pid == 0)
		return NULL;

	struct list_element *temp;
	struct task_info *ti;
	temp=list->next;
	while (temp != NULL)
	{
		ti=(struct task_info *)temp->element;
		if (ti->pid == pid)
		{
			return temp;
		}
		temp=temp->next;
	}
	return NULL;
}


void
add_task_info (const pid_t pid) {
	struct task_info *ti;
	struct timespec ts;
	ti = init_task_info();
	ti->pid = pid;
	ti->started = false;
	ti->signalCount = 0;
	ti->insyscall = 0;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	ti->syscall_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
	ti->syscall_end_time = 0;
	ti->ui = _UPT_create (pid);
	list_add_element (ti_list, ti);
}

bool
get_started_task_info (const pid_t pid) {
	struct task_info *ti;
	ti = pid_to_task_info(ti_list, pid);
	return ti->started;
}

void
set_started_task_info (const pid_t pid) {
	struct task_info *ti;
	ti = pid_to_task_info(ti_list, pid);
	ti->started = true;
}

struct task_info *
get_task_info (const pid_t pid) {
	struct task_info *ti;
	ti = pid_to_task_info(ti_list, pid);
	return ti;
}


pid_t
get_first_pid ()
{
	struct task_info *ti;
	struct list_element *temp;
	temp=ti_list->next;
	ti=(struct task_info *)temp->element;
	return ti->pid;
}


int
remove_task_info (const pid_t pid) {
	struct task_info *ti;
	struct list_element *le;

	ti = pid_to_task_info(ti_list, pid);

	if (ti == NULL)
		return 0;
	

	le = pid_to_list_element(ti_list, pid);

	list_remove_element(ti_list, le);

	_UPT_destroy (ti->ui);

	free(ti);

	return 1;
}


int
check_last_task_info_removed() {
	if (check_if_list_empty(ti_list) > 0)
		return 1;
	else 
		return 0;
}

void
remove_task_info_list() {
	list_free(ti_list);
}
