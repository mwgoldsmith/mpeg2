/* 
 * video_out_pgm.c, pgm interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "video_out.h"
#include "video_out_internal.h"

LIBVO_EXTERN (pgm)

static vo_info_t vo_info = 
{
	"PGM",
	"pgm",
	"walken",
	""
};

static int image_width;
static int image_height;
static char header[1024];
static int framenum = -2;

static uint32_t
init(int width, int height, int fullscreen, char *title, uint32_t format)
{
    image_height = height;
    image_width = width;

    sprintf (header, "P5\n\n%d %d\n255\n", width, height*3/2);

    return 0;
}

static const vo_info_t*
get_info(void)
{
    return &vo_info;
}

static void flip_page (void)
{
}

static uint32_t draw_slice(uint8_t * src[], int slice_num)
{
    return 0;
}

uint32_t output_pgm_frame (char * fname, uint8_t * src[])
{
    FILE * f;
    int i;

    f = fopen (fname, "wb");
    if (f == NULL) return 1;
    fwrite (header, strlen (header), 1, f);
    fwrite (src[0], image_width, image_height, f);
    for (i = 0; i < image_height/2; i++) {
	fwrite (src[1]+i*image_width/2, image_width/2, 1, f);
	fwrite (src[2]+i*image_width/2, image_width/2, 1, f);
    }
    fclose (f);

    return 0;
}

static uint32_t draw_frame(uint8_t * src[])
{
    char buf[100];

    if (++framenum < 0)
	return 0;

    sprintf (buf, "%d.pgm", framenum);
    return output_pgm_frame (buf, src);
}

static vo_image_buffer_t* 
allocate_image_buffer()
{
    return allocate_image_buffer_common(image_height,image_width,0x32315659);
}

static void	
free_image_buffer(vo_image_buffer_t* image)
{
    free_image_buffer_common(image);
}
