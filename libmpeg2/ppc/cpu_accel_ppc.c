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

#if defined(ARCH_PPC)
#if defined(ACCEL_DETECT)

#include <inttypes.h>

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

#include <signal.h>
#include <setjmp.h>

static sigjmp_buf jmpbuf;
static volatile sig_atomic_t canjump = 0;

static RETSIGTYPE sigill_handler (int sig) {
  if (!canjump) {
	  signal (sig, SIG_DFL);
	  raise (sig);
  }

  canjump = 0;
  siglongjmp (jmpbuf, 1);
}

static uint32_t arch_accel (uint32_t accel) {
  if ((accel & (MPEG2_ACCEL_PPC_ALTIVEC | MPEG2_ACCEL_DETECT)) ==	MPEG2_ACCEL_DETECT) {
  	static RETSIGTYPE (* oldsig) (int);

	  oldsig = signal (SIGILL, sigill_handler);
	  if (sigsetjmp (jmpbuf, 1)) {
	    signal (SIGILL, oldsig);
	    return accel;
	  }

	  canjump = 1;

#if defined(__APPLE_CC__)	/* apple */
#  define VAND(a,b,c) "vand v" #a ",v" #b ",v" #c "\n\t"
#else				/* gnu */
#  define VAND(a,b,c) "vand " #a "," #b "," #c "\n\t"
#endif
	    asm volatile ("mtspr 256, %0\n\t"
		      VAND (0, 0, 0)
		      :
		      : "r" (-1));

	  canjump = 0;
	  accel |= MPEG2_ACCEL_PPC_ALTIVEC;

	  signal (SIGILL, oldsig);
  }

  return accel;
}

uint32_t mpeg2_detect_accel (uint32_t accel) {
  accel = arch_accel(accel);
  return accel;
}

#endif /* ACCEL_DETECT */
#endif /* ARCH_PPC */
