#include <linux/module.h>
#include <linux/printk.h>
#include <linux/dmi.h>

#include "dell-smbios.h"

static void dell_fill_request(struct calling_interface_buffer *buffer,
			       u32 arg0, u32 arg1, u32 arg2, u32 arg3)
{
	memset(buffer, 0, sizeof(struct calling_interface_buffer));
	buffer->input[0] = arg0;
	buffer->input[1] = arg1;
	buffer->input[2] = arg2;
	buffer->input[3] = arg3;
}

static int dell_send_request(struct calling_interface_buffer *buffer,
			     u16 class, u16 select)
{
	int ret;

	buffer->cmd_class = class;
	buffer->cmd_select = select;
	ret = dell_smbios_call(buffer);
	if (ret != 0)
		return ret;
	return dell_smbios_error(buffer->output[0]);
}

int init_module(void)
{
	pr_info("Hello world 1.\n");

	struct calling_interface_buffer buffer;
	int fan_state;
	int fan_ret;
	int status;
	int ret;

	dell_fill_request(&buffer, 0, 0, 0, 0);
	ret = dell_send_request(&buffer, CLASS_INFO, 19);
	if (ret)
		return ret;
	status = buffer.output[1];

	dell_fill_request(&buffer, 0x0, 0, 0, 0);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, 19);
	if (fan_ret)
		return fan_ret;
	fan_state = buffer.output[2];

	pr_info("return:\t%d\n", ret);
	pr_info("status:\t0x%X\n", status);
	pr_info("Bit 0 : Balanced supported:   %lu\n",
		   status & BIT(0));
	pr_info("Bit 1 : Cool Bottom supported:   %lu\n",
		   (status & BIT(1)) >> 1);
	pr_info("Bit 2 : Quiet supported:   %lu\n",
		   (status & BIT(2)) >> 2);
	pr_info("Bit 3 : Performance supported:   %lu\n",
		   (status & BIT(3)) >> 3);
	pr_info("Bit 0 : Balanced enabled:   %lu\n",
		   fan_state & BIT(0));
	pr_info("Bit 1 : Cool Bottom enabled:   %lu\n",
		   (fan_state & BIT(1)) >> 1);
	pr_info("Bit 2 : Quiet enabled:   %lu\n",
		   (fan_state & BIT(2)) >> 2);
	pr_info("Bit 3 : Performance enabled:   %lu\n",
		   (fan_state & BIT(3)) >> 3);

	return 0;
}

void cleanup_module(void)
{
	pr_info("Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");
