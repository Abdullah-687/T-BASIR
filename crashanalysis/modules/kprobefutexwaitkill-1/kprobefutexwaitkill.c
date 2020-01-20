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



int my_Pre_futexWaitHandler(struct kprobe *p, struct pt_regs *regs){
	unsigned char myinternalcommand[sizeof(current->comm)];
	get_task_comm(myinternalcommand, current);

	if (strstr(myinternalcommand, mycommand) != NULL)
	{
		printk(KERN_CRIT "KProbeFutexWait: Pre handler called\n");
	}
	return 0;
}



void my_Post_futexWaitHandler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{

    unsigned char myinternalcommand[sizeof(current->comm)];
	get_task_comm(myinternalcommand, current);

	if (strstr(myinternalcommand, mycommand) != NULL)
	{

        printk(KERN_CRIT "KProbeFutexWait: futex called for %s\n",myinternalcommand);
		printk(KERN_CRIT "Sending Kill signal to %s will potentially result in locked threads\n",myinternalcommand);

        
		send_sig(SIGTRACE,current,0);

        
	}
	return;
}
 
 
static struct kprobe kp_futex_wait;
 
int myinit(void)
{
	printk(KERN_CRIT "Kprobefutexwait module inserted\n ");
	printk(KERN_CRIT "Program to monitor is      : %s\n",mycommand);
	printk(KERN_CRIT "Kernel Func to monitor     : sys_futex\n");
	kp_futex_wait.addr = (kprobe_opcode_t *) kallsyms_lookup_name("sys_futex");
	kp_futex_wait.pre_handler =  my_Pre_futexWaitHandler;
	kp_futex_wait.post_handler = my_Post_futexWaitHandler;
	register_kprobe(&kp_futex_wait);
	return 0;
}
 
void myexit(void)
{
	unregister_kprobe(&kp_futex_wait);
	printk(KERN_CRIT "Kprobefutexwait module removed\n ");
}
 
module_init(myinit);
module_exit(myexit);
MODULE_DESCRIPTION("KPROBE MODULE FOR FUTEX (Wait) TRAP");
MODULE_LICENSE("GPL");

