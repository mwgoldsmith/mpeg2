/* common code to all oms libvo plugins */

#include <inttypes.h>
#include <stdlib.h>
#include "video_out.h"
#include "video_out_internal.h"

int pluginRegister (char *whoami, uint32_t plugin_id, char *fourcc_in,
		    char *fourcc_out, char *xml_config, void *plugin_data);

#define PLUGIN_ID_OUTPUT_VIDEO  0x08 

int plugin_init (char * whoami)
{
    extern vo_output_video_t * video_out_oms_plugin;

    pluginRegister (whoami, PLUGIN_ID_OUTPUT_VIDEO, video_out_oms_plugin->name,
		    NULL, NULL, video_out_oms_plugin);
    return 0;
}

void plugin_exit (void)
{
}

frame_t * libvo_common_alloc (int width, int height)
{
    frame_t * frame;

    if (!(frame = malloc (sizeof (frame_t))))
	return NULL;

    /* we only know how to do 4:2:0 planar yuv right now. */
    if (!(frame->private = malloc (width * height * 3 / 2))) {
	free (frame);
	return NULL;
    }

    frame->base[0] = frame->private;
    frame->base[1] = frame->base[0] + width * height;
    frame->base[2] = frame->base[0] + width * height * 5 / 4;

    return frame;
}

void libvo_common_free (frame_t * frame)
{
    free (frame->private);
    free (frame);
}
