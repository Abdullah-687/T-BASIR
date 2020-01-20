// File: backtrace.h


#ifndef TRACER_BACKTRACE_H
#define TRACER_BACKTRACE_H
#include "defs.h"
#ifndef STACKFRAME_DEPTH
	#define STACKFRAME_DEPTH 256
#endif


void
unwind_create_address_space();

void
unwind_backtrace (struct task_info *);

#endif
