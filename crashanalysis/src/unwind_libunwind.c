// File: unwind_libunwind.c


#include <errno.h>
#include <fcntl.h>
#include <libunwind-ptrace.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ptrace.h>

#include "unwind_libunwind.h"

static unw_addr_space_t as;

void
unwind_create_address_space()
{
	as = unw_create_addr_space (&_UPT_accessors, 0);
	if (!as)
		printf ("unw_create_addr_space() failed\n");

}

void
unwind_backtrace (struct task_info *ti)
{

    unw_word_t ip, sp, start_ip = 0, off;
	int n = 0, ret;
	unw_cursor_t c;
	char buf[1024];
	size_t len;

	ret = unw_init_remote (&c, as, ti->ui);
	if (ret < 0)
		printf ("unw_init_remote() failed: ret=%d\n", ret);

	do
	{
		if ((ret = unw_get_reg (&c, UNW_REG_IP, &ip)) < 0
			|| (ret = unw_get_reg (&c, UNW_REG_SP, &sp)) < 0)
			printf ("unw_get_reg() failed: ret=%d\n", ret);


		if (n == 0)
			start_ip = ip;


        buf[0] = '\0';
		if ((ret = unw_get_proc_name (&c, buf, sizeof (buf), &off)) < 0)
			printf ("unw_get_proc_name() failed: ret=%d\n", ret);
		else
		{

            if (off)
			{
				len = strlen (buf);
				if (len >= sizeof (buf) - 32)

                len = sizeof (buf) - 32;

                sprintf (buf + len, "+0x%lx", (unsigned long) off);
			}

            printf ("\t%-32s (ip=%016lx)\n", buf, (long) ip);
		}

        if ((ret = unw_step (&c)) < 0)
			printf ("unw_step() failed: ret=%d\n", ret);
		else if (++n > STACKFRAME_DEPTH)
		{
			printf ("too deeply nested---assuming bogus unwind (start ip=%lx)\n",
			(long) start_ip);
			break;
		}
	}
	while (ret > 0);

	if (ret < 0)
		printf ("unwind failed with ret=%d\n", ret);

}


