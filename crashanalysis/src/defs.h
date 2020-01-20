//File: defs.h


#ifndef TRACER_DEFS_H
#define TRACER_DEFS_H
#include <stdlib.h>
#include <stdbool.h>
#include <linux/aio_abi.h>

#define DEBUG 0


struct list_element {
	void * element;
	void * next;
};

struct task_info {
	pid_t pid;
	unsigned int signalCount;
	bool started;
	unsigned int insyscall;
	unsigned int infutexwait;
	unsigned long syscall_start_time;
	unsigned long syscall_end_time;
	void *futex_uaddr;
	struct UPT_info *ui;
};

struct futex_info {
	void * futex_id;
	unsigned int active;
	unsigned long num_times_acquired;
	unsigned long num_times_waited;
	unsigned long acquired_duration;
	unsigned long waited_duration;
	unsigned long lock_start_time;
	unsigned long lock_stop_time;
	struct list_element *wait_list;
};

struct wait_info {
	pid_t pid;
	unsigned long wait_start_time;
	unsigned long wait_stop_time;
};


struct iocb_info {
	void * ctx;
	struct iocb cb;
	unsigned long submit_time;
	unsigned long complete_time;
};

struct fd_info {
	int fd;
	void * ctx;
	unsigned long data;
	unsigned int read_count;
	unsigned int write_count;
	unsigned long read_duration;
	unsigned long write_duration;
};

struct aio_info {
	unsigned int submit_count;
	unsigned long submit_duration;
	unsigned int getevents_count;
	unsigned long getevents_duration;
	struct list_element *fd_list;
	struct list_element *iocb_list;
};


struct mini_futex_info {
	unsigned int wait_count;
	unsigned long wait_duration;
	unsigned int wake_count;
	unsigned int wake_duration;
};

struct mini_futex_info mf_i;
struct write_info {
	unsigned int count;
	unsigned long duration;
};

struct write_info write_i;


unsigned long starttime;
unsigned long endtime;
unsigned int futex_current_call_count;


#endif 
