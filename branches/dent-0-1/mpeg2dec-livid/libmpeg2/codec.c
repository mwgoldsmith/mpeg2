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

static plugin_codec_video_t codec_mpeg2dec = {
        open:		_mpeg2dec_open,
        close:		_mpeg2dec_close,
        read:		_mpeg2dec_read,
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
	uint8_t *buf_start = buf_entry->data;
	uint8_t *buf_end = data_start+buf_entry->data_len;
	const plugin_output_video_t *output = (plugin_output_video_t *) (((plugin_codec_video_t *) plugin)->output);

	return mpeg2_decode_data (plugin->output, buf_start, buf_end);

	return ret;
}


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
