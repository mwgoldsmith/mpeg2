#ifndef __LINUX_MGAVID_H
#define __LINUX_MGAVID_H

#ifndef AARONS_TYPES
typedef unsigned long uint_32;
typedef unsigned char uint_8;
#endif

typedef struct mga_vid_config_s
{
uint_32 src_width;
uint_32 src_height;
uint_32 dest_width;
uint_32 dest_height;
uint_32 x_org;
uint_32 y_org;
uint_8  colkey_on;
uint_8  colkey_red;
uint_8  colkey_green;
uint_8  colkey_blue;
} mga_vid_config_t;

#define MGA_VID_CONFIG _IOR('J', 1, mga_vid_config_t)
#define MGA_VID_ON     _IO ('J', 2)
#define MGA_VID_OFF    _IO ('J', 3)


#endif
