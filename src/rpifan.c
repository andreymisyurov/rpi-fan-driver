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
#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Misiurov");
MODULE_DESCRIPTION("Raspberry Pi Fan Control Driver");
MODULE_VERSION("0.1.0");

#define FAN_GPIO 18
#define DEVICE_NAME "rpifan"
#define PROC_BUFFER_SIZE 64

static struct proc_dir_entry *rpifan_proc_dir = NULL;
static struct proc_dir_entry *proc_status = NULL;
static struct proc_dir_entry *proc_threshold_temp = NULL;

static struct thermal_zone_device *cpu_thermal_zone = NULL;
static struct timer_list fan_timer;
static unsigned int fan_enabled;
static unsigned int threshold_temp;

static ssize_t status_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t status_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);
static ssize_t threshold_temp_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset);
static ssize_t threshold_temp_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *offset);

static int get_cpu_temp(int *out_temp);

static const struct proc_ops status_proc_ops = {
    .proc_read = status_proc_read,
    .proc_write = status_proc_write,
};

static const struct proc_ops threshold_temp_proc_ops = {
    .proc_read = threshold_temp_proc_read,
    .proc_write = threshold_temp_proc_write,
};

static ssize_t status_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char status[PROC_BUFFER_SIZE] = {0};
    size_t status_len = 0;
    int temp = 0;

    if (*offset > 0) return 0;
    if (len == 0) return 0;

    if (0 == get_cpu_temp(&temp)) {
        status_len = snprintf(status, sizeof(status), 
        "Fan: %s\nTemperature: %d.%d °C\n", 
        fan_enabled ? "on" : "off", 
        temp/10, temp%10);
    } else {
        status_len = snprintf(status, sizeof(status), 
        "Fan: %s\nTemperature: N/A\n", 
        fan_enabled ? "on" : "off");
    }
    
    status_len = status_len < len ? status_len : len;
    if (0 !=copy_to_user(buf, status, status_len)) return -EFAULT;

    *offset += status_len;
    return status_len;
}

static ssize_t status_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    char command[PROC_BUFFER_SIZE] = {0};

    if (len > PROC_BUFFER_SIZE) len = PROC_BUFFER_SIZE;
    if (len == 0) return -EINVAL;

    if (copy_from_user(command, buf, len)) return -EFAULT;

    if (command[len - 1] == '\n')
        command[len - 1] = '\0';

    if (0 == strcmp(command, "on")) {
        fan_enabled = 1;
    } else if (0 == strcmp(command, "off")) {
        fan_enabled = 0;
    } else {
        pr_warn("%s: Unknown command: '%s'\n", DEVICE_NAME, command);
        return -EINVAL;
    }
    return len;
}

static ssize_t threshold_temp_proc_write(struct file *file, const char __user *buf, size_t len, loff_t *offset) {
    char command[PROC_BUFFER_SIZE] = {0};
    unsigned temp = 0;
    
    if (len > PROC_BUFFER_SIZE) len = PROC_BUFFER_SIZE;
    if (len == 0) return -EINVAL;

    if (copy_from_user(command, buf, len)) return -EFAULT;

    if (command[len - 1] == '\n')
    command[len - 1] = '\0';

    if (kstrtouint(command, 10, &temp) != 0) {
        pr_warn("%s: Invalid number: '%s'\n", DEVICE_NAME, command);
        return -EINVAL;
    }
    if (temp < 20 || temp > 90) {
        pr_warn("%s: Temperature %u out of range [20-90]°C\n", DEVICE_NAME, temp);
        return -EINVAL;
    }
    threshold_temp = temp;
    return len;
}


static ssize_t threshold_temp_proc_read(struct file *file, char __user *buf, size_t len, loff_t *offset) {
    char output[PROC_BUFFER_SIZE] = {0};
    size_t output_len = 0;

    if (*offset > 0) return 0;
    if (len == 0) return 0;

    output_len = snprintf(output, sizeof(output),
    "Threshold Temperature: %d °C\n", threshold_temp);
    
    output_len = output_len < len ? output_len : len;
    if (0 !=copy_to_user(buf, output, output_len)) return -EFAULT;

    *offset += output_len;
    return output_len;
}

static int get_cpu_temp(int *out_temp) {
    int temp = 0;
    int ret = 0;

    if (IS_ERR(cpu_thermal_zone)) {
        ret = PTR_ERR(cpu_thermal_zone);
    } else {
        ret = thermal_zone_get_temp(cpu_thermal_zone, &temp);
        if (0 == ret) {
            *out_temp = temp / 100;
        }
    }
    return ret;
}

static void fan_timer_callback(struct timer_list *t) {
    int temp = 0;
    if (0 == get_cpu_temp(&temp)) {
        if (temp / 10 >= threshold_temp)
            fan_enabled = 1;
        else if (temp / 10 < (threshold_temp - 5))
            fan_enabled = 0;
    }

    gpio_set_value(FAN_GPIO, fan_enabled);
    mod_timer(&fan_timer, jiffies + msecs_to_jiffies(5000));
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
    return 0;
}

static int __init rpifan_init(void) {
    int ret = 0;

    threshold_temp = 50;
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

    proc_status = proc_create("status", 0666, rpifan_proc_dir, &status_proc_ops);
    if (!proc_status) {
        pr_err("%s: Failed to create /proc/rpifan/status\n", DEVICE_NAME);
        ret = -ENOMEM;
        goto err_proc;
    }

    proc_threshold_temp = proc_create("threshold_temp", 0666, rpifan_proc_dir, &threshold_temp_proc_ops);
    if (!proc_threshold_temp) {
        pr_err("%s: Failed to create /proc/rpifan/threshold_temp\n", DEVICE_NAME);
        ret = -ENOMEM;
        goto err_proc;
    }

    timer_setup(&fan_timer, fan_timer_callback, 0);
    mod_timer(&fan_timer, jiffies + msecs_to_jiffies(5000));

    return ret;

err_proc:
    proc_remove(rpifan_proc_dir);
err_gpio:
    gpio_free(FAN_GPIO);
    return ret;
}

static void __exit rpifan_exit(void) {
    del_timer_sync(&fan_timer);
    proc_remove(proc_status);
    proc_remove(proc_threshold_temp);
    proc_remove(rpifan_proc_dir);
    gpio_set_value(FAN_GPIO, 0);
    gpio_free(FAN_GPIO);
}

module_init(rpifan_init);
module_exit(rpifan_exit);
