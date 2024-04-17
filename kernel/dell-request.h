
#ifndef _DELL_REQUEST_H_
#define _DELL_REQUEST_H_

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

#endif