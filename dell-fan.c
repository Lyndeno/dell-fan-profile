#include <linux/module.h>
#include <linux/printk.h>
#include <linux/dmi.h>
#include <linux/platform_profile.h>
#include <linux/slab.h>

#include "dell-smbios.h"

static struct platform_profile_handler *handler;

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

enum dell_fan_modes {
	DELL_BALANCED = BIT(0),
	DELL_COOL_BOTTOM = BIT(1),
	DELL_QUIET = BIT(2),
	DELL_PERFORMANCE = BIT(3),
};

enum dell_fan_modes get_state(void) {
	struct calling_interface_buffer buffer;
	int fan_state;
	int fan_ret;

	dell_fill_request(&buffer, 0x0, 0, 0, 0);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, 19);
	if (fan_ret)
		return fan_ret;
	fan_state = buffer.output[2];
	if ((fan_state & DELL_BALANCED) == DELL_BALANCED){
		return DELL_BALANCED;
	} else if ((fan_state & DELL_COOL_BOTTOM) == DELL_COOL_BOTTOM) {
		return DELL_COOL_BOTTOM;
	} else if ((fan_state & DELL_QUIET) == DELL_QUIET) {
		return DELL_QUIET;
	} else if ((fan_state & DELL_PERFORMANCE) == DELL_PERFORMANCE) {
		return DELL_PERFORMANCE;
	} else {
		return DELL_PERFORMANCE;
	}
}

int set_state(enum dell_fan_modes state) {
	struct calling_interface_buffer buffer;
	int fan_ret;

	dell_fill_request(&buffer, 0x1, state, 0, 0);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, 19);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, 19);
	return fan_ret;
}

static int pp_set(struct platform_profile_handler *pprof,
					enum platform_profile_option profile) {
	int ret;
	switch (profile) {
		case PLATFORM_PROFILE_BALANCED:
			ret = set_state(DELL_BALANCED);
			break;
		case PLATFORM_PROFILE_PERFORMANCE:
			ret = set_state(DELL_PERFORMANCE);
			break;
		case PLATFORM_PROFILE_LOW_POWER:
			ret = set_state(DELL_QUIET);
			break;
		default:
			return -EOPNOTSUPP;
	}

	if (ret < 0)
		return ret;

	return 0;
}

static int pp_get(struct platform_profile_handler *pprof, enum platform_profile_option *profile) {
	switch (get_state()) {
		case DELL_BALANCED:
			*profile = PLATFORM_PROFILE_BALANCED;
			break;
		case DELL_PERFORMANCE:
			*profile = PLATFORM_PROFILE_PERFORMANCE;
			break;
		case DELL_COOL_BOTTOM:
			*profile = PLATFORM_PROFILE_PERFORMANCE;
			break;
		case DELL_QUIET:
			*profile = PLATFORM_PROFILE_LOW_POWER;
			break;
		default:
			return -EINVAL;
	}

	return 0;
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
	pr_info("Bit 0 : Balanced supported:   %u\n",
		   status & DELL_BALANCED);
	pr_info("Bit 1 : Cool Bottom supported:   %u\n",
		   (status & DELL_COOL_BOTTOM) >> 1);
	pr_info("Bit 2 : Quiet supported:   %u\n",
		   (status & DELL_QUIET) >> 2);
	pr_info("Bit 3 : Performance supported:   %u\n",
		   (status & DELL_PERFORMANCE) >> 3);
	pr_info("Bit 0 : Balanced enabled:   %lu\n",
		   fan_state & BIT(0));
	pr_info("Bit 1 : Cool Bottom enabled:   %lu\n",
		   (fan_state & BIT(1)) >> 1);
	pr_info("Bit 2 : Quiet enabled:   %lu\n",
		   (fan_state & BIT(2)) >> 2);
	pr_info("Bit 3 : Performance enabled:   %lu\n",
		   (fan_state & BIT(3)) >> 3);
	
	switch (get_state()) {
		case DELL_BALANCED:
			pr_info("Balanced\n");
			break;
		case DELL_COOL_BOTTOM:
			pr_info("Cool bottom\n");
			break;
		case DELL_QUIET:
			pr_info("Quiet\n");
			break;
		case DELL_PERFORMANCE:
			pr_info("Performance\n");
			break;
	}
	
	struct platform_profile_handler *handler;
	handler = kzalloc(sizeof(struct platform_profile_handler), GFP_KERNEL);
	if (!handler)
		return -ENOMEM;
	handler->profile_get = pp_get;
	handler->profile_set = pp_set;

	set_bit(PLATFORM_PROFILE_LOW_POWER, handler->choices);
	set_bit(PLATFORM_PROFILE_BALANCED, handler->choices);
	set_bit(PLATFORM_PROFILE_PERFORMANCE, handler->choices);

	platform_profile_register(handler);

	return 0;
}


void cleanup_module(void)
{
	platform_profile_remove();
	kfree(handler);
	pr_info("Goodbye world 1.\n");
}

MODULE_LICENSE("GPL");
