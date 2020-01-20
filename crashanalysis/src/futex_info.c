// File: futex_info.c
 
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <linux/futex.h>
#include "unwind_libunwind.h"
#include "task_info.h"
#include <dlfcn.h>
#include "futex_info.h"
#include "list.h"


void
init_futex_info_list() {
	fi_list = (struct list_element *)list_init();
}

struct futex_info *
init_futex_info() {
	return (struct futex_info *)malloc(sizeof(struct futex_info));
}

struct list_element *
init_wait_info_list() {
	return (struct list_element *)list_init();
}

struct wait_info *
init_wait_info() {
	return (struct wait_info *)malloc(sizeof(struct wait_info));
}

static struct futex_info *
fid_to_futex_info (struct list_element *list, const void *futex_id) {
	if (futex_id == 0)
		return NULL;

	struct list_element *temp;
	struct futex_info *fi;
	temp=list->next;
	while (temp != NULL)
	{
		fi=(struct futex_info *)temp->element;
		if (fi->futex_id == futex_id)
		{
			return fi;
		}
		temp=temp->next;
	}
	return NULL;
}

truct futex_info *
add_futex_info (const void *futex_id) {
	struct futex_info *fi;

	fi = init_futex_info();
	
	fi->futex_id = futex_id;
	fi->active = 0;
	fi->num_times_acquired = 0;
	fi->num_times_waited = 0;
	fi->acquired_duration = 0;
	fi->waited_duration = 0;
	fi->lock_start_time = 0;
	fi->lock_stop_time = 0;
	
	fi->wait_list = init_wait_info_list();

	list_add_element (fi_list, fi);
	return fi;
}

truct futex_info *
get_futex_info (struct list_element *list, const void *futex_id) {
	struct futex_info *fi;
	fi = fid_to_futex_info(list, futex_id);
	return fi;
}

void
inc_num_times_acquired (struct futex_info *fi)
{
	fi->num_times_acquired++;
}	

void
inc_num_times_waited (struct futex_info *fi)
{
	fi->num_times_waited++;
}	

void
update_acquired_duration (struct futex_info *fi)
{
	if (fi->lock_stop_time >= fi->lock_start_time)
	{
		fi->acquired_duration += (fi->lock_stop_time - fi->lock_start_time);
	}	
}	

void
update_waited_duration (struct futex_info *fi, int val)
{	
	struct list_element *temp;
        struct wait_info *wi;
	int orig_val = val;
        temp=fi->wait_list->next;

        while (temp != NULL && val > 0)
        {
                wi=(struct wait_info *)temp->element;

		if (wi->wait_stop_time >= wi->wait_start_time)
		{
			fi->waited_duration += (wi->wait_stop_time - wi->wait_start_time);
		}	
                val--;
		temp=temp->next;
        }
        
	temp=fi->wait_list->next;

        while (temp != NULL && orig_val > 0)
        {
		wi=(struct wait_info *)temp->element;
		list_remove_element(fi->wait_list, temp);
		free(wi);
		temp=fi->wait_list->next;
		orig_val--;
        }
}

void
update_waited_duration_for_pid (struct futex_info *fi, pid_t pid)
{	
	struct list_element *temp;
        struct wait_info *wi;
        temp=fi->wait_list->next;

        while (temp != NULL)
        {
                wi=(struct wait_info *)temp->element;
		
		if(wi->pid == pid)
		{
			if (wi->wait_stop_time >= wi->wait_start_time)
			{
				fi->waited_duration += (wi->wait_stop_time - wi->wait_start_time);
			}

			list_remove_element(fi->wait_list, temp);
			break;
		}	
		temp=temp->next;
        }
}	

void
set_lock_start_time (struct futex_info *fi)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	fi->lock_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
	fi->active = 1;
}

void
set_lock_stop_time (struct futex_info *fi)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	fi->lock_stop_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
	fi->active = 0;
}

void
set_wait_start_time (struct futex_info *fi, pid_t pid)
{
	struct wait_info *wi;
	struct timespec ts;
	
	wi = init_wait_info();
	clock_gettime(CLOCK_MONOTONIC, &ts);
	wi->wait_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
	wi->wait_stop_time = 0;
	wi->pid = pid;

	list_add_element (fi->wait_list, wi);

}
void
set_wait_stop_time (struct futex_info *fi, int val)
{
	struct timespec ts;
	struct list_element *temp;
        struct wait_info *wi;
	
	clock_gettime(CLOCK_MONOTONIC, &ts);
	
        temp=fi->wait_list->next;

        while (temp != NULL && val > 0)
        {
                wi=(struct wait_info *)temp->element;
		wi->wait_stop_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
                val--;
		temp=temp->next;
        }

}

void
set_wait_stop_time_for_pid (struct futex_info *fi, pid_t pid)
{
	struct timespec ts;
	struct list_element *temp;
        struct wait_info *wi;
	
	clock_gettime(CLOCK_MONOTONIC, &ts);
	
        temp=fi->wait_list->next;

        while (temp != NULL)
        {
                wi=(struct wait_info *)temp->element;
		if (wi->pid == pid)
		{
			wi->wait_stop_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
			break;
		}
		temp=temp->next;
        }

}

