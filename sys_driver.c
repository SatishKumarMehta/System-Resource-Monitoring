#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/mm.h>
#include <linux/sched/signal.h>
#include <linux/rcupdate.h>
#include <linux/jiffies.h>
#include <linux/version.h>
#include "../include/sys_driver.h"
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Capstone Student");
MODULE_DESCRIPTION("System Resource Monitor Pseudo Character Device Driver");
MODULE_VERSION("1.0");
/* Global Driver State */
static dev_t dev_num;
static struct cdev sys_cdev;
static struct class *sys_class = NULL;
static struct device *sys_device = NULL;
/* Configuration parameters */
static int memory_threshold = 80; /* Default warning threshold: 80% */
static int driver_version = 100;  /* Version 1.0.0 encoded */
static int open_count = 0;  
static int dev_open(struct inode *inodep, struct file *filep)
{
    open_count++;
    pr_info("sys_monitor: Device opened successfully (Active sessions: %d)\n", open_count);
    return 0;
}
/**
 * @brief Called when a user-space process closes /dev/sys_monitor
 */
static int dev_release(struct inode *inodep, struct file *filep)
{
    pr_info("sys_monitor: Device closed\n");
    return 0;
}
/**
 * @brief Handles read() system calls from user space.
 * Gathers system statistics from kernel internal data structures
 * and safely copies them to the user-space buffer.
 */
static ssize_t dev_read(struct file *filep, char __user *buffer, size_t len, loff_t *offset)
{
    struct sysinfo si;
    unsigned long mem_unit;
    unsigned long total_ram_mb;
    unsigned long free_ram_mb;
    unsigned long used_ram_mb;
    unsigned long uptime_sec;
    unsigned long hours, mins, secs;
    int active_tasks = 0;
    struct task_struct *task;
     char kbuf[512];
    int msg_len;
    /* If user has already read the data, signal End-Of-File (EOF) */
    if (*offset > 0) {
        return 0;
    }
    /* 1. Retrieve Kernel Memory Statistics */
    si_meminfo(&si);
    mem_unit = si.mem_unit ? si.mem_unit : 1;
    total_ram_mb = (si.totalram * mem_unit) / (1024 * 1024);
    free_ram_mb = (si.freeram * mem_unit) / (1024 * 1024);
    used_ram_mb = (total_ram_mb >= free_ram_mb) ? (total_ram_mb - free_ram_mb) : 0;
    uptime_sec = jiffies / HZ;
    hours = uptime_sec / 3600;
    mins = (uptime_sec % 3600) / 60;
    secs = uptime_sec % 60;
    /* 3. Count Active Linux Tasks using RCU-protected traversal */
    rcu_read_lock();
    for_each_process(task) {
        active_tasks++;
    }
    rcu_read_unlock();
    /* 4. Format Statistics into key-value structure */
    msg_len = snprintf(kbuf, sizeof(kbuf),
                       "UPTIME_SEC=%lu\n"
                       "UPTIME_HMS=%02lu:%02lu:%02lu\n"
                       "TOTAL_RAM_MB=%lu\n"
                       "FREE_RAM_MB=%lu\n"
                       "USED_RAM_MB=%lu\n"
                       "THRESHOLD=%d\n"
                       "ACTIVE_TASKS=%d\n",
                       uptime_sec, hours, mins, secs,
                       total_ram_mb, free_ram_mb, used_ram_mb,
                       memory_threshold, active_tasks);
    if (len < (size_t)msg_len) {
        return -EINVAL;
    }
    if (copy_to_user(buffer, kbuf, msg_len)) {
        pr_err("sys_monitor: copy_to_user failed\n");
        return -EFAULT;
    }
    *offset += msg_len;
    return msg_len;
}
/**
 * @brief Handles ioctl() system calls for dynamic hardware/software control.
 */
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
    int temp_val;
    switch (cmd) {
    case IOCTL_GET_VERSION:
        if (copy_to_user((int __user *)arg, &driver_version, sizeof(driver_version))) {
            return -EFAULT;
        }
        break;
    case IOCTL_SET_THRESHOLD:
        if (copy_from_user(&temp_val, (int __user *)arg, sizeof(int))) {
            return -EFAULT;
        }
        if (temp_val < 1 || temp_val > 100) {
            return -EINVAL; /* Threshold must be a percentage between 1 and 100 */
        }
        memory_threshold = temp_val;
        pr_info("sys_monitor: Memory warning threshold updated to %d%%\n", memory_threshold);
        break;
        case IOCTL_GET_THRESHOLD:
        if (copy_to_user((int __user *)arg, &memory_threshold, sizeof(int))) {
            return -EFAULT;
        }
        break;
    default:
        return -ENOTTY; /* Inappropriate ioctl for device */
    }
    return 0;
}
/* File Operations structure mapping standard system calls to driver functions */
static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .open           = dev_open,
    .release        = dev_release,
    .read           = dev_read,
    .llseek         = default_llseek,
    .unlocked_ioctl = dev_ioctl,
};
static int __init sys_driver_init(void)
{
    int ret;
    pr_info("sys_monitor: Initializing System Resource Monitor Driver...\n");
    /* 1. Dynamically allocate major and minor numbers */
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("sys_monitor: Failed to allocate major number\n");
        return ret;
    }
    /* 2. Initialize and register the character device */
    cdev_init(&sys_cdev, &fops);
    sys_cdev.owner = THIS_MODULE;
    ret = cdev_add(&sys_cdev, dev_num, 1);
    if (ret < 0) {
        pr_err("sys_monitor: Failed to add cdev\n");
        goto unregister_chrdev;
    }
    /* 3. Create device class (compatible with both old and new kernel versions) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0)
    sys_class = class_create(CLASS_NAME);
#else
    sys_class = class_create(THIS_MODULE, CLASS_NAME);
#endif
    if (IS_ERR(sys_class)) {
        pr_err("sys_monitor: Failed to create class\n");
        ret = PTR_ERR(sys_class);
        goto delete_cdev;
    }
    /* 4. Automatically create device node (/dev/sys_monitor) */
    sys_device = device_create(sys_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(sys_device)) {
        pr_err("sys_monitor: Failed to create device /dev/%s\n", DEVICE_NAME);
        ret = PTR_ERR(sys_device);
        goto destroy_class;
    }
    pr_info("sys_monitor: Driver loaded successfully! Device node created at /dev/%s [Major: %d, Minor: %d]\n",
            DEVICE_NAME, MAJOR(dev_num), MINOR(dev_num));
    return 0;
destroy_class:
    class_destroy(sys_class);
delete_cdev:
    cdev_del(&sys_cdev);
unregister_chrdev:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}
static void __exit sys_driver_exit(void)
{
    device_destroy(sys_class, dev_num);
    class_destroy(sys_class);
    cdev_del(&sys_cdev);
    unregister_chrdev_region(dev_num, 1);
    pr_info("sys_monitor: Driver unloaded and device /dev/%s removed cleanly.\n", DEVICE_NAME);
}
module_init(sys_driver_init);
module_exit(sys_driver_exit);