
#ifndef AARONS_TYPES
#define AARONS_TYPES
//typedef to appropriate type for your architecture
typedef unsigned char uint_8;
typedef unsigned short uint_16;
typedef unsigned int uint_32;
typedef signed int sint_32;
typedef signed short sint_16;
typedef signed char sint_8;
#endif

//config flags
#define MPEG2_MMX_ENABLE        0x1
#define MPEG2_3DNOW_ENABLE      0x2
#define MPEG2_SSE_ENABLE        0x4
#define MPEG2_ALTIVEC_ENABLE    0x8

typedef struct mpeg2_config_s
{
	//Bit flags that enable various things
	uint_32 flags;
	//Callback that points the decoder to new stream data
  void   (*fill_buffer_callback)(uint_8 **, uint_8 **);
} mpeg2_config_t;

typedef struct mpeg2_frame_s
{
	uint_32 horizontal_size;
	uint_32 vertical_size;
} mpeg2_frame_t;

//void mpeg2_init(mpeg2_config_t *config);
//void mpeg2_decode_frame(void);
