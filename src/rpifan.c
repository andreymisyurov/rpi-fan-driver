#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Misiurov");
MODULE_DESCRIPTION("Raspberry Pi Fan Control Driver");
MODULE_VERSION("0.1.0");

#define DEVICE_NAME "rpifan"
#define CLASS_NAME "rpifan"

static int major_num;
static struct class *rpifan_class = NULL;
static struct device *rpifan_device = NULL;

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


    rpifan_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(rpifan_class)) {
        unregister_chrdev(major_num, DEVICE_NAME);
        pr_err("%s: Failed to create device class\n", DEVICE_NAME);
        return PTR_ERR(rpifan_class);
    }
    
    pr_info("%s: Device class created\n", DEVICE_NAME);

    rpifan_device = device_create(rpifan_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
    if (IS_ERR(rpifan_device)) {
        class_destroy(rpifan_class);
        unregister_chrdev(major_num, DEVICE_NAME);
        pr_err("%s: Failed to create device\n", DEVICE_NAME);
        return PTR_ERR(rpifan_device);
    }

    pr_info("%s: Device created successfully (auto /dev/%s)\n", DEVICE_NAME, DEVICE_NAME);

    return 0;
}

static void __exit rpifan_exit(void) {
    device_destroy(rpifan_class, MKDEV(major_num, 0));
    class_destroy(rpifan_class);
    unregister_chrdev(major_num, DEVICE_NAME);
    pr_info("%s: Device unregistered\n", DEVICE_NAME);
}

module_init(rpifan_init);
module_exit(rpifan_exit);
