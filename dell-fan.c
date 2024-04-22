#include <linux/module.h>
#include <linux/printk.h>
#include <linux/dmi.h>
#include <linux/platform_profile.h>
#include <linux/slab.h>

#include "kernel/dell-request.h"

#define SELECT_THERMAL_MANAGEMENT 19

static struct platform_profile_handler *handler;


enum dell_fan_mode_bits {
	DELL_BALANCED = 0,
	DELL_COOL_BOTTOM = 1,
	DELL_QUIET = 2,
	DELL_PERFORMANCE = 3,
};

int get_state(void) {
	struct calling_interface_buffer buffer;
	int fan_state;
	int fan_ret;

	dell_fill_request(&buffer, 0x0, 0, 0, 0);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, SELECT_THERMAL_MANAGEMENT);
	if (fan_ret)
		return fan_ret;
	fan_state = buffer.output[2];
	if ((fan_state >> DELL_BALANCED) & 1){
		return DELL_BALANCED;
	} else if ((fan_state >> DELL_COOL_BOTTOM) & 1) {
		return DELL_COOL_BOTTOM;
	} else if ((fan_state >> DELL_QUIET) & 1) {
		return DELL_QUIET;
	} else if ((fan_state >> DELL_PERFORMANCE) & 1) {
		return DELL_PERFORMANCE;
	} else {
		return 0;
	}
}

int get_supported(int *supported_bits) {
	struct calling_interface_buffer buffer;
	int fan_ret;

	dell_fill_request(&buffer, 0x0, 0, 0, 0);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, SELECT_THERMAL_MANAGEMENT);
	if (fan_ret)
		return fan_ret;
	*supported_bits = buffer.output[1] & 0xF;
	return 0;
}

int get_acc_mode(int *acc_mode) {
	struct calling_interface_buffer buffer;
	int fan_ret;
	dell_fill_request(&buffer, 0x0, 0, 0, 0);
	fan_ret = dell_send_request(&buffer, CLASS_INFO, SELECT_THERMAL_MANAGEMENT);
	if (fan_ret)
		return fan_ret;
	*acc_mode = ((buffer.output[3] >> 8) & 0xFF);
	return 0;
}

int set_state(enum dell_fan_mode_bits state) {
	struct calling_interface_buffer buffer;
	int ret;
	int acc_mode;
	ret = get_acc_mode(&acc_mode);
	if (ret) {
		return ret;
	}

	dell_fill_request(&buffer, 0x1, (acc_mode << 8) | BIT(state), 0, 0);
	ret = dell_send_request(&buffer, CLASS_INFO, SELECT_THERMAL_MANAGEMENT);
	return ret;
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
	int ret;
	int supported;
	ret = get_supported(&supported);

	if (ret != 0 || supported == 0) {
		pr_info("Dell Thermal Management not supported");
		return -ENXIO;
	}

	handler = kzalloc(sizeof(struct platform_profile_handler), GFP_KERNEL);
	if (!handler)
		return -ENOMEM;
	handler->profile_get = pp_get;
	handler->profile_set = pp_set;


	if ((supported >> DELL_QUIET) & 1)
		set_bit(PLATFORM_PROFILE_QUIET, handler->choices);
	if ((supported >> DELL_COOL_BOTTOM) & 1)
		set_bit(PLATFORM_PROFILE_COOL, handler->choices);
	if ((supported >> DELL_BALANCED) & 1)
		set_bit(PLATFORM_PROFILE_BALANCED, handler->choices);
	if ((supported >> DELL_PERFORMANCE) & 1)
		set_bit(PLATFORM_PROFILE_PERFORMANCE, handler->choices);

	platform_profile_register(handler);

	return 0;
}


void cleanup_module(void)
{
	platform_profile_remove();
	kfree(handler);
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