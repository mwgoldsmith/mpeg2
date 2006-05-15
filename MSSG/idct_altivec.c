#include <inttypes.h>

/***** altivec-emulation routines, so I can work and test on x86 *****/

typedef union {
    int16_t s[8];
} vector_t;

static int saturate (int i)
{
    if (i > 32767)
	return 32767;
    else if (i < -32768)
	return -32768;
    else
	return i;
}

static vector_t vec_adds (vector_t a, vector_t b)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 8; i++)
	tmp.s[i] = saturate ((int)a.s[i] + b.s[i]);

    return tmp;
}

static vector_t vec_subs (vector_t a, vector_t b)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 8; i++)
	tmp.s[i] = saturate ((int)a.s[i] - b.s[i]);

    return tmp;
}

static vector_t vec_mradds (vector_t a, vector_t b, vector_t c)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 8; i++)
	tmp.s[i] = saturate ((((int)a.s[i] * b.s[i] + 16384) >> 15) + c.s[i]);

    return tmp;
}

static vector_t vec_splat (vector_t a, int b)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 8; i++)
	tmp.s[i] = a.s[b];

    return tmp;
}

/* vec_splat32 is a vec_splat with the right casts - should expand to vspltw */
static vector_t vec_splat32 (vector_t a, int b)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 4; i++) {
	tmp.s[2*i] = a.s[2*b];
	tmp.s[2*i+1] = a.s[2*b+1];
    }

    return tmp;
}

static vector_t vec_splat_s16 (int a)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 8; i++)
	tmp.s[i] = a;

    return tmp;
}

static vector_t vec_mergeh (vector_t a, vector_t b)
{
    vector_t tmp;

    tmp.s[0] = a.s[0];
    tmp.s[1] = b.s[0];
    tmp.s[2] = a.s[1];
    tmp.s[3] = b.s[1];
    tmp.s[4] = a.s[2];
    tmp.s[5] = b.s[2];
    tmp.s[6] = a.s[3];
    tmp.s[7] = b.s[3];

    return tmp;
}

static vector_t vec_mergel (vector_t a, vector_t b)
{
    vector_t tmp;

    tmp.s[0] = a.s[4];
    tmp.s[1] = b.s[4];
    tmp.s[2] = a.s[5];
    tmp.s[3] = b.s[5];
    tmp.s[4] = a.s[6];
    tmp.s[5] = b.s[6];
    tmp.s[6] = a.s[7];
    tmp.s[7] = b.s[7];

    return tmp;
}

static vector_t vec_sra (vector_t a, vector_t b)
{
    vector_t tmp;
    int i;

    for (i = 0; i < 8; i++)
	tmp.s[i] = a.s[i] >> b.s[i];

    return tmp;
}

/***** the actual IDCT routine *****/

#define VEC_S16(a,b,c,d,e,f,g,h) {{a, b, c, d, e, f, g, h}}

static vector_t constants =
    VEC_S16 (23170, 13573, 6518, 21895, -23170, -21895, 32, 31);
static const vector_t constants_1 =
    VEC_S16 (16384, 22725, 21407, 19266, 16384, 19266, 21407, 22725);
static const vector_t constants_2 =
    VEC_S16 (16069, 22289, 20995, 18895, 16069, 18895, 20995, 22289);
static const vector_t constants_3 =
    VEC_S16 (21407, 29692, 27969, 25172, 21407, 25172, 27969, 29692);
static const vector_t constants_4 =
    VEC_S16 (13623, 18895, 17799, 16019, 13623, 16019, 17799, 18895);

