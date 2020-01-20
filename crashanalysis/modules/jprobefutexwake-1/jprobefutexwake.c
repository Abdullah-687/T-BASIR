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

char *mycommand = "mysqld";



module_param(mycommand, charp, 0);
MODULE_PARM_DESC(mycommand, "Name of program to monitor, Use this to get pid. Default is \"mysqld\"");


int my_futexhandler(void *addr1, int op, int val1, struct timespec *timeout,
		      void *addr2, int val3){
	unsigned char myinternalcommand[sizeof(current->comm)];
	get_task_comm(myinternalcommand, current);
	printk("futex called for %s\n",myinternalcommand);
	

    if (strstr(myinternalcommand, mycommand) != NULL){
		if (op == FUTEX_WAKE || op == FUTEX_WAKE_OP || op == FUTEX_WAKE_PRIVATE || op == FUTEX_WAKE_OP_PRIVATE) {
			printk(KERN_CRIT "JProbefutex: futex called for %s\n",myinternalcommand);

        }
    
	}
	jprobe_return();
	return 0;
}
 
 
static struct jprobe jp_futex;
 
int myinit(void)
{
	printk(KERN_CRIT "Jprobefutex module inserted\n ");
	printk(KERN_CRIT "Program to monitor is      : %s\n",mycommand);
	printk(KERN_CRIT "Kernel Func to monitor     : sys_futex\n");
	jp_futex.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("sys_futex");
	jp_futex.entry = (kprobe_opcode_t *) my_futexhandler;
	register_jprobe(&jp_futex);
	return 0;
}
 
void myexit(void)
{
	unregister_jprobe(&jp_futex);
	printk(KERN_CRIT "Jprobefutex module removed\n ");
}
 
module_init(myinit);
module_exit(myexit);
MODULE_DESCRIPTION("JPROBE MODULE FOR FUTEX TRAP");
MODULE_LICENSE("GPL");