void
print_futex(pid_t pid, void *uaddr, long OP, long val)
{
	printf("[%d] FUTEX (uaddr=%p, OP=", pid, uaddr);
	switch(OP)
	{
		case 0:
			printf("FUTEX_WAIT, val=%ld, ... )\n", val);
			break;
		case 1:
			printf("FUTEX_WAKE, val=%ld, ... )\n", val);
			break;
		case 5:
			printf("FUTEX_WAKE_OP, val=%ld, ... )\n", val);
			break;
		case 9:
			printf("FUTEX_WAIT_BITSET, val=%ld, ... )\n", val);
			break;
		case 10:
			printf("FUTEX_WAKE_BITSET, val=%ld, ... )\n", val);
			break;
		case 11:
			printf("FUTEX_WAIT_REQUEUE_PI, val=%ld, ... )\n", val);
			break;
		case 128:
			printf("FUTEX_WAIT_PRIVATE, val=%ld, ... )\n", val);
			break;
		case 129:
			printf("FUTEX_WAKE_PRIVATE, val=%ld, ... )\n", val);
			break;
		case 133:
			printf("FUTEX_WAKE_OP_PRIVATE, val=%ld, ... )\n", val);
			break;
		case 393:
			printf("FUTEX_CLOCK_REALTIME | FUTEX_WAIT_BITSET_PRIVATE, val=%ld, ...\n", val);
			break;
		default:
			printf("%ld, val=%ld, ... )\n", OP, val);
			break;

	}

}

void
handle_futex(int status, pid_t pid, int futex_call_count, int should_kill, void *uaddr, unsigned long OP, unsigned long val)
{
	struct timespec ts;
	struct task_info *ti;
	if ((ti = get_task_info(pid)) != NULL)
	{
		if(ti->insyscall == 0)
		{
			ti->insyscall = 1;
			futex_current_call_count++;
			
			printf("TRACER: Seen Futex # %d\n",futex_current_call_count);

			print_futex(pid, uaddr, OP, val);
			unwind_backtrace(get_task_info(pid));

			if ((OP == FUTEX_WAIT) ||
				(OP == FUTEX_WAIT_PRIVATE) ||
				(OP == (FUTEX_CLOCK_REALTIME | FUTEX_WAIT_BITSET_PRIVATE)))
			{
				mf_i.wait_count++;
			}
			else if ((OP == FUTEX_WAKE) ||
					(OP == FUTEX_WAKE_OP) ||
					(OP == FUTEX_WAKE_PRIVATE) ||
					(OP == FUTEX_WAKE_OP_PRIVATE))
			{
				mf_i.wake_count++;
			}
			 
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ti->syscall_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
		}
		else
		{
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ti->syscall_end_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
			printf("[%d] FUTEX EXITED (...)\n", pid);
			ti->insyscall = 0;
			if ((OP == FUTEX_WAIT) ||
				(OP == FUTEX_WAIT_PRIVATE) ||
				(OP == (FUTEX_CLOCK_REALTIME | FUTEX_WAIT_BITSET_PRIVATE)))
			{
				mf_i.wait_duration += ti->syscall_end_time - ti->syscall_start_time;
			}
			else if ((OP == FUTEX_WAKE) ||
					(OP == FUTEX_WAKE_OP) ||
					(OP == FUTEX_WAKE_PRIVATE) ||
					(OP == FUTEX_WAKE_OP_PRIVATE))
			{
				mf_i.wake_duration += ti->syscall_end_time - ti->syscall_start_time;
			}
		}
	}

}

void
print_futex_stats ()
{
	struct list_element *temp;
  struct futex_info *fi;
	unsigned long total_num_times_acquired = 0;
	unsigned long total_num_times_waited = 0;
	unsigned long total_acquired_duration = 0;
	unsigned long total_waited_duration = 0;

  temp=fi_list->next;
	printf("Futex Stats:\n");
	printf("---------------------------------------------------\n");
	printf("FutexID \t Count \t WCount \t Time \t\t Wtime\n");
	printf("---------------------------------------------------\n");
		

  while (temp != NULL)
  {
    fi=(struct futex_info *)temp->element;
    printf("%p \t %5ld \t %5ld \t %9ld \t %9ld\n",fi->futex_id, fi->num_times_acquired, fi->num_times_waited, fi->acquired_duration, fi->waited_duration);
	total_num_times_acquired += fi->num_times_acquired;
	total_num_times_waited += fi->num_times_waited;
	total_acquired_duration += fi->acquired_duration;
	total_waited_duration += fi->waited_duration;
    temp=temp->next;
  }
	printf("---------------------------------------------------\n");
    printf("Total \t\t %5ld \t %5ld \t %9ld \t %9ld\n", total_num_times_acquired, total_num_times_waited, total_acquired_duration, total_waited_duration);
	printf("---------------------------------------------------\n");
	
}

void
remove_futex_info_list() {
	list_free(fi_list);
}
