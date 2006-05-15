#ifndef __LINUX_SYNCFB_H
#define __LINUX_SYNCFB_H

#ifdef __KERNEL__
#include <linux/config.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/malloc.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/videodev.h>

#ifdef CONFIG_MTRR
#include <asm/mtrr.h>
#endif

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/io.h>

#define TRUE 1
#define FALSE 0

#define SFB_STATUS_FREE 0
#define SFB_STATUS_OFFS 1
#define SFB_STATUS_WAIT 2
#define SFB_STATUS_LIVE 3

#endif /* KERNEL */


#ifndef AARONS_TYPES
typedef unsigned long uint_32;
typedef unsigned char uint_8;
#endif

#define SYNCFB_MAJOR 178

#define SYNCFB_ERROR_NO_ERROR                 0;
#define SYNCFB_ERROR_NO_BUFFER_AVAILABLE      1;
#define SYNCFB_ERROR_PALETTE_NOT_SUPPORTED    2;
#define SYNCFB_ERROR_NOT_ENOUGH_MEMORY        3;



#ifndef __LINUX_VIDEODEV_H
#define VIDEO_PALETTE_GREY      1       /* Linear greyscale */
#define VIDEO_PALETTE_HI240     2       /* High 240 cube (BT848) */
#define VIDEO_PALETTE_RGB565    3       /* 565 16 bit RGB */
#define VIDEO_PALETTE_RGB24     4       /* 24bit RGB */
#define VIDEO_PALETTE_RGB32     5       /* 32bit RGB */
#define VIDEO_PALETTE_RGB555    6       /* 555 15bit RGB */
#define VIDEO_PALETTE_YUV422    7       /* YUV422 capture */
#define VIDEO_PALETTE_YUYV      8
#define VIDEO_PALETTE_UYVY      9       /* The great thing about standards is ... */
#define VIDEO_PALETTE_YUV420    10
#define VIDEO_PALETTE_YUV411    11      /* YUV411 capture */
#define VIDEO_PALETTE_RAW       12      /* RAW capture (BT848) */
#define VIDEO_PALETTE_YUV422P   13      /* YUV 4:2:2 Planar */
#define VIDEO_PALETTE_YUV411P   14      /* YUV 4:1:1 Planar */
#define VIDEO_PALETTE_YUV420P   15      /* YUV 4:2:0 Planar */
#define VIDEO_PALETTE_YUV410P   16      /* YUV 4:1:0 Planar */
#define VIDEO_PALETTE_PLANAR    13      /* start of planar entries */
#define VIDEO_PALETTE_COMPONENT 7       /* start of component entries */
#endif


#define VIDEO_PALETTE_YUV422P3  13      /* YUV 4:2:2 Planar (3 Plane, same as YUV422P) */
#define VIDEO_PALETTE_YUV422P2  17      /* YUV 4:2:2 Planar (2 Plane) */

#define VIDEO_PALETTE_YUV411P3  14      /* YUV 4:1:1 Planar (3 Plane, same as YUV411P) */
#define VIDEO_PALETTE_YUV411P2  18      /* YUV 4:1:1 Planar (2 Plane) */

#define VIDEO_PALETTE_YUV420P3  15      /* YUV 4:2:0 Planar (3 Plane, same as YUV420P) */
#define VIDEO_PALETTE_YUV420P2  19      /* YUV 4:2:0 Planar (2 Plane) */

#define VIDEO_PALETTE_YUV410P3  16      /* YUV 4:1:0 Planar (3 Plane, same as YUV410P) */
#define VIDEO_PALETTE_YUV410P2  20      /* YUV 4:1:0 Planar (2 Plane) */



#define SYNCFB_FEATURE_SCALE_H           1
#define SYNCFB_FEATURE_SCALE_V           2
#define SYNCFB_FEATURE_SCALE             3
#define SYNCFB_FEATURE_CROP              4 
#define SYNCFB_FEATURE_OFFSET            8
#define SYNCFB_FEATURE_DEINTERLACE      16 
#define SYNCFB_FEATURE_PROCAMP          32 
#define SYNCFB_FEATURE_TRANSITIONS      64 
#define SYNCFB_FEATURE_COLKEY          128 
#define SYNCFB_FEATURE_MIRROR_H        256 
#define SYNCFB_FEATURE_MIRROR_V        512 
#define SYNCFB_FEATURE_BLOCK_REQUEST  1024 
#define SYNCFB_FEATURE_FREQDIV2       2048 


typedef struct syncfb_config_s
{
	uint_32 syncfb_mode;		/* bitfield: turn on/off the available features */
	uint_32 error_code;		/* RO: returns 0 on successful config calls, error code otherwise */

	uint_32 fb_screen_size;		/* WO, size in bytes of video memory reserved for fbdev */
	uint_32 fb_screen_width;	/* WO, visible screen width in pixel */
	uint_32 fb_screen_height;	/* WO, visible screen height in pixel */

	uint_32 buffers;		/* RO, number of available buffers */
	uint_32 buffer_size;		/* RO, filled in by syncfb */

	uint_32 default_repeat; 	/* default repeat time for a single frame, can be overridden in syncfb_buffer_info_t */

	uint_32 src_width;		/* source image width in pixel */
	uint_32 src_height;		/* source image height in pixel */
	uint_32 src_palette;		/* set palette mode, see videodev.h for palettes */
	uint_32 src_pitch;		/* RO: filled in by ioctl: actual line length in pixel */

	uint_32 image_xorg;		/* x position of the image on the screen */
	uint_32 image_yorg;		/* y position of the image on the screen */

	/* if syncfb has FEATURE_SCALE */
	uint_32 scale_filters;		/* 0: no filtering, 255: all filters on */
	uint_32 image_width;		/* onscreen image width */
	uint_32 image_height;		/* onscreen image height */
	
	/* if syncfb has FEATURE_CROP */
	uint_32 src_crop_left;		/* */
	uint_32 src_crop_right;		/* */
	uint_32 src_crop_top;		/* */
	uint_32 src_crop_bot;		/* */

	/* if syncfb has FEATURE_OFFSET */
	uint_32 image_offset_left;	/* */
	uint_32 image_offset_right;	/* */
	uint_32 image_offset_top;	/* */
	uint_32 image_offset_bot;	/* */

	/* if syncfb has FEATURE_COLKEY */
	uint_8  colkey_red;
	uint_8  colkey_green;
	uint_8  colkey_blue;

} syncfb_config_t;


