/*
 * cpu_accel.c
 * Copyright (C) 2000-2004 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with mpeg2dec; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#if defined(ARCH_ALPHA)

#include <inttypes.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

static inline uint32_t arch_accel (uint32_t accel) {
  if (accel & MPEG2_ACCEL_ALPHA_MVI)
	  accel |= MPEG2_ACCEL_ALPHA;

#ifdef ACCEL_DETECT
  if (accel & MPEG2_ACCEL_DETECT) {
  	uint64_t no_mvi;

	  asm volatile ("amask %1, %0"
		      : "=r" (no_mvi)
		      : "rI" (256));	/* AMASK_MVI */
	  accel |= no_mvi ? MPEG2_ACCEL_ALPHA : (MPEG2_ACCEL_ALPHA | MPEG2_ACCEL_ALPHA_MVI);
    }
#endif /* ACCEL_DETECT */

  return accel;
}

uint32_t mpeg2_detect_accel (uint32_t accel) {
  accel = arch_accel(accel);
  return accel;
}

#endif /* ARCH_ALPHA */