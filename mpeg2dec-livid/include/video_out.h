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
    void (* field) (vo_frame_t * frame, int flags);
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

/* return NULL terminated array of all drivers */
vo_driver_t * vo_drivers (void);

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

#define VO_TOP_FIELD 1
#define VO_BOTTOM_FIELD 2
#define VO_BOTH_FIELDS (VO_TOP_FIELD | VO_BOTTOM_FIELD)
#define VO_PREDICTION_FLAG 4

static inline vo_frame_t * vo_get_frame (vo_instance_t * this, int flags)
{
    return this->get_frame (this, flags);
}

static inline void vo_field (vo_frame_t * frame, int flags)
{
    if (frame->field != NULL)
	frame->field (frame, flags);
}

static inline void vo_draw (vo_frame_t * frame)
{
    frame->draw (frame);
}
