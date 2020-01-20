//File: aio_info.c

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <linux/aio_abi.h>
#include "aio_info.h"
#include "task_info.h"
#include "list.h"
#include "unwind_libunwind.h"

static struct list_element *
init_info_list() {
	return (struct list_element *)list_init();
}

void
init_aio_info_struct() {

	aio_i.submit_count = 0;
	aio_i.submit_duration = 0;
	aio_i.getevents_count = 0;
	aio_i.getevents_duration = 0;
	aio_i.fd_list = init_info_list();
	aio_i.iocb_list = init_info_list();
}

static struct iocb_info *
init_iocb_info() {
	return (struct iocb_info *)malloc(sizeof(struct iocb_info));
}

static struct fd_info *
init_fd_info() {
	return (struct fd_info *)malloc(sizeof(struct fd_info));
}


static struct iocb_info *
create_and_append_iocb_info () {
	struct iocb_info *iocb_i;

	iocb_i = init_iocb_info();
	iocb_i->submit_time = 0;
	iocb_i->complete_time = 0;
	list_add_element(aio_i.iocb_list, iocb_i);
	return iocb_i;
}


static struct iocb_info *
get_iocb_info (void * ctx, unsigned long data) {

	struct list_element *temp;
	struct iocb_info *iocb_i;
	temp=aio_i.iocb_list->next;
	while (temp != NULL)
	{
		iocb_i=(struct iocb_info *)temp->element;
		if ((iocb_i->ctx == ctx) &&
			(iocb_i->cb.aio_data == data))
		{
			return iocb_i;
		}
		temp=temp->next;
	}
	return NULL;
}

static struct fd_info *
get_fd_info(struct iocb_info *iocb_i)
{
	struct list_element *temp;
	struct fd_info *fd_i;
	temp=aio_i.fd_list->next;
	while (temp != NULL)
	{
		fd_i=(struct fd_info *)temp->element;
		if ((iocb_i->ctx == fd_i->ctx) &&
				(iocb_i->cb.aio_fildes == fd_i->fd))
		{
			return fd_i;
		}
		temp=temp->next;
	}
	return NULL;
}

static struct fd_info *
create_and_append_fd_info (struct iocb_info *iocb_i) {
	struct fd_info *fd_i;

	fd_i = init_fd_info();
	fd_i->fd = iocb_i->cb.aio_fildes;
	fd_i->ctx = iocb_i->ctx;
	fd_i->data = iocb_i->cb.aio_data;
	fd_i->read_count = 0;
	fd_i->write_count = 0;
	fd_i->read_duration = 0;
	fd_i->write_duration = 0;
	list_add_element(aio_i.fd_list, fd_i);
	return fd_i;
}

static void
remove_iocb_info (struct iocb_info *iocb_i)
{	
	struct list_element *temp;
	struct iocb_info *temp_i;
	temp=aio_i.iocb_list->next;

	while (temp != NULL)
	{
		temp_i=(struct iocb_info *)temp->element;
		
		if((temp_i->ctx == iocb_i->ctx) &&
				(temp_i->cb.aio_data == iocb_i->cb.aio_data) &&
				(temp_i->cb.aio_fildes == iocb_i->cb.aio_fildes) &&
				(temp_i->cb.aio_lio_opcode == iocb_i->cb.aio_lio_opcode))
		{
			list_remove_element(aio_i.iocb_list, temp);
			break;
		}	
		temp=temp->next;
	}
}	

