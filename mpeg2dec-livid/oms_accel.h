// generic accelerations
#define OMS_ACCEL_MLIB		0x00000001

// x86 accelerations
#define OMS_ACCEL_X86_MMX	0x80000000
#define OMS_ACCEL_X86_3DNOW	0x40000000
#define OMS_ACCEL_X86_MMXEXT	0x20000000

uint32_t oms_cpu_accel (void);
