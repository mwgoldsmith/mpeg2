//PLUGIN_INFO(INFO_NAME, "MPEG2 video decoder");
//PLUGIN_INFO(INFO_AUTHOR, "Aaron Holtzman <aholtzma@ess.engr.uvic.ca>");
//PLUGIN_INFO(INFO_AUTHOR, "Thomas Mirlacher <dent@linuxvideo.org>");

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <oms/oms.h>
#include <oms/plugin/codec_video.h>

#include "config.h"
#include "mpeg2.h"

static int _mpeg2dec_open	(plugin_t *plugin, void *foo);
static int _mpeg2dec_close	(plugin_t *plugin);
static int _mpeg2dec_read	(plugin_codec_video_t *plugin, buf_t *buf, buf_entry_t *buf_entry);
static int _mpeg2dec_set_flag	(plugin_codec_video_t *plugin, uint flag, uint val);

static plugin_codec_video_t codec_mpeg2dec = {
        open:		_mpeg2dec_open,
        close:		_mpeg2dec_close,
        read:		_mpeg2dec_read,
	set_flag:	_mpeg2dec_set_flag,
};


/************************************************/


static int _mpeg2dec_open (plugin_t *plugin, void *foo)
{
	mpeg2_init ();
	return 0;
}


static int _mpeg2dec_close (plugin_t *plugin)
{
	return 0;
}


static int _mpeg2dec_read (plugin_codec_video_t *plugin, buf_t *buf, buf_entry_t *buf_entry)
{
	mpeg2_decode_data (plugin->output, buf_entry->data, buf_entry->data+buf_entry->data_len);

	return 0;
}

static int _mpeg2dec_set_flag	(plugin_codec_video_t *plugin, uint flag, uint val)
{

	switch (flag) {
	case FLAG_VIDEO_DROP_FRAME:
		fprintf (stderr, "%c", val ? '-':'+');
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
		"mpg2",
		NULL,
		NULL,
		&codec_mpeg2dec);

	return 0;
}


void plugin_exit (void)
{
}
