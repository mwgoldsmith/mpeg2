#include <inttypes.h>
#include "oms_accel.h"

#ifdef __i386__

#define cpuid(op,eax,ebx,ecx,edx)	\
    asm ("cpuid"			\
	 : "=a" (eax),			\
	   "=b" (ebx),			\
	   "=c" (ecx),			\
	   "=d" (edx)			\
	 : "a" (op)			\
	 : "cc")

static uint32_t x86_accel (void)
{
    uint32_t eax, ebx, ecx, edx;
    int AMD;
    uint32_t caps;

    asm ("pushfl\n\t"
	 "popl %0\n\t"
	 "movl %0,%1\n\t"
	 "xorl $0x200000,%0\n\t"
	 "pushl %0\n\t"
	 "popfl\n\t"
	 "pushfl\n\t"
	 "popl %0"
         : "=a" (eax),
	   "=b" (ebx)
	 :
	 : "cc");

    if (eax == ebx)		// no cpuid
	return 0;

    cpuid (0x00000000, eax, ebx, ecx, edx);
    if (!eax)			// vendor string only
	return 0;

    AMD = (ebx == 0x68747541) && (ecx == 0x444d4163) && (edx == 0x69746e65);

    cpuid (0x00000001, eax, ebx, ecx, edx);
    if (! (edx & 0x00800000))	// no MMX
	return 0;

    caps = OMS_ACCEL_X86_MMX;
    if (edx & 0x02000000)	// SSE - identical to AMD MMX extensions
	caps = OMS_ACCEL_X86_MMX | OMS_ACCEL_X86_MMXEXT;

    cpuid (0x80000000, eax, ebx, ecx, edx);
    if (eax < 0x80000001)	// no extended capabilities
	return caps;

    cpuid (0x80000001, eax, ebx, ecx, edx);

    if (edx & 0x80000000)
	caps |= OMS_ACCEL_X86_3DNOW;

    if (AMD && (edx & 0x00400000))	// AMD MMX extensions
	caps |= OMS_ACCEL_X86_MMXEXT;

    return caps;
}

#endif

uint32_t oms_cpu_accel (void)
{
#ifdef __i386__
    static int got_accel = 0;
    static uint32_t accel;

    if (!got_accel) {
	got_accel = 1;
	accel = x86_accel ();
    }

    return accel;
#else
    return 0;
#endif
}
