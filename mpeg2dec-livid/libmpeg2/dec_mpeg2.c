//PLUGIN_INFO(INFO_NAME, "MPEG2 video decoder");
//PLUGIN_INFO(INFO_AUTHOR, "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>");
//PLUGIN_INFO(INFO_AUTHOR, "Thomas Mirlacher <dent@linuxvideo.org>");

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <oms/log.h>
#include <oms/plugin/codec.h>

#include "config.h"
#include "mpeg2.h"

static int _mpeg2dec_open	(void *plugin, void *foo);
static int _mpeg2dec_close	(void *plugin);
static int _mpeg2dec_read	(void *plugin, buf_t *buf, buf_entry_t *buf_entry);
static int _mpeg2dec_set_flag	(void *plugin, uint flag, uint val);

static plugin_codec_t codec_mpeg2dec = {
        open:		_mpeg2dec_open,
        close:		_mpeg2dec_close,
        read:		_mpeg2dec_read,
	set_flag:	_mpeg2dec_set_flag,
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


static int _mpeg2dec_set_flag (void *plugin, uint flag, uint val)
{
	switch (flag) {
	case FLAG_VIDEO_INITIALIZED:
		mpeg2_output_init (val);
		break;
	case FLAG_VIDEO_DROP_FRAME:
//                fprintf (stderr, "%c", val ? '-':'+');
                mpeg2_drop (val);
		break;
	default:
		return -1;
	}

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
