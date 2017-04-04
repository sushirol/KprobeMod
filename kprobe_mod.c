#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/time.h>
#include <linux/string.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>

char symbol[64];
uint64_t ts, diff;
int counter;
struct timespec        start, stop, result;
struct proc_dir_entry *entry;
static struct kprobe   kp;


static int kprobe_show(struct seq_file *s, void *what);
static int
kprobe_open(struct inode *inode, struct file *file)
{
    return (single_open(file, kprobe_show, NULL));
}
static ssize_t kprobe_write(struct file *, const char __user *, size_t,
                            loff_t *);
static const struct file_operations kprobe_fops = {
    .owner   = THIS_MODULE,
    .open    = kprobe_open,
    .read    = seq_read,
    .write   = kprobe_write,
    .llseek  = seq_lseek,
    .release = single_release,
};

static int
kprobe_show(struct seq_file *s, void *what)
{
    seq_printf(s, "ns %llu function %s called %d times\n", diff, symbol,
               counter);
    return (0);
}

#define DEBUG_WRITE_STR_LEN    64
static ssize_t
kprobe_write(struct file *file, const char __user *buf, size_t count,
             loff_t *data)
{
    char str[DEBUG_WRITE_STR_LEN];

    size_t num = (count > (DEBUG_WRITE_STR_LEN - 1)) ?
                 (DEBUG_WRITE_STR_LEN - 1) : count;

    if (copy_from_user(str, buf, num)) { return (-EFAULT); }
    str[num] = '\0';
    while ((num > 0) && ((str[num - 1] == '\r') || (str[num - 1] == '\n')))
    {
        str[--num] = '\0';
    }
    if (strcmp(str, "clear") == 0)
    {
        counter = 0;
        diff = 0;
        printk("stats cleared %d times %lld ns\n", counter, diff);
    }
    else
    {
        sscanf(str, "%s", symbol);
        printk("%s", symbol);
        counter = 0;
        diff = 0;
        kp.symbol_name = kstrdup_const(symbol, GFP_KERNEL);
    }
    return (count);
}
void
timespec_diff(struct timespec *start, struct timespec *stop,
              struct timespec *result)
{
    if ((stop->tv_nsec - start->tv_nsec) < 0)
    {
        result->tv_sec = stop->tv_sec - start->tv_sec - 1;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000;
    }
    else
    {
        result->tv_sec = stop->tv_sec - start->tv_sec;
        result->tv_nsec = stop->tv_nsec - start->tv_nsec;
    }
    return;
}

int
Pre_Handler(struct kprobe *p, struct pt_regs *regs)
{
    getnstimeofday64(&start);
    counter++;
    return (0);
}

void
Post_Handler(struct kprobe *p, struct pt_regs *regs, unsigned long flags)
{
    uint64_t diff2;

    getnstimeofday64(&stop);
    timespec_diff(&start, &stop, &result);
    diff2 = result.tv_nsec + (result.tv_sec * 1000000);
    diff -= diff / counter;
    diff += diff2 / counter;
}

int
myinit(void)
{
    printk("module inserted\n ");
    ts = 0;
    diff = 0;
    kp.pre_handler = Pre_Handler;
    kp.post_handler = Post_Handler;
    /*kp.addr = (kprobe_opcode_t *)0xffffffffa02a7a60;*/
    kp.symbol_name = "my_symbol";
    register_kprobe(&kp);
    entry = proc_create_data("my_kprobe", S_IRUGO | S_IWUGO, NULL,
                             &kprobe_fops, NULL);
    return (entry ? 0 : -ENOMEM);
    return (0);
}

void
myexit(void)
{
    unregister_kprobe(&kp);
    if (entry)
    {
        proc_remove(entry);
        entry = NULL;
    }
    printk("diff %llu %d\r\n", diff, counter);
    return;
}

module_init(myinit);
module_exit(myexit);
MODULE_AUTHOR("sushrut");
MODULE_DESCRIPTION("KPROBE MODULE");
MODULE_LICENSE("GPL");
