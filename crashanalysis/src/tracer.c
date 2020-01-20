// File: tracer.c



#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <dirent.h>
#include <time.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/user.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include "unwind_libunwind.h"
#include "task_info.h"
#include "futex_info.h"
#include "aio_info.h"

extern char **environ;
extern int optind;
extern char *optarg;

void handle_events(int, int);

void
sig_handler(int sig)
{
	pid_t pid;
	struct timespec ts;
	if (sig == SIGINT)
	{
		printf("Termination signal received, detaching tracee...\n");
 
		while (check_last_task_info_removed() == 0)
		{
			pid = get_first_pid();

			ptrace(PTRACE_DETACH, pid, 0L, 0L);
			printf("Detached task %d\n", pid);

			remove_task_info(pid);
		}

 		remove_task_info_list();
		
		if (endtime == 0)
		{
			clock_gettime(CLOCK_MONOTONIC, &ts);
			endtime = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
			printf("Execution time = %9ld nanoseconds\n",endtime - starttime);
		}
 		printf("FUTEX SYSCALL STATISTICS\n");
		printf("Number of WAIT calls = %d\n", mf_i.wait_count);
		printf("Total Duration = %lu\n", mf_i.wait_duration);
		printf("Number of WAKE calls = %d\n", mf_i.wake_count);
		printf("Total Duration = %u\n", mf_i.wake_duration);
		printf("----------------------------------\n");
		printf("WRITE SYSCALL STATISTICS\n");
		printf("Number of times called = %d\n", write_i.count);
		printf("Total Duration = %lu\n", write_i.duration);	
		print_aio_stats_and_remove_lists();		
		fflush(stdout);
		fflush(stderr);
		exit(0);
	}
}
				
int
main (int argc, char **argv)
{
	int status, c, pid, attach=0, child_pid;
	int futex_call_count=0, should_kill=0;
 
	unsigned int ptrace_setoptions = PTRACE_O_TRACESYSGOOD  
									| PTRACE_O_TRACEEXIT 
									| PTRACE_O_TRACECLONE 
									| PTRACE_O_TRACEFORK 
									| PTRACE_O_TRACEVFORK
									| PTRACE_O_TRACEEXEC; 

	
	if (argc < 2)
	{
		printf("USAGE: tracer [p c k] <tracee> <arguments to tracee>\n");
		exit(1);
	}

	if (DEBUG)
		printf("TRACER: checking if we have to trace already running process\n");
	while((c = getopt(argc, argv, "p:k:c:")) !=EOF)
	{
		switch(c)
		{
			case 'p':
				if(DEBUG)
					printf("TRACER: -p supplied, getting PID of process\n");
				attach = 1;
				pid = atoi(optarg);
				if (DEBUG)
					printf("TRACER: Tracee PID is %d\n", pid);
				break;
			case 'k':
				if (DEBUG)
					printf("TRACER: -k supplied, setting kill variable\n");
				should_kill = 1;
				break;
			case 'c':
				if (DEBUG)
					printf("TRACER: -c supplied, setting futex syscall count\n");
				futex_call_count = atoi(optarg);
				break;
			default:
				break;
		}
	}
 
	
	unwind_create_address_space();

 
	if (attach == 0)
	{
		pid = fork ();
		if (!pid)
		{
 
			optind = 1;
			ptrace (PTRACE_TRACEME, 0, NULL, NULL);
 
			kill(getpid(), SIGSTOP);
 
			if (DEBUG)
				printf("TRACER: Starting tracee execution using execve\n"); 
			fflush(stdout);
			execve (argv[optind], argv+optind, environ);
 
			_exit (-1);
		}
	}
	else
	{
		if (DEBUG)
			printf("TRACER: Attaching tracer to pid: %d\n", pid);
 
		if (signal(SIGINT, sig_handler) == SIG_ERR)
		{
  		printf("\nUnable to register signal handler. Cannot continue...\n");
			exit(-1);
		}
		
 		ptrace(PTRACE_ATTACH, pid, 0L, 0L);
	}
	
 
	
	child_pid = waitpid(pid, &status, WSTOPPED);
	if (child_pid != -1)

    {
		if (DEBUG)
			printf("TRACER: Tracee has called stop on itself: %d\n",child_pid);
		
		ptrace(PTRACE_SETOPTIONS, child_pid, NULL, ptrace_setoptions);
		
 		init_task_info_list();

        init_futex_info_list();
			

        add_task_info (child_pid);
			

        ptrace(PTRACE_SYSCALL, child_pid, 0L, 0L);
		if(attach == 1)
		{

            char path[] = "/proc/%d/task";
      char procdir[sizeof(path) + sizeof(int) * 3];
      DIR *dir;
     
			sprintf(procdir, path, pid);
			dir = opendir(procdir);
			struct dirent *de;

			while ((de = readdir(dir)) != NULL) 
			{
				if (de->d_fileno == 0)
					continue;

				int tid = atoi(de->d_name);
				if (tid <= 0 || tid == pid)
					continue;

				ptrace(PTRACE_ATTACH, tid, 0L, 0L);

                
				child_pid = waitpid(tid, &status, WSTOPPED);
				if (child_pid == tid)
				{
					if (DEBUG)
						printf("TRACER: Child process has called stop on itself: %d\n",child_pid);
		

                    ptrace(PTRACE_SETOPTIONS, child_pid, NULL, ptrace_setoptions);
			

                    add_task_info (child_pid);
			
					ptrace(PTRACE_SYSCALL, child_pid, 0L, 0L);
				}
			}
			closedir(dir);

		}
		if (DEBUG)
			printf("TRACER: Tracing on child process has started\n");
	}
	else
	{
		if (DEBUG)
		{
			printf("TRACER: We have received a PID that is not Child\n");
			printf("TRACER: No tracing will be done, therefore, exiting\n");
		}
		exit(-1);
	}


	write_i.count = 0;
	write_i.duration = 0;
	mf_i.wait_count = 0;
	mf_i.wait_duration = 0;	
	mf_i.wake_count = 0;
	mf_i.wake_duration = 0;	
	init_aio_info_struct();

    handle_events(futex_call_count, should_kill);

    
	printf("FUTEX SYSCALL STATISTICS\n");
	printf("Number of WAIT calls = %d\n", mf_i.wait_count);
	printf("Total Duration = %lu\n", mf_i.wait_duration);
	printf("Number of WAKE calls = %d\n", mf_i.wake_count);
	printf("Total Duration = %u\n", mf_i.wake_duration);
	printf("----------------------------------\n");
	printf("WRITE SYSCALL STATISTICS\n");
	printf("Number of times called = %d\n", write_i.count);
	printf("Total Duration = %lu\n", write_i.duration);
	print_aio_stats_and_remove_lists();	
	printf ("TRACER: SUCCESS\n");
	return 0;
}

