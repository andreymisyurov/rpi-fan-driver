#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Misiurov");
MODULE_DESCRIPTION("Raspberry Pi Fan Control Driver");
MODULE_VERSION("0.1.0");

#define DEVICE_NAME "rpifan"

static int major_num;

static struct file_operations fops = {
    .owner = THIS_MODULE,
};

static int __init rpifan_init(void) {
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    
    if (major_num < 0) {
        pr_err("%s: Failed to register device: %d\n", DEVICE_NAME, major_num);
        return major_num;
    }
    
    pr_info("%s: Registered with major number %d\n", DEVICE_NAME, major_num);
    pr_info("%s: Create device: sudo mknod /dev/%s c %d 0\n", 
            DEVICE_NAME, DEVICE_NAME, major_num);
    
    return 0;
}

static void __exit rpifan_exit(void) {
    unregister_chrdev(major_num, DEVICE_NAME);
    pr_info("%s: Device unregistered\n", DEVICE_NAME);
}

module_init(rpifan_init);
module_exit(rpifan_exit);
