//PLUGIN_INFO(INFO_NAME, "MPEG2 video decoder");
//PLUGIN_INFO(INFO_AUTHOR, "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>");
//PLUGIN_INFO(INFO_AUTHOR, "Thomas Mirlacher <dent@linuxvideo.org>");

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#include <oms/log.h>
#include <oms/plugin/codec.h>

#include "mpeg2.h"

static int _mpeg2dec_open	(void *plugin, void *foo);
static int _mpeg2dec_close	(void *plugin);
static int _mpeg2dec_read	(void *plugin, buf_t *buf, buf_entry_t *buf_entry);
static int _mpeg2dec_ctrl	(void *plugin, uint flag, ...);

static plugin_codec_t codec_mpeg2dec = {
        open:		_mpeg2dec_open,
        close:		_mpeg2dec_close,
        read:		_mpeg2dec_read,
	ctrl:		_mpeg2dec_ctrl,
};


/************************************************/

static int _mpeg2dec_open (void *plugin, void *foo)
{
	mpeg2_init ();
	return 0;
}


static int _mpeg2dec_close (void *plugin)
{
	mpeg2_close(((plugin_codec_t *) plugin)->output);

	return 0;
}


static int _mpeg2dec_read (void *plugin, buf_t *buf, buf_entry_t *buf_entry)
{       
	mpeg2_decode_data (((plugin_codec_t *) plugin)->output, buf_entry->data, buf_entry->data+buf_entry->data_len);

        return 0;
}


static int _mpeg2dec_ctrl (void *plugin, uint ctrl_id, ...)
{
	va_list arg_list;

	va_start (arg_list, ctrl_id);
	switch (ctrl_id) {
		case FLAG_VIDEO_INITIALIZED: {
			int val = va_arg (arg_list, int);
			mpeg2_output_init (val);
			break;
		}
		case FLAG_VIDEO_DROP_FRAME: {
			int val = va_arg (arg_list, int);
//			fprintf (stderr, "%c", val ? '-':'+');
                	mpeg2_drop (val);
			break;
		}
		default:
			va_end (arg_list);
			return -1;
	}

	va_end (arg_list);
	return 0;
}


/*****************************************/


int plugin_init (char *whoami)
{
	pluginRegister (whoami,
		PLUGIN_ID_CODEC_VIDEO,
		"mpg1;mpg2",
		NULL,
		NULL,
		&codec_mpeg2dec);

	return 0;
}


void plugin_exit (void)
{
}
