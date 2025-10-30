#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/gpio.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Misiurov");
MODULE_DESCRIPTION("Raspberry Pi Fan Control Driver");
MODULE_VERSION("0.1.0");

#define FAN_GPIO 18
#define DEVICE_NAME "rpifan"
#define CLASS_NAME "rpifan"

static int major_num;
static struct class *rpifan_class = NULL;
static struct device *rpifan_device = NULL;

static unsigned int fan_enabled = 0;

static ssize_t rpifan_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = rpifan_write,
};

static ssize_t rpifan_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    char command[5] = {0};

    if (len > 4) len = 4;
    if (len == 0) return -EINVAL;

    if (copy_from_user(command, buf, len))
        return -EFAULT;

    if (command[len - 1] == '\n')
        command[len - 1] = '\0';

    if (0 == strcmp(command, "on")) {
        fan_enabled = 1;
        gpio_set_value(FAN_GPIO, fan_enabled);
        pr_info("%s: Fan turned ON\n", DEVICE_NAME);
    } else if (0 == strcmp(command, "off")) {
        fan_enabled = 0;
        gpio_set_value(FAN_GPIO, fan_enabled);
        pr_info("%s: Fan turned OFF\n", DEVICE_NAME);
    } else {
        pr_warn("%s: Unknown command: '%s'\n", DEVICE_NAME, command);
        return -EINVAL;
    }
    return len;
}

static int rpifan_gpio_init(void) {
    int ret = 0;

    if (!gpio_is_valid(FAN_GPIO)) {
        pr_err("%s: Invalid GPIO %d\n", DEVICE_NAME, FAN_GPIO);
        return -ENODEV;
    }

    ret = gpio_request(FAN_GPIO, "rpifan_gpio");
    if (ret != 0) {
        pr_err("%s: Cannot request GPIO %d\n", DEVICE_NAME, FAN_GPIO);
        return ret;
    }

    gpio_direction_output(FAN_GPIO, 0);
    pr_info("%s: GPIO %d configured\n", DEVICE_NAME, FAN_GPIO);
    return 0;
}

static int __init rpifan_init(void) {
    major_num = register_chrdev(0, DEVICE_NAME, &fops);
    int ret = 0;

    if (major_num < 0) {
        pr_err("%s: Failed to register device: %d\n", DEVICE_NAME, major_num);
        return major_num;
    }

    rpifan_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(rpifan_class)) {
        unregister_chrdev(major_num, DEVICE_NAME);
        pr_err("%s: Failed to create device class\n", DEVICE_NAME);
        return PTR_ERR(rpifan_class);
    }

    rpifan_device = device_create(rpifan_class, NULL, MKDEV(major_num, 0), NULL, DEVICE_NAME);
    if (IS_ERR(rpifan_device)) {
        pr_err("%s: Failed to create device\n", DEVICE_NAME);
        ret = PTR_ERR(rpifan_device);
        goto err_class;
    }

    ret = rpifan_gpio_init();
    if (ret != 0) goto err_device;

    pr_info("%s: Device created successfully (auto /dev/%s)\n", DEVICE_NAME, DEVICE_NAME);
    return 0;

err_device:
    device_destroy(rpifan_class, MKDEV(major_num, 0));
err_class:
    class_destroy(rpifan_class);
err_chrdev:
    unregister_chrdev(major_num, DEVICE_NAME);
    return ret;
}

static void __exit rpifan_exit(void) {
    gpio_set_value(FAN_GPIO, 0);
    gpio_free(FAN_GPIO);
    device_destroy(rpifan_class, MKDEV(major_num, 0));
    class_destroy(rpifan_class);
    unregister_chrdev(major_num, DEVICE_NAME);
    pr_info("%s: Device unregistered\n", DEVICE_NAME);
}

module_init(rpifan_init);
module_exit(rpifan_exit);
