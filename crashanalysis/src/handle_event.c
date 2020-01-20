// File: process_event.c


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <linux/futex.h>
#include "unwind_libunwind.h"
#include "task_info.h"
#include "futex_info.h"
#include "aio_info.h"
#include <dlfcn.h>


unsigned int num_tasks;
#define SIGTRACE 60

static void
handle_kernel_signal(int status, pid_t pid)
{
	if (WSTOPSIG(status) == SIGTRACE)
	{
			printf("TRACER: kernel signal received\n");
		
		if(get_task_info(pid) != NULL)
		{
				printf("TRACER: Stack Backtrace is:\n");
			unwind_backtrace(get_task_info(pid));
		}
		else
		{
				printf("TRACER: task_info for task not available.\n");
				printf("TRACER: Unable to generate Stack Backtrace.\n");
		}
	}
	
}


static int
handle_exit (int status, pid_t pid)
{
	if (DEBUG)
		printf("TRACER: A task has exited. PID: %d\n",pid);
	if (WIFEXITED (status) && (WEXITSTATUS(status) != 0))
	{
		if (DEBUG)
			printf ("TRACER: task exit status %d\n", WEXITSTATUS (status));
	}

	if (WIFSIGNALED(status))
	{
		if (DEBUG)
			printf ("TRACER: task exited using signal %d\n", WTERMSIG(status));
	}
			
	if ((remove_task_info(pid)) > 0)
	{
		if (DEBUG)
			printf("TRACER: Task info successfully removed, pid: %d\n",pid);
	}
	else
		if (DEBUG)
			printf("TRACER: Unable to remove task info, pid: %d\n",pid);
	num_tasks--;
	if (DEBUG)
		printf("TRACER: Number of tasked traced: %d\n", num_tasks);
		
	if (num_tasks == 0)
	{
		if (DEBUG)
			printf("TRACER: Task count reached zero\n");
		if (check_last_task_info_removed() > 0)
		{
			if (DEBUG)
				printf("TRACER: Task list is also empty. Lets exit\n");
			return 0;
		}
		else
		{
			if (DEBUG)
				printf("TRACER: Task list is not empty. Some processes exited without reporting\n");
			return 1;
		}
	}
	return 1;
}

static void
handle_event_exit (int status, pid_t pid)
{
	if (DEBUG)
		printf("TRACER: A task will exit in a while: %d\n",pid);
	
	if (DEBUG)
		printf("TRACER: Removing its task info now.\n");
	if (remove_task_info(pid) > 0)
	{
		if (DEBUG)
			printf("TRACER: Successfully removed task info\n");
	}
	else
		if (DEBUG)
			printf("TRACER: Unable to find task info with pid: %d\n",pid);
	num_tasks--;
	if (DEBUG)
		printf("TRACER: Number of tasked traced: %d\n", num_tasks);
		
	if (num_tasks == 0)
	{
		if (DEBUG)
			printf("TRACER: Task count reached zero\n");
		ptrace(PTRACE_DETACH, pid, 0L, 0L);
	}
	else
		ptrace(PTRACE_CONT, pid, 0, 0);
}

static void
handle_stop (int status, pid_t pid)
{
	siginfo_t si;
	if (ptrace(PTRACE_GETSIGINFO, pid, 0, &si) < 0)
	{
		if (DEBUG)
			printf("TRACER: handle_stop(): group stop\n");
		ptrace(PTRACE_SYSCALL, pid, 0, WSTOPSIG(status));
	}
	else
	{
		if (DEBUG)
			printf("TRACER: Handle_stop(): Signal delivery stop\n");
		ptrace(PTRACE_SYSCALL, pid, 0, WSTOPSIG(status));
	}
}

static void
handle_clone (int status, pid_t pid)
{
	pid_t new_child;
	unsigned int ptrace_setoptions = PTRACE_O_TRACESYSGOOD
																		| PTRACE_O_TRACEEXEC
																		| PTRACE_O_TRACEEXIT
																		| PTRACE_O_TRACECLONE
																		| PTRACE_O_TRACEFORK
																		| PTRACE_O_TRACEVFORK;
	if (DEBUG)
		printf("TRACER: New child created, get PID\n");
		
	if(ptrace(PTRACE_GETEVENTMSG, pid, 0, &new_child) != -1)
	{        
		if (DEBUG)
			printf("TRACER: Child PID is: %d\n", new_child);

		if (pid != new_child)
		{
			add_task_info (new_child);

			num_tasks++;
			if (DEBUG)
				printf("TRACER: Number of tasks being traced: %d\n",num_tasks);
			
			ptrace(PTRACE_SETOPTIONS, new_child, NULL, ptrace_setoptions);
			ptrace(PTRACE_SYSCALL, new_child, 0, 0);
			if (DEBUG)
       	printf("TRACER: New child tracing started: %d\n", new_child);
		}
	}
	if (DEBUG)
		printf("TRACER: Continuing parent process\n");
	fflush(stdout);	
	ptrace(PTRACE_SYSCALL, pid, 0, 0);
	if (DEBUG)
		printf("TRACER: Parent process tracing resumed\n");
	fflush(stdout);
}

