/*
  definitions for video library
  Copyright (C) 2000  Martin Vogt

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Library General Public License as published by
  the Free Software Foundation.

  For more information look at the file COPYRIGHT in this package

 */

typedef struct vo_frame_s vo_frame_t;
typedef struct vo_instance_s vo_instance_t;

struct vo_frame_s {
    uint8_t * base[3];	/* pointer to 3 planes */
    void (* copy) (vo_frame_t * frame, uint8_t **);
    void (* draw) (vo_frame_t * frame);
    vo_instance_t * this;
};

typedef vo_instance_t * vo_setup_t (vo_instance_t *, int, int);

struct vo_instance_s {
    vo_setup_t * reinit;
    void (* close) (vo_instance_t * this);
    vo_frame_t * (* get_frame) (vo_instance_t * this, int prediction);
};

typedef struct vo_driver_s {
    char * name;
    vo_setup_t * setup;
} vo_driver_t;

/* NULL terminated array of all drivers */
extern vo_driver_t video_out_drivers[];

static vo_instance_t * vo_setup (vo_setup_t * setup, int width, int height)
{
    return setup (NULL, width, height);
}

static vo_instance_t * vo_reinit (vo_instance_t * this, int width, int height)
{
    return this->reinit (this, width, height);
}

static inline void vo_close (vo_instance_t * this)
{
    this->close (this);
}

static inline vo_frame_t * vo_get_frame (vo_instance_t * this, int prediction)
{
    return this->get_frame (this, prediction);
}

static inline void vo_draw (vo_frame_t * frame)
{
    frame->draw (frame);
}
