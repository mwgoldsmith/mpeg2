#define VOODOO_IO_REG_OFFSET     ((unsigned long int)0x0000000)
#define VOODOO_YUV_REG_OFFSET    ((unsigned long int)0x0080100)
#define VOODOO_AGP_REG_OFFSET    ((unsigned long int)0x0080000)
#define VOODOO_2D_REG_OFFSET     ((unsigned long int)0x0100000)
#define VOODOO_YUV_PLANE_OFFSET  ((unsigned long int)0x0C00000)

#define VOODOO_BLT_FORMAT_YUYV   (8<<16)
#define VOODOO_BLT_FORMAT_16     (3<<16)

#define VOODOO_YUV_STRIDE        (1024>>2)

typedef unsigned long int uint_32;
typedef unsigned char uint_8;

struct voodoo_yuv_fb_t {
  uint_32 Y[0x0040000];
  uint_32 U[0x0040000];
  uint_32 V[0x0040000];
};

struct voodoo_yuv_reg_t {
  uint_32 yuvBaseAddr;
  uint_32 yuvStride;
};

struct voodoo_2d_reg_t {
  uint_32 status;
  uint_32 intCtrl;
  uint_32 clip0Min;
  uint_32 clip0Max;
  uint_32 dstBaseAddr;
  uint_32 dstFormat;
  uint_32 srcColorkeyMin;
  uint_32 srcColorkeyMax;
  uint_32 dstColorkeyMin;
  uint_32 dstColorkeyMax;
  signed long bresError0;
  signed long bresError1;
  uint_32 rop;
  uint_32 srcBaseAddr;
  uint_32 commandExtra;
  uint_32 lineStipple;
  uint_32 lineStyle;
  uint_32 pattern0Alias;
  uint_32 pattern1Alias;;
  uint_32 clip1Min;
  uint_32 clip1Max;
  uint_32 srcFormat;
  uint_32 srcSize;
  uint_32 srcXY;
  uint_32 colorBack;
  uint_32 colorFore;
  uint_32 dstSize;
  uint_32 dstXY;
  uint_32 command;
  uint_32 RESERVED1;
  uint_32 RESERVED2;
  uint_32 RESERVED3;
  uint_8  launchArea[128];
};


struct voodoo_io_reg_t {
  uint_32 status;
  uint_32 pciInit0;
  uint_32 sipMonitor;
  uint_32 lfbMemoryConfig;
  uint_32 miscInit0;
  uint_32 miscInit1;
  uint_32 dramInit0;
  uint_32 dramInit1;
  uint_32 agpInit;
  uint_32 tmuGbeInit;
  uint_32 vgaInit0;
  uint_32 vgaInit1;
  uint_32 dramCommand;
  uint_32 dramData;
  uint_32 RESERVED1;
  uint_32 RESERVED2;

  uint_32 pllCtrl0;
  uint_32 pllCtrl1;
  uint_32 pllCtrl2;
  uint_32 dacMode;
  uint_32 dacAddr;
  uint_32 dacData;

  uint_32 rgbMaxDelta;
  uint_32 vidProcCfg;
  uint_32 hwCurPatAddr;
  uint_32 hwCurLoc;
  uint_32 hwCurC0;
  uint_32 hwCurC1;
  uint_32 vidInFormat;
  uint_32 vidInStatus;
  uint_32 vidSerialParallelPort;
  uint_32 vidInXDecimDeltas;
  uint_32 vidInDecimInitErrs;
  uint_32 vidInYDecimDeltas;
  uint_32 vidPixelBufThold;
  uint_32 vidChromaMin;
  uint_32 vidChromaMax;
  uint_32 vidCurrentLine;
  uint_32 vidScreenSize;
  uint_32 vidOverlayStartCoords;
  uint_32 vidOverlayEndScreenCoord;
  uint_32 vidOverlayDudx;
  uint_32 vidOverlayDudxOffsetSrcWidth;
  uint_32 vidOverlayDvdy;

  uint_32 vga_registers_not_mem_mapped[12];
  uint_32 vidOverlayDvdyOffset;
  uint_32 vidDesktopStartAddr;
  uint_32 vidDesktopOverlayStride;
  uint_32 vidInAddr0;
  uint_32 vidInAddr1;
  uint_32 vidInAddr2;
  uint_32 vidInStride;
  uint_32 vidCurrOverlayStartAddr;
};


struct pioData_t {
  short port;
  short size;
  int device;
  void *value;
};

typedef struct pioData_t pioData;
typedef struct voodoo_2d_reg_t voodoo_2d_reg;
typedef struct voodoo_io_reg_t voodoo_io_reg;
typedef struct voodoo_yuv_reg_t voodoo_yuv_reg;
typedef struct voodoo_yuv_fb_t voodoo_yuv_fb;






