/**
   Temporary dummy prototypes AFTER porting to new
   interface.
   its included in the video_out_xxx drivers.
*/

#include <stdio.h>

/*
 * Dummy prototypes. [START]
 *
 * Fill with content for porting.
*/

static const vo_info_t* get_info (void) {
	return get_info2(NULL);
}
static uint32_t draw_frame(uint8_t *src[]) {
	return draw_frame2(src,NULL);
}
static uint32_t draw_slice (uint8_t *src[], int slice_num) {
	return draw_slice2(src,slice_num,NULL);
}
static void flip_page (void) {
       flip_page2(NULL);
}
static vo_image_buffer_t * allocate_image_buffer (void) {
	return allocate_image_buffer2(0,0,0,NULL);
}
static void free_image_buffer (vo_image_buffer_t* image) {
        free_image_buffer2(image,NULL);
}
/*
 * Dummy prototypes [END]
 *
 */