void
handle_io_submit(int status, pid_t pid)
{
	void * ctx;
	unsigned int nr;
 	int	i;
	void * buf;
	unsigned long  iocbp;
	unsigned long iocbpp;
	struct iocb_info *iocb_i;
	unsigned int residue, len, iocbsize;
	
	struct user_regs_struct regs;
	struct timespec ts; 
	struct task_info *ti;
 
	if(DEBUG)
		printf("TRACER: handling io_submit syscall\n");

	ptrace(PTRACE_GETREGS, pid, 0L, &regs);

	if(DEBUG)
		printf("TRACER: got registers, accessing task_info\n");
	
	if ((ti = get_task_info(pid)) != NULL)
	{
		if(DEBUG)
			printf("TRACER: got task_info\n");
		
		if(ti->insyscall == 0)
		{
			ctx = (void *)regs.rdi;
			nr = regs.rsi;
			iocbpp = regs.rdx;
			printf("[%d] io_submit (ctx= %p, nr= %d, iocbpp=%p)\n", pid, ctx, nr, (void *)iocbpp);
			unwind_backtrace(get_task_info(pid));
			aio_i.submit_count++;
	
			ti->insyscall = 1;


			clock_gettime(CLOCK_MONOTONIC, &ts);
			ti->syscall_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;

			for (i=0; i< nr; i++)
			{
				if (DEBUG)
					printf("TRACER: Reading iocb address\n");
				iocbp = ptrace(PTRACE_PEEKDATA, pid, iocbpp+i*sizeof(long));
				printf("\tio_cb=%p",(void *)iocbp);
				iocbsize = sizeof(struct iocb);

				iocb_i = create_and_append_iocb_info();
				buf = &(iocb_i->cb);
				
				if(DEBUG)
					printf("TRACER: Getting data for iocb from address %p\n",(void *)iocbp);
				
				residue = iocbp & (sizeof(long) - 1);
				
				while (iocbsize) 
				{
					iocbp &= -sizeof(long);
					union {
						long val;
						char x[sizeof(long)];
					} u = { .val = ptrace(PTRACE_PEEKDATA, pid, iocbp, 0) };

					len	= ((sizeof(long) - residue) < iocbsize) ? (sizeof(long) - residue) : iocbsize;
					
					memcpy(buf, &u.x[residue], len);
					residue = 0;
					iocbp += sizeof(long);
					buf += len;
					iocbsize -= len;
				}


				if(DEBUG)
					printf("TRACER: Copied data from buffer to data structure\n");
				
				printf(" data=%llx, opcode=%d, fildes=%d\n", iocb_i->cb.aio_data, iocb_i->cb.aio_lio_opcode, iocb_i->cb.aio_fildes);
				
				iocb_i->ctx = ctx;
				iocb_i->submit_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
			}
			
		}
		else
		{
			ti->insyscall = 0;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ti->syscall_end_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
			printf("[%d] io_submit end\n", pid);
			aio_i.submit_duration += ti->syscall_end_time - ti->syscall_start_time;
		}
	}
}

