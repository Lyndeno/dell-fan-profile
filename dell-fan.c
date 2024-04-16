#include <linux/module.h>
#include <linux/printk.h>

int init_module(void)
{
	pr_info("Hello world 1.\n");

	return 0;
}

void cleanup_module(void)
{
	pr_info("Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");
