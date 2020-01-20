#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/kallsyms.h>
#include <linux/fdtable.h>
#include <linux/sched.h>
#include <linux/string.h> 
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/futex.h>

#define SIGTRACE 60
char *mycommand = "mysqld";

module_param(mycommand, charp, 0);
MODULE_PARM_DESC(mycommand, "Name of program to monitor, Use this to get pid. Default is \"mysqld\"");



int my_Pre_iosubmitHandler(struct kprobe *p, struct pt_regs *regs){
	unsigned char myinternalcommand[sizeof(current->comm)];
	get_task_comm(myinternalcommand, current);

	if (strstr(myinternalcommand, mycommand) != NULL)
	{
		printk(KERN_CRIT "iosubmitkillKProbe: Pre handler called\n");
	}
	return 0;
}



void my_Post_iosubmitHandler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{

    unsigned char myinternalcommand[sizeof(current->comm)];
	get_task_comm(myinternalcommand, current);

	if (strstr(myinternalcommand, mycommand) != NULL)
	{

        printk(KERN_CRIT "iosubmitkillKProbe: io_submit called for %s\n",myinternalcommand);
		printk(KERN_CRIT "Sending Kill signal to %s\n",myinternalcommand);
 	  dump_stack();

        

		send_sig(SIGKILL,current,0);

        
	}
	return;
}
 
 
static struct kprobe kp_io_submit;
 
int myinit(void)
{
	printk(KERN_CRIT "iosubmitkillKprobe module inserted\n ");
	printk(KERN_CRIT "Program to monitor is      : %s\n",mycommand);
	printk(KERN_CRIT "Kernel Func to monitor     : sys_io_submit\n");
	kp_io_submit.addr = (kprobe_opcode_t *) kallsyms_lookup_name("sys_io_submit");
	kp_io_submit.pre_handler =  my_Pre_iosubmitHandler;
	kp_io_submit.post_handler = my_Post_iosubmitHandler;
	register_kprobe(&kp_io_submit);
	return 0;
}
 
void myexit(void)
{
	unregister_kprobe(&kp_io_submit);
	printk(KERN_CRIT "iosubmitkillKprobe module removed\n ");
}
 
module_init(myinit);
module_exit(myexit);
MODULE_DESCRIPTION("KPROBE MODULE FOR IO_SUBMIT TRAP");
MODULE_LICENSE("GPL");

