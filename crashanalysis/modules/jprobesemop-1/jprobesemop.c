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

#define SEMOPM_FAST	64
char *mycommand = "writer";



module_param(mycommand, charp, 0);
MODULE_PARM_DESC(mycommand, "Name of program to monitor, Use this to get pid. Default is \"writer\"");


int my_efhandler(int semid, struct sembuf __user *tsops,
				unsigned nsops){
    unsigned char myinternalcommand[sizeof(current->comm)];
    struct sembuf fast_sops[SEMOPM_FAST];
    struct sembuf *sops = fast_sops, *sop;
    int max, error;
    bool undos = false, alter = false, dupsop = false;
    unsigned long dup = 0;
    get_task_comm(myinternalcommand, current);

    if (strstr(myinternalcommand, mycommand) != NULL){
	printk(KERN_CRIT "JProbesemop: semop called for %s\n",myinternalcommand);
    
    	if (nsops > SEMOPM_FAST) {    
	    sops = kmalloc(sizeof(*sops)*nsops, GFP_KERNEL);
	    	if (sops == NULL)
		    return -ENOMEM;
    	}

    	if (copy_from_user(sops, tsops, nsops * sizeof(*tsops))) {
	    error = -EFAULT;
	    goto my_free;
        }
	
	max = 0;
	for (sop = sops; sop < sops + nsops; sop++) {
	    unsigned long mask = 1ULL << ((sop->sem_num) % BITS_PER_LONG);

	    if (sop->sem_num >= max)
		max = sop->sem_num;
	    if (sop->sem_flg & SEM_UNDO)
		undos = true;
	    if (dup & mask) {

            
		dupsop = true;
	    }
	    if (sop->sem_op != 0) {
		alter = true;
		dup |= mask;
	    }	

        
	}
	if (!undos && alter) {
	    printk(KERN_CRIT "Semaphore has been changed");
	    printk(KERN_CRIT "This command has potential of creating deadlock if crashed\n");
	}
	else
	    printk(KERN_CRIT "This command will not result in deadlock if crashed\n");
    
	
my_free:
    if (sops != fast_sops)
	kfree(sops);
    if (alter) {
	printk(KERN_CRIT "Sending Kill signal to %s will kill application. Not actually sending\n",myinternalcommand);

    }
    else
	printk(KERN_CRIT "Skipping this call because this is not alter call\n");
    }
    jprobe_return();
    return 0;
}
 
 
static struct jprobe jp_ef;
 
int myinit(void)
{
    printk(KERN_CRIT "Jprobesemop module inserted\n ");
    printk(KERN_CRIT "Program to monitor is      : %s\n",mycommand);
    printk(KERN_CRIT "Kernel Func to monitor     : sys_semop\n");
    jp_ef.kp.addr = (kprobe_opcode_t *) kallsyms_lookup_name("sys_semop");
    jp_ef.entry = (kprobe_opcode_t *) my_efhandler;
    register_jprobe(&jp_ef);
    return 0;
}
 
void myexit(void)
{
    unregister_jprobe(&jp_ef);
    printk(KERN_CRIT "Jprobesemop module removed\n ");
}
 
module_init(myinit);
module_exit(myexit);
MODULE_DESCRIPTION("JPROBE MODULE FOR SEM_OP TRAP");
MODULE_LICENSE("GPL");