void
handle_io_getevents(int status, pid_t pid)
{
	void * ctx;
	unsigned long min, max;
	struct user_regs_struct regs;
	struct timespec ts;
	struct task_info *ti;
	struct iocb_info *iocb_i;
	struct fd_info *fd_i;
	struct io_event ioe;
 	int	i;
	struct io_event *buf;
	unsigned long ioep;
	unsigned long ioepp;
		
	unsigned int residue, len, ioesize;
	int ret;

	if(DEBUG)
		printf("TRACER: handling io_getevents syscall\n");
	
	ptrace(PTRACE_GETREGS, pid, 0L, &regs);

	if(DEBUG)
		printf("TRACER: got registers, accessing task_info\n");
	if ((ti = get_task_info(pid)) != NULL)
	{
		if(DEBUG)
			printf("TRACER: got task_info\n");
		if(ti->insyscall == 0)
		{
			ioepp = regs.r10;
			printf("[%d] io_getevents (%p)\n", pid,(void *) regs.r10);
			unwind_backtrace(get_task_info(pid));
			aio_i.getevents_count++;

			ti->insyscall = 1;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ti->syscall_start_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
		}
		else
		{
			ti->insyscall = 0;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			ti->syscall_end_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;
			aio_i.getevents_duration += ti->syscall_end_time - ti->syscall_start_time;
			ret = regs.rax;
			ctx = (void *)regs.rdi;
			min = regs.rsi;
			max = regs.rdx;
			ioep = regs.r10;
			ioepp = regs.r10;

			if(DEBUG)
				printf("Events arrary is at: %p\n",(void *)ioep);
			printf("[%d] %d = io_getevents(ctx= %p, min=%lu, max=%lu, ", pid, ret, ctx, min, max);
			for (i=0; i< ret; i++)
			{
				if (DEBUG)
					printf("TRACER: Reading io_event address\n");
				ioesize = sizeof(struct io_event);
				buf = &ioe;
				
				if(DEBUG)
					printf("TRACER: Getting data for io_event from address %p\n",(void *)ioep);
				
				residue = ioep & (sizeof(long) - 1);
				
				while (ioesize) 
				{
					ioep &= -sizeof(long);
					union {
						long val;
						char x[sizeof(long)];
					} u = { .val = ptrace(PTRACE_PEEKDATA, pid, ioep, 0) };

					len	= ((sizeof(long) - residue) < ioesize) ? (sizeof(long) - residue) : ioesize;
					
					memcpy(buf, &u.x[residue], len);
					residue = 0;
					ioep += sizeof(long);
					buf += len;
					ioesize -= len;
				}


				if(DEBUG)
					printf("TRACER: Copied data from buffer to data structure\n");
				
				printf("[obj= %p, data=%llx], ", (void *)ioe.obj, ioe.data);
				

				

				if ((iocb_i = get_iocb_info(ctx, ioe.data)) != NULL)
				{
					iocb_i->complete_time = ts.tv_sec*1000*1000*1000 + ts.tv_nsec;

					if ((fd_i = get_fd_info(iocb_i)) == NULL)
					{
						fd_i = create_and_append_fd_info(iocb_i);
					}
					
					if ((iocb_i->cb.aio_lio_opcode == IOCB_CMD_PREAD) ||
							(iocb_i->cb.aio_lio_opcode == IOCB_CMD_PREADV))
					{
						fd_i->read_count++;
						fd_i->read_duration += iocb_i->complete_time - iocb_i->submit_time;
					}
					else if ((iocb_i->cb.aio_lio_opcode == IOCB_CMD_PWRITE) ||
										(iocb_i->cb.aio_lio_opcode == IOCB_CMD_FSYNC) ||
										(iocb_i->cb.aio_lio_opcode == IOCB_CMD_FDSYNC) ||
										(iocb_i->cb.aio_lio_opcode == IOCB_CMD_PWRITEV))
					{
						fd_i->write_count++;
						fd_i->write_duration += iocb_i->complete_time - iocb_i->submit_time;
					}
					
					remove_iocb_info(iocb_i);

				}

				ioep = ptrace(PTRACE_PEEKDATA, pid, ioepp+i*sizeof(ioe));
			}
			printf("...)\n");

		}
	}
}

static void
remove_info_list(struct list_element * rm_list) {
	list_free(rm_list);
}

void
print_aio_stats_and_remove_lists ()
{
	struct list_element *temp;
	struct fd_info *fd_i;
	unsigned long total_read_count, total_read_duration, total_write_count, total_write_duration;
	total_read_count = 0; 
	total_read_duration = 0; 
	total_write_count = 0; 
	total_write_duration = 0;
	
	printf("AIO Stats:\n");
	printf("-----------------------------------------\n");
	printf("SYSCALL \t Count \t Time\n");
	printf("-----------------------------------------\n");
	printf("io_submit \t %d \t %lu\n",aio_i.submit_count, aio_i.submit_duration);
	printf("io_getevents \t %d \t %lu\n",aio_i.getevents_count, aio_i.getevents_duration);
	printf("-----------------------------------------\n\n");
	printf("FD \t CTX \t\t RCount \t RTime \t WCount \t Wtime\n");
	printf("-------------------------------------------------------\n");
  temp=aio_i.fd_list->next;
	while (temp != NULL)
	{
		fd_i=(struct fd_info *)temp->element;
		printf("%d \t %p %u \t %9lu \t %u \t %9lu\n",fd_i->fd, fd_i->ctx, fd_i->read_count, fd_i->read_duration, fd_i->write_count, fd_i->write_duration);
    		
		total_read_count += fd_i->read_count; 
		total_read_duration += fd_i->read_duration; 
		total_write_count += fd_i->write_count; 
		total_write_duration += fd_i->write_duration;
		temp=temp->next;
  	}
	printf("-------------------------------------------------------\n");
    printf("Total: \t\t\t %lu \t %9lu \t %lu \t %9lu\n",total_read_count, total_read_duration, total_write_count, total_write_duration);
	printf("-------------------------------------------------------\n");

	remove_info_list(aio_i.iocb_list);
	remove_info_list(aio_i.fd_list);
}

