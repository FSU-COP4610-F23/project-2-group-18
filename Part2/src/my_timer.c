#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Example of kernel module for timer");

#define ENTRY_NAME "timer_example"
#define PERMS 0644
#define PARENT NULL

bool first_time = true;
static int procfs_buf_len;

static struct timespec64 previous_time;
static struct proc_dir_entry* timer_entry;

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 ts_now; // variable to hold current time
    char buf[256]; // buffer for printing current time
    int len = 0;

    procfs_buf_len = strlen(buf);
    if(*ppos > 0 || count < procfs_buf_len)
        return 0;

    ktime_get_real_ts64(&ts_now); //this is where we get current time

    if(first_time)
    {
        len = snprintf(buf, sizeof(buf), "current time: %lld.%09lld\n", 
            (long long)(ts_now.tv_sec), (long long)(ts_now.tv_nsec));
        first_time = false;
    }

    else
    {
        //this is to compute elapsed time
        long sec = (long long)ts_now.tv_sec - (long long)previous_time.tv_sec;
        long nsec = ((long long)ts_now.tv_nsec - (long long)previous_time.tv_nsec);
        if(nsec < 0){
            nsec = nsec + 1000000000; //make possitive
            sec--;
        }

        len = snprintf(buf, sizeof(buf), "current time: %lld.%09lld\nelapsed time: %lld.%09lld\n", 
            (long long)(ts_now.tv_sec), (long long)(ts_now.tv_nsec), (long long)sec, (long long)nsec);
    }

    previous_time = ts_now; //change the previous time to current time


    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // better than copy_from_user
}

static const struct proc_ops timer_fops = {
    .proc_read = timer_read,
};

static int __init timer_init(void)
{
    timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    if (!timer_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);