void
handle_syscall(int status, pid_t pid, int futex_call_count, int should_kill)
{
	unsigned long OP, val;
	void *uaddr;
	struct user_regs_struct regs;
	struct timespec ts;
	struct task_info *ti;

	ptrace(PTRACE_GETREGS, pid, 0L, &regs);
	if(DEBUG)
		printf("TRACER: Syscall received: %llx\n",regs.orig_rax);
	if(regs.orig_rax == SYS_futex)
	{
		futex_current_call_count++;
		uaddr = (void *) regs.rdi;
		OP = regs.rsi;
		val = regs.rdx;

		handle_futex(status, pid, futex_call_count, should_kill, uaddr, OP, val);
		
	}
	else if (regs.orig_rax == SYS_write)
	{
		if ((ti = get_task_info(pid)) != NULL)
		{
			if(ti->insyscall == 0)
			{
				printf("[%d] WRITE (...)\n", pid);
				ti->insyscall = 1;
				clock_gettime(CLOCK_MONOTONIC, &ts);
				ti->syscall_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
				write_i.count++;
			}
			else
			{
				printf("[%d] WRITE END (...)\n", pid);
				ti->insyscall = 0;
				clock_gettime(CLOCK_MONOTONIC, &ts);
				ti->syscall_end_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
				write_i.duration += ti->syscall_end_time - ti->syscall_start_time;
			}

	}
	else if (regs.orig_rax == SYS_io_submit)
	{
		handle_io_submit(status, pid);
	}
	else if	(regs.orig_rax == SYS_io_getevents)
	{
		handle_io_getevents(status, pid);
	}
	ptrace(PTRACE_SYSCALL, pid, 0, 0);
}



void
handle_events (int futex_call_count, int should_kill)
{
	int status, pid;
 	unsigned int	sig, event;
	starttime = 0;
	endtime = 0;	
	
	num_tasks=1;
	futex_current_call_count = 0;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	starttime = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
	while (1)
	{
		if (num_tasks > 0)
		{
			pid = waitpid(-1, &status, __WALL);
			if (pid < 0)
			{
				if (errno == EINTR)
					continue;

				if (DEBUG)
					printf ("TRACER: waitpid() failed (errno=%d)\n", errno);
				if (errno == ECHILD)
					if (DEBUG)
						printf ("TRACER: No running task left. Exiting\n");
				break;
			}
		}
		else
		{
			if (DEBUG)
				printf("TRACER: No running tasks left. Exiting\n");
			break;
		}

		if (!WIFSTOPPED(status))
			continue;
	
		if (DEBUG)
			printf("TRACER: Got a signal for task with pid: %d\n",pid);
	
		if (WIFSIGNALED(status) || WIFEXITED(status))
		{
			if (DEBUG)
				printf("TRACER: handle_exit()\n");
			fflush(stdout);
			if (handle_exit(status, pid) > 0)
				continue;
			else
				break;
		}

		sig = WSTOPSIG(status);
		event = (unsigned int) status >> 16;
		if (DEBUG)
		{
			printf("TRACER: sig=%d and event=%d\n",sig, event);
		}

		switch (event)
		{
		case 0:
			if (DEBUG)
				printf("TRACER: Case 0\n");
			if (sig == SIGTRACE)
			{
				if (DEBUG)
					printf("TRACER: We received Signal from kernel\n");
				handle_kernel_signal(status, pid);
				kill(pid, SIGKILL);
				break;
			}

			else if (sig == (SIGTRAP | 0x80))
			{
				if (DEBUG)
					printf("TRACER: Case 0: SIGTRAP\n");

				handle_syscall(status, pid, futex_call_count, should_kill);
			}
			else
			{
				if (DEBUG)
					printf ("TRACER: Case 0: Actual stop signal\n");
				handle_stop(status, pid);
			}
			break;
		case PTRACE_EVENT_CLONE:
		case PTRACE_EVENT_FORK:
		case PTRACE_EVENT_VFORK:
				printf ("TRACER: Case PTRACE_EVENT_(CLONE,FORK,VFORK)\n");
			handle_clone(status, pid);
			break;
		case PTRACE_EVENT_EXIT:
			if (DEBUG)
				printf ("TRACER: Case PTRACE_EVENT_EXIT\n");

			handle_event_exit(status, pid);
			break;
		case PTRACE_EVENT_EXEC:
				printf ("TRACER: Case PTRACE_EVENT_EXEC\n");
			ptrace(PTRACE_SYSCALL, pid, 0, 0);
			break;
		case PTRACE_EVENT_STOP:

			if (DEBUG)
				printf ("TRACER: Case PTRACE_EVENT_STOP\n");
			switch (sig)
			{
			case SIGSTOP:
			case SIGTSTP:
			case SIGTTIN:
			case SIGTTOU:
				if (DEBUG)
					printf ("TRACER: Case PTRACE_EVENT_STOP: SIGSTOP\n");
				handle_stop(status, pid);
				break;
			default:
				if (DEBUG)
					printf ("TRACER: Case PTRACE_EVENT_STOP: Interrupted\n");
				ptrace (PTRACE_SYSCALL, pid, 0, 0);
			}
			break;
		default:
			if (DEBUG)
				printf("TRACER: Case Default\n");
			ptrace(PTRACE_SYSCALL, pid, 0, 0);
		}
	}
	if (endtime == 0)
	{
		clock_gettime(CLOCK_MONOTONIC, &ts);
		endtime = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
		printf("Execution time = %9ld nanoseconds\n",endtime - starttime);
	}
}

