#include <linux/module.h>
#include <linux/printk.h>
#include <linux/dmi.h>
#include <linux/platform_profile.h>
#include <linux/slab.h>

#include "kernel/dell-request.h"

static struct platform_profile_handler *handler;


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
		case PLATFORM_PROFILE_QUIET:
			ret = set_state(DELL_QUIET);
			break;
		case PLATFORM_PROFILE_COOL:
			ret = set_state(DELL_COOL_BOTTOM);
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
			*profile = PLATFORM_PROFILE_COOL;
			break;
		case DELL_QUIET:
			*profile = PLATFORM_PROFILE_QUIET;
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
	
	handler = kzalloc(sizeof(struct platform_profile_handler), GFP_KERNEL);
	if (!handler)
		return -ENOMEM;
	handler->profile_get = pp_get;
	handler->profile_set = pp_set;

	set_bit(PLATFORM_PROFILE_QUIET, handler->choices);
	set_bit(PLATFORM_PROFILE_COOL, handler->choices);
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

// Derived from smbios-thermal-ctl
//
// cbClass 17
// cbSelect 19
// User Selectable Thermal Tables(USTT)
// cbArg1 determines the function to be performed
// cbArg1 0x0 = Get Thermal Information
//  cbRES1         Standard return codes (0, -1, -2)
//  cbRES2, byte 0  Bitmap of supported thermal modes. A mode is supported if its bit is set to 1
//     Bit 0 Balanced
//     Bit 1 Cool Bottom
//     Bit 2 Quiet
//     Bit 3 Performance
//  cbRES2, byte 1 Bitmap of supported Active Acoustic Controller (AAC) modes. Each mode
//                 corresponds to the supported thermal modes in byte 0. A mode is supported if
//                 its bit is set to 1.
//     Bit 0 AAC (Balanced)
//     Bit 1 AAC (Cool Bottom
//     Bit 2 AAC (Quiet)
//     Bit 3 AAC (Performance)
//  cbRes3, byte 0 Current Thermal Mode
//     Bit 0 Balanced
//     Bit 1 Cool Bottom
//     Bit 2 Quiet
//     Bit 3 Performanc
//  cbRes3, byte 1  AAC Configuration type
//          0       Global (AAC enable/disable applies to all supported USTT modes)
//          1       USTT mode specific
//  cbRes3, byte 2  Current Active Acoustic Controller (AAC) Mode
//     If AAC Configuration Type is Global,
//          0       AAC mode disabled
//          1       AAC mode enabled
//     If AAC Configuration Type is USTT mode specific (multiple bits may be set),
//          Bit 0 AAC (Balanced)
//          Bit 1 AAC (Cool Bottom
//          Bit 2 AAC (Quiet)
//          Bit 3 AAC (Performance)
//  cbRes3, byte 3  Current Fan Failure Mode
//     Bit 0 Minimal Fan Failure (at least one fan has failed, one fan working)
//     Bit 1 Catastrophic Fan Failure (all fans have failed)
//  cbArg1 0x1   (Set Thermal Information), both desired thermal mode and
//               desired AAC mode shall be applied
//  cbArg2, byte 0  Desired Thermal Mode to set (only one bit may be set for this parameter)
//     Bit 0 Balanced
//     Bit 1 Cool Bottom
//     Bit 2 Quiet
//     Bit 3 Performance
//  cbArg2, byte 1  Desired Active Acoustic Controller (AAC) Mode to set
//     If AAC Configuration Type is Global,
//         0  AAC mode disabled
//         1  AAC mode enabled
//
//     If AAC Configuration Type is USTT mode specific (multiple bits may be set for this parameter),
//         Bit 0 AAC (Balanced)
//         Bit 1 AAC (Cool Bottom
//         Bit 2 AAC (Quiet)
//         Bit 3 AAC (Performance)