/*
	picture parameters,
*/
typedef struct syncfb_param_s
{
	/* the idea is to enable smooth transitions between eg. image sizes (not yet implemented) */
	/* if syncfb has FEATURE_TRANSITIONS */
	uint_32 transition_time;

	/* if syncfb has FEATURE_PROCAMP */
	uint_32 contrast;		/* 0: least contrast, 1000: normal contrast,  */
	uint_32 brightness;
	uint_32 color;       /* for syncfb_matrox: color=0: b/w else: full color */

	/* if syncfb has FEATURE_SCALE , currently only supported in CONFIG call */
	uint_8  scale_filters;  /* 0: no filtering, 255: all filters on */
	uint_32 image_xorg;     /* x position of the image on the screen */
	uint_32 image_yorg;     /* y position of the image on the screen */
	uint_32 image_width;    /* onscreen image width */
	uint_32 image_height;   /* onscreen image height */

} syncfb_param_t;



typedef struct syncfb_status_info_s
{
	uint_32 field_cnt;			/* basically all vbi's since the start of syncfb */
	uint_32 frame_cnt;			/* number of frames comitted & output */

	uint_32 hold_field_cnt;			/* number of repeated fields becaus no new data was available */
	uint_32 skip_field_cnt;			/* skipped fields when fifo was about to fill up */

	uint_32 request_frames;			/* number of request_buffer calls */
	uint_32 commit_frames; 			/* number of commit_buffer calls */
	
	uint_32 failed_requests;		/* number of calls to request_buffer that failed */

	uint_32 buffers_waiting;
	uint_32 buffers_free;

} syncfb_status_info_t;




typedef struct syncfb_capability_s 
{
	char name[64];			/* A name for the syncfb ... */
	uint_32 palettes;		/* supported palettes - see videodev.h for palettes, test the corresponding bit here */
	uint_32 features;		/* supported features - see SYNCFB_FEATURE_* */
	uint_32 memory_size;		/* total size of mappable video memory */

} syncfb_capability_t;



typedef struct syncfb_buffer_info_s
{
	int     id;			/* buffer id: a return value of -1 means no buffer available */
	uint_32 repeat;			/* the buffer will be shown <repeat> times */
	uint_32 offset;			/* buffer offset from start of video memory */
	uint_32 offset_p2;		/* yuv plane 2 buffer offset from start of video memory */
	uint_32 offset_p3;		/* yuv plane 3 buffer offset from start of video memory */

} syncfb_buffer_info_t;







/* get syncfb capabilities */
#define SYNCFB_GET_CAPS _IOR('J', 1, syncfb_config_t)

#define SYNCFB_GET_CONFIG _IOR('J', 2, syncfb_config_t)
#define SYNCFB_SET_CONFIG _IOR('J', 3, syncfb_config_t)
#define SYNCFB_ON     _IO ('J', 4)
#define SYNCFB_OFF    _IO ('J', 5)
#define SYNCFB_REQUEST_BUFFER    _IOR ('J', 6, syncfb_buffer_info_t)
#define SYNCFB_COMMIT_BUFFER    _IOR ('J', 7, syncfb_buffer_info_t)
#define SYNCFB_STATUS    _IOR ('J', 8, syncfb_status_info_t)
#define SYNCFB_VBI    _IO ('J', 9)  /* simulate interrupt - debugging only */
#define SYNCFB_SET_PARAMS _IOR('J', 10, syncfb_param_t)
#define SYNCFB_GET_PARAMS _IOR('J', 11, syncfb_param_t)




#ifdef __KERNEL__

typedef struct syncfb_device_s
{
	syncfb_capability_t *(*capability) (void);
	void (*enable) (void);
	void (*disable) (void);
	void (*cleanup) (void);
	void (*request_buffer) (syncfb_buffer_info_t *buf);
	void (*select_buffer) (uint_8 buf_id);
	int  (*configure) (uint_8 set, syncfb_config_t *config);
	int  (*ioctl) (struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg); /* optional: can be NULL */
	int  (*mmap) (struct file *file, struct vm_area_struct *vma);

} syncfb_device_t;


int syncfb_interrupt(void);
uint_8 syncfb_get_bpp(uint_8 pal);
 
/*
	SUPPORTED HARDWARE
*/
#ifdef SYNCFB_MATROX_SUPPORT
syncfb_device_t *syncfb_get_matrox_device(void);
#endif

#ifdef SYNCFB_GENERIC_SUPPORT
syncfb_device_t *syncfb_get_generic_device(void); 
#endif

#endif /* KERNEL */
#endif /* __LINUX_SYNCFB_H */

