
/* 
 *    display_gatos.c by - Stea Greene - Aug 1999
 *
 *  This is an add-on to mpeg2dec, a free MPEG-2 video stream decoder.
 *	
 *  This was possible as a result of the GATOS project and it's members:
 *     http://www.core.binghamton.edu/~insomnia/gatos/
 *  ...and as such should be considered to be part of that project.
 */

/**************************************************************************\
 gatos (General ATI TV and Overlay Software)

  Project Coordinated By Insomnia (Steaphan Greene)
  (insomnia@core.binghamton.edu)

  Copyright (C) 1999 Steaphan Greene, Øyvind Aabling, Octavian Purdila, 
        Vladimir Dergachev and Christian Lupien.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

\**************************************************************************/

//Only define ONE of these
#define GATOS_RAGE_INTERLACED // WORKS WELL
//#define GATOS_R128_INTERLACED // HAS PROBLEMS BUT WORKS.
//#define GATOS_R128_LINEAR // DOESN'T WORK!

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <gatos/gatos.h>

#include <gatos/atiregs.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <string.h>
#include <errno.h>
   
#include "mpeg2.h"
#include "mpeg2_internal.h"

static uint_32 horizontal_size = 720;
static uint_32 vertical_size = 480;
static unsigned long winbg = 0x102030;

void display_frame(uint_8 *src[]) {

#ifdef GATOS_RAGE_INTERLACED
  unsigned char *Y=src[0], *Cb1=src[1], *Cr1=src[2], *Cb2=src[1], *Cr2=src[2];
  unsigned long *dest1=(unsigned long *)(ATIFB+CAPTURE_BUF0_OFFSET);
  unsigned long *dest2=(unsigned long *)(ATIFB+CAPTURE_BUF1_OFFSET);
  unsigned long buf[2];  
  unsigned char *cbuf;
  int h, w;

  dest2 += horizontal_size>>1;
  for (h=0; h!=vertical_size;) {
    ++h;
    if(h&1) {
      for(w=0; w!=horizontal_size>>2;) {
        ++w;
        cbuf = (unsigned char *)buf;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cb2++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr2++;
        (*dest2++) = *buf;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cb2++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr2++;
        (*dest2++) = buf[1];
        }
      }
    else {
      for(w=0; w!=horizontal_size>>2;) {
        ++w;
        cbuf = (unsigned char *)buf;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cb1++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr1++;  
        (*dest1++) = *buf;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cb1++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr1++;
        (*dest1++) = buf[1];
        }
      }
    }
#endif

#ifdef GATOS_R128_LINEAR
  unsigned char *Y=src[0], *Cb1=src[1], *Cr1=src[2], *Cb2=src[1], *Cr2=src[2];
  unsigned long *dest1=(unsigned long *)(ATIFB+CAP0_BUF0_OFFSET);
  unsigned long buf[2];  
  unsigned char *cbuf;
  int h, w;

  for (h=0; h!=vertical_size;) {
    ++h;
    for(w=0; w!=horizontal_size>>2;) {
      ++w;
      cbuf = (unsigned char *)buf;
      (*cbuf++) = *Cb1++;
      (*cbuf++) = *Y++;
      (*cbuf++) = *Cr1++;  
      (*cbuf++) = *Y++;
      (*dest1++) = *buf;
      (*cbuf++) = *Cb1++;
      (*cbuf++) = *Y++;
      (*cbuf++) = *Cr1++;
      (*cbuf++) = *Y++;
      (*dest1++) = buf[1];
      }
    }
#endif

#ifdef GATOS_R128_INTERLACED
  unsigned char *Y=src[0], *Cb1=src[1], *Cr1=src[2], *Cb2=src[1], *Cr2=src[2];
  unsigned long *dest1=(unsigned long *)(ATIFB+CAP0_BUF0_OFFSET);
  unsigned long *dest2=(unsigned long *)(ATIFB+CAP0_BUF1_OFFSET);
  unsigned long buf[2];  
  unsigned char *cbuf;
  int h, w;

  dest2 += horizontal_size>>1;
  for (h=0; h!=vertical_size;) {
    ++h;
    if(h&1) {
      for(w=0; w!=horizontal_size>>2;) {
        ++w;
        cbuf = (unsigned char *)buf;
        (*cbuf++) = *Cb2++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr2++;
        (*cbuf++) = *Y++;
        (*dest2++) = *buf;
        (*cbuf++) = *Cb2++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr2++;
        (*cbuf++) = *Y++;
        (*dest2++) = buf[1];
        }
      }
    else {
      for(w=0; w!=horizontal_size>>2;) {
        ++w;
        cbuf = (unsigned char *)buf;
        (*cbuf++) = *Cb1++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr1++;  
        (*cbuf++) = *Y++;
        (*dest1++) = *buf;
        (*cbuf++) = *Cb1++;
        (*cbuf++) = *Y++;
        (*cbuf++) = *Cr1++;
        (*cbuf++) = *Y++;
        (*dest1++) = buf[1];
        }
      }
    }
#endif
  }

void display_init(uint_32 width, uint_32 height) {
  // ATI video via GATOS
  printf("GATOS init(%d,%d)\n", width, height);
  gatos_setverbosity(1);
  if(gatos_init(GATOS_WRITE_BUFFER|GATOS_OVERLAY))
    { perror(_("display: gatos_init()")); exit(1); }

#ifdef GATOS_RAGE_INTERLACED
  gatos_setcapturesize(width, height);
#endif
#ifdef GATOS_R128_LINER
  gatos_setcapturesize(width, height);
#endif
#ifdef GATOS_R128_INTERLACED
  gatos_setcapturesize(width, height>>1);
#endif

  horizontal_size = width;
  vertical_size = height;
  gatos_enable_video(1);
  gatos_enable_capture(0);
  gatos_setmapped(1);
  gatos_setvisibility(0);
  if(gatos_setcolorkey(winbg)) {
    perror(_("xatitv: gatos_setcolorkey()"));
    exit(1);
    }
  }
