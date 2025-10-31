#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/gpio.h>
#include <linux/thermal.h>
#include <linux/proc_fs.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Misiurov");
MODULE_DESCRIPTION("Raspberry Pi Fan Control Driver");
MODULE_VERSION("0.1.0");

#define FAN_GPIO 18
#define DEVICE_NAME "rpifan"
#define CLASS_NAME "rpifan"

static struct proc_dir_entry *rpifan_proc_dir = NULL;
static struct proc_dir_entry *proc_status = NULL;

static struct thermal_zone_device *cpu_thermal_zone = NULL;
static unsigned int fan_enabled = 0;

static ssize_t status_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t status_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static int get_cpu_temp(void);

static const struct proc_ops status_proc_ops = {
    .proc_read = status_proc_read,
    .proc_write = status_proc_write,
};

static ssize_t status_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char status[256] = {0};
    size_t status_len = 0;
    int temp = 0;

    if (*offset > 0) return 0;
    if (len == 0) return 0;

    
    temp = get_cpu_temp();
    if (temp >= 0) {
        status_len = snprintf(status, sizeof(status), 
        "Fan: %s\nTemperature: %d.%d °C\n", 
        fan_enabled ? "on" : "off", 
        temp/10, temp%10);
    } else {
        status_len = snprintf(status, sizeof(status), 
        "Fan: %s\nTemperature: N/A (error: %d)\n", 
        fan_enabled ? "on" : "off", 
        temp);
    }
    
    status_len = status_len < len ? status_len : len;
    if (0 !=copy_to_user(buf, status, status_len)) return -EFAULT;

    *offset += status_len;
    return status_len;
}

static ssize_t status_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
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

static int get_cpu_temp(void) {
    int temp = 0;
    int ret = 0;
    
    if (IS_ERR(cpu_thermal_zone)) {
        return PTR_ERR(cpu_thermal_zone);
    }
    
    ret = thermal_zone_get_temp(cpu_thermal_zone, &temp);
    if (ret) {
        pr_err("%s: Failed to read temperature: %d\n", DEVICE_NAME, ret);
        return ret;
    }
    
    return temp / 100;
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
    int ret = 0;

    ret = rpifan_gpio_init();
    if (ret != 0) return ret;

    cpu_thermal_zone = thermal_zone_get_zone_by_name("cpu-thermal");
    if (IS_ERR(cpu_thermal_zone)) {
        pr_err("%s: Failed to get thermal zone\n", DEVICE_NAME);
        ret = PTR_ERR(cpu_thermal_zone);
        goto err_gpio;
    }

    rpifan_proc_dir = proc_mkdir("rpifan", NULL);
    if (!rpifan_proc_dir) {
        pr_err("%s: Failed to create /proc/rpifan\n", DEVICE_NAME);
        ret = -ENOMEM;
        goto err_gpio;
    }

    proc_status = proc_create("status", 0444, rpifan_proc_dir, &status_proc_ops);
    if (!proc_status) {
        pr_err("%s: Failed to create /proc/rpifan/status\n", DEVICE_NAME);
        ret = -ENOMEM;
        goto err_proc;
    }

    pr_info("%s: /proc/rpifan created successfully\n", DEVICE_NAME);
    return ret;

err_proc:
    proc_remove(rpifan_proc_dir);
err_gpio:
    gpio_free(FAN_GPIO);
    return ret;
}

static void __exit rpifan_exit(void) {
    proc_remove(proc_status);
    proc_remove(rpifan_proc_dir);
    gpio_set_value(FAN_GPIO, 0);
    gpio_free(FAN_GPIO);
    pr_info("%s: /proc/rpifan removed\n", DEVICE_NAME);
}

module_init(rpifan_init);
module_exit(rpifan_exit);
