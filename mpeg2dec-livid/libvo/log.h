#include <stdio.h>
#include <stdarg.h>

#ifndef __LOG
#define __LOG

#define LOG_ERROR		0
#define LOG_WARNING		1
#define LOG_INFO		6
#define LOG_DEBUG		7
#define LOG_DEVEL		8


#ifdef DEBUG
#define LOG(lvl,args...)\
do {\
fprintf (stderr, "%d\t%s:%s#%d\t", lvl, __FILE__, __FUNCTION__, __LINE__);\
fprintf (stderr, args);\
fprintf (stderr, "\n");\
} while (0)
#else
#define LOG(lvl,args...)\
if (lvl != LOG_DEBUG) {\
fprintf (stderr, #lvl"\t%s:%s#%d\t", __FILE__, __FUNCTION__, __LINE__);\
fprintf (stderr, args);\
fprintf (stderr, "\n");\
}
#endif

#endif
