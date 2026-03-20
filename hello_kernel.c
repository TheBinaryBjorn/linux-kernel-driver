#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

// Metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("TheBinaryBjorn");
MODULE_DESCRIPTION("A simple first Linux kernel module");

static int __init hello_init(void) {
    pr_info("Kernel: Module loaded. Hello from Kernel Space!\n");
    return 0;
}

static void __exit hello_exit(void) {
    pr_info("Kernel: Module unloaded. Goodbye!\n");
}

module_init(hello_init);
module_exit(hello_exit);
