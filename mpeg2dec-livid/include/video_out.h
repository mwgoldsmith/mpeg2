/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */

typedef struct frame_s {
    uint8_t * base[3];	/* pointer to 3 planes */
    void * private;
} frame_t;

typedef struct vo_instance_s vo_instance_t;

typedef vo_instance_t * vo_setup_t (vo_instance_t *, int, int);

struct vo_instance_s {
    vo_setup_t * setup;
    int (* close) (vo_instance_t * this);
    frame_t * (* get_frame) (vo_instance_t * this, int prediction);
    void (* draw_frame) (frame_t * frame);	/* FIXME */
};

typedef struct vo_driver_s {
    char * name;
    vo_setup_t * setup;
} vo_driver_t;

/* NULL terminated array of all drivers */
extern vo_driver_t video_out_drivers[];

static vo_instance_t * vo_setup (vo_instance_t * this, int width, int height)
{
    return this->setup (this, width, height);
}

static inline int vo_close (vo_instance_t * this)
{
    return this->close (this);
}

static inline frame_t * vo_get_frame (vo_instance_t * this, int prediction)
{
    return this->get_frame (this, prediction);
}
