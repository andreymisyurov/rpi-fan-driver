#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Gabella");
MODULE_DESCRIPTION("Simple Hello World kernel module");
MODULE_VERSION("1.0");

static int __init hello_init(void) {
    pr_err("Hello World! Module loaded.\n");
    return 0;
}

static void __exit hello_exit(void) {
    pr_err("Goodbye World! Module unloaded.\n");
}

module_init(hello_init);
module_exit(hello_exit);
