
/**
   Temporary dummy prototypes for porting to new libvo
   interface. its included in the video_out_xxx drivers.
*/

#include <stdio.h>

/*
 * Dummy prototypes. [START]
 *
 * Fill with content for porting.
*/

static const vo_info_t* get_info2 (void *user_data) {
	return NULL;
}
static uint32_t draw_frame2(uint8_t *src[], void *user_data) {
	return 0;
}
static uint32_t draw_slice2 (uint8_t *src[], int slice_num, void*user_data ) {
	return 0;
}
static void flip_page2 (void* user_data) {
}
static vo_image_buffer_t * allocate_image_buffer2 (void *user_data) {
	return NULL;
}
static void free_image_buffer2 (vo_image_buffer_t* image, void *user_data) {
}
/*
 * Dummy prototypes [END]
 *
 */
