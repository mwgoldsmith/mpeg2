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

static struct timeval tv_beg, tv_end, tv_start;
static uint32_t elapsed;
static uint32_t total_elapsed;
static uint32_t last_count = 0;
static uint32_t frame_counter = 0;
/************************************************/

static void print_fps (int final) 
{
    int fps, tfps, frames;
	
    gettimeofday (&tv_end, NULL);

    if (frame_counter++ == 0) {
	tv_start = tv_beg = tv_end;
    }

    elapsed = (tv_end.tv_sec - tv_beg.tv_sec) * 100 +
	(tv_end.tv_usec - tv_beg.tv_usec) / 10000;
    total_elapsed = (tv_end.tv_sec - tv_start.tv_sec) * 100 +
	(tv_end.tv_usec - tv_start.tv_usec) / 10000;

    if (final) {
	if (total_elapsed) 
	    tfps = frame_counter * 10000 / total_elapsed;
	else
	    tfps = 0;

	fprintf (stderr,"\n%d frames decoded in %d.%02d "
		 "seconds (%d.%02d fps)\n", frame_counter,
		 total_elapsed / 100, total_elapsed % 100,
		 tfps / 100, tfps % 100);

	return;
    }

    if (elapsed < 50)	/* only display every 0.50 seconds */
	return;

    tv_beg = tv_end;
    frames = frame_counter - last_count;

    fps = frames * 10000 / elapsed;			/* 100x */
    tfps = frame_counter * 10000 / total_elapsed;	/* 100x */

    fprintf (stderr, "%d frames in %d.%02d sec (%d.%02d fps), "
	     "%d last %d.%02d sec (%d.%02d fps)\033[K\r", frame_counter,
	     total_elapsed / 100, total_elapsed % 100,
	     tfps / 100, tfps % 100, frames, elapsed / 100, elapsed % 100,
	     fps / 100, fps % 100);

    last_count = frame_counter;
}
 
static int _mpeg2dec_open (plugin_t *plugin, void *foo)
{
	mpeg2_init ();
	return 0;
}

static int _mpeg2dec_close (plugin_t *plugin)
{
	//FIXME: how are we supposed to call mpeg2_close without
	//the output struct?
	//mpeg2_close(plugin->output);
	print_fps(1);	
	return 0;
}

static int _mpeg2dec_read (plugin_codec_video_t *plugin, buf_t *buf, buf_entry_t *buf_entry)
{       
	int foo;
	
	foo = mpeg2_decode_data (plugin->output, buf_entry->data, buf_entry->data+buf_entry->data_len);

        while (foo--)
		print_fps(0);

        return 0;
}

static int _mpeg2dec_set_flag	(plugin_codec_video_t *plugin, uint flag, uint val)
{
	switch (flag) {
	case FLAG_VIDEO_DROP_FRAME:
                //fprintf (stderr, "%c", val ? '-':'+');
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