#define IDCT								\
    vector_t vx0, vx1, vx2, vx3, vx4, vx5, vx6, vx7;			\
    vector_t vy0, vy1, vy2, vy3, vy4, vy5, vy6, vy7;			\
    vector_t a0, a1, a2, ma2, c4, mc4, zero, bias, shift;		\
    vector_t t0, t1, t2, t3, t4, t5, t6, t7, t8;			\
									\
    c4 = vec_splat (constants, 0);					\
    a0 = vec_splat (constants, 1);					\
    a1 = vec_splat (constants, 2);					\
    a2 = vec_splat (constants, 3);					\
    mc4 = vec_splat (constants, 4);					\
    ma2 = vec_splat (constants, 5);					\
    bias = vec_splat32 (constants, 3);					\
									\
    zero = vec_splat_s16 (0);						\
									\
    vx0 = vec_adds (block[0], block[4]);				\
    vx4 = vec_subs (block[0], block[4]);				\
    t5 = vec_mradds (vx0, constants_1, zero);				\
    t0 = vec_mradds (vx4, constants_1, zero);				\
									\
    vx1 = vec_mradds (a1, block[7], block[1]);				\
    vx7 = vec_mradds (a1, block[1], vec_subs (zero, block[7]));		\
    t1 = vec_mradds (vx1, constants_2, zero);				\
    t8 = vec_mradds (vx7, constants_2, zero);				\
									\
    vx2 = vec_mradds (a0, block[6], block[2]);				\
    vx6 = vec_mradds (a0, block[2], vec_subs (zero, block[6]));		\
    t2 = vec_mradds (vx2, constants_3, zero);				\
    t4 = vec_mradds (vx6, constants_3, zero);				\
									\
    vx3 = vec_mradds (block[3], constants_4, zero);			\
    vx5 = vec_mradds (block[5], constants_4, zero);			\
    t7 = vec_mradds (a2, vx5, vx3);					\
    t3 = vec_mradds (ma2, vx3, vx5);					\
									\
    t6 = vec_adds (t8, t3);						\
    t3 = vec_subs (t8, t3);						\
    t8 = vec_subs (t1, t7);						\
    t1 = vec_adds (t1, t7);						\
    t6 = vec_mradds (a0, t6, t6);	/* a0+1 == 2*c4 */		\
    t1 = vec_mradds (a0, t1, t1);	/* a0+1 == 2*c4 */		\
									\
    t7 = vec_adds (t5, t2);						\
    t2 = vec_subs (t5, t2);						\
    t5 = vec_adds (t0, t4);						\
    t0 = vec_subs (t0, t4);						\
    t4 = vec_subs (t8, t3);						\
    t3 = vec_adds (t8, t3);						\
									\
    vy0 = vec_adds (t7, t1);						\
    vy7 = vec_subs (t7, t1);						\
    vy1 = vec_adds (t5, t3);						\
    vy6 = vec_subs (t5, t3);						\
    vy2 = vec_adds (t0, t4);						\
    vy5 = vec_subs (t0, t4);						\
    vy3 = vec_adds (t2, t6);						\
    vy4 = vec_subs (t2, t6);						\
									\
    vx0 = vec_mergeh (vy0, vy4);					\
    vx1 = vec_mergel (vy0, vy4);					\
    vx2 = vec_mergeh (vy1, vy5);					\
    vx3 = vec_mergel (vy1, vy5);					\
    vx4 = vec_mergeh (vy2, vy6);					\
    vx5 = vec_mergel (vy2, vy6);					\
    vx6 = vec_mergeh (vy3, vy7);					\
    vx7 = vec_mergel (vy3, vy7);					\
									\
    vy0 = vec_mergeh (vx0, vx4);					\
    vy1 = vec_mergel (vx0, vx4);					\
    vy2 = vec_mergeh (vx1, vx5);					\
    vy3 = vec_mergel (vx1, vx5);					\
    vy4 = vec_mergeh (vx2, vx6);					\
    vy5 = vec_mergel (vx2, vx6);					\
    vy6 = vec_mergeh (vx3, vx7);					\
    vy7 = vec_mergel (vx3, vx7);					\
									\
    vx0 = vec_mergeh (vy0, vy4);					\
    vx1 = vec_mergel (vy0, vy4);					\
    vx2 = vec_mergeh (vy1, vy5);					\
    vx3 = vec_mergel (vy1, vy5);					\
    vx4 = vec_mergeh (vy2, vy6);					\
    vx5 = vec_mergel (vy2, vy6);					\
    vx6 = vec_mergeh (vy3, vy7);					\
    vx7 = vec_mergel (vy3, vy7);					\
									\
    vx0 = vec_adds (vx0, bias);						\
    t5 = vec_adds (vx0, vx4);						\
    t0 = vec_subs (vx0, vx4);						\
									\
    t1 = vec_mradds (a1, vx7, vx1);					\
    t8 = vec_mradds (a1, vx1, vec_subs (zero, vx7));			\
									\
    t2 = vec_mradds (a0, vx6, vx2);					\
    t4 = vec_mradds (a0, vx2, vec_subs (zero, vx6));			\
									\
    t7 = vec_mradds (a2, vx5, vx3);					\
    t3 = vec_mradds (ma2, vx3, vx5);					\
									\
    t6 = vec_adds (t8, t3);						\
    t3 = vec_subs (t8, t3);						\
    t8 = vec_subs (t1, t7);						\
    t1 = vec_adds (t1, t7);						\
									\
    t7 = vec_adds (t5, t2);						\
    t2 = vec_subs (t5, t2);						\
    t5 = vec_adds (t0, t4);						\
    t0 = vec_subs (t0, t4);						\
    t4 = vec_subs (t8, t3);						\
    t3 = vec_adds (t8, t3);						\
									\
    vy0 = vec_adds (t7, t1);						\
    vy7 = vec_subs (t7, t1);						\
    vy1 = vec_mradds (c4, t3, t5);					\
    vy6 = vec_mradds (mc4, t3, t5);					\
    vy2 = vec_mradds (c4, t4, t0);					\
    vy5 = vec_mradds (mc4, t4, t0);					\
    vy3 = vec_adds (t2, t6);						\
    vy4 = vec_subs (t2, t6);						\
									\
    shift = vec_splat_s16 (6);						\
    vx0 = vec_sra (vy0, shift);						\
    vx1 = vec_sra (vy1, shift);						\
    vx2 = vec_sra (vy2, shift);						\
    vx3 = vec_sra (vy3, shift);						\
    vx4 = vec_sra (vy4, shift);						\
    vx5 = vec_sra (vy5, shift);						\
    vx6 = vec_sra (vy6, shift);						\
    vx7 = vec_sra (vy7, shift);

static void idct (vector_t * block)
{
    IDCT

    block[0] = vx0;
    block[1] = vx1;
    block[2] = vx2;
    block[3] = vx3;
    block[4] = vx4;
    block[5] = vx5;
    block[6] = vx6;
    block[7] = vx7;
}

void idct_altivec (short * block)
{
    int i;
    int j;
    short block2[64];

    for (i = 0; i < 64; i++)
	block2[i] = block[i];

    for (i = 0; i < 8; i++)
	for (j = 0; j < 8; j++)
	    block[i+8*j] = block2[j+8*i] << 4;

    idct ((vector_t *)block);
}
