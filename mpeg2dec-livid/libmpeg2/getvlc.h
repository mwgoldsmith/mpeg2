/* getvlc.h, variable length code tables */

/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis. The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose. In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders. Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

/* NOTE: #define constants such as MACROBLOCK_QUANT are upper case 
   as per C programming convention. However, the MPEG document 
   (ISO/IEC 13818-2) lists them in all lower case (e.g. Annex B) */

/* NOTE: the VLC tables are in a flash format---a transformation
   of the tables in Annex B to a form more convenient towards 
   parallel (more than one-bit-at-a-time) decoding */

typedef struct {
    char val, len;
} VLCtab;

#define ERROR -1


/* Table B-9, coded_block_pattern, codes 01000 ... 111xx */
static VLCtab CBPtab0[32] = {
    {ERROR,0}, {ERROR,0}, {ERROR,0}, {ERROR,0},
    {ERROR,0}, {ERROR,0}, {ERROR,0}, {ERROR,0},
    {62,5}, {2,5}, {61,5}, {1,5}, {56,5}, {52,5}, {44,5}, {28,5},
    {40,5}, {20,5}, {48,5}, {12,5}, {32,4}, {32,4}, {16,4}, {16,4},
    {8,4}, {8,4}, {4,4}, {4,4}, {60,3}, {60,3}, {60,3}, {60,3}
};

/* Table B-9, coded_block_pattern, codes 00000100 ... 001111xx */
static VLCtab CBPtab1[64] = {
    {ERROR,0}, {ERROR,0}, {ERROR,0}, {ERROR,0},
    {58,8}, {54,8}, {46,8}, {30,8},
    {57,8}, {53,8}, {45,8}, {29,8}, {38,8}, {26,8}, {37,8}, {25,8},
    {43,8}, {23,8}, {51,8}, {15,8}, {42,8}, {22,8}, {50,8}, {14,8},
    {41,8}, {21,8}, {49,8}, {13,8}, {35,8}, {19,8}, {11,8}, {7,8},
    {34,7}, {34,7}, {18,7}, {18,7}, {10,7}, {10,7}, {6,7}, {6,7},
    {33,7}, {33,7}, {17,7}, {17,7}, {9,7}, {9,7}, {5,7}, {5,7},
    {63,6}, {63,6}, {63,6}, {63,6}, {3,6}, {3,6}, {3,6}, {3,6},
    {36,6}, {36,6}, {36,6}, {36,6}, {24,6}, {24,6}, {24,6}, {24,6}
};

/* Table B-9, coded_block_pattern, codes 000000001 ... 000000111 */
static VLCtab CBPtab2[8] = {
    {ERROR,0}, {0,9}, {39,9}, {27,9}, {59,9}, {55,9}, {47,9}, {31,9}
};

/* Table B-1, macroblock_address_increment, codes 00010 ... 011xx */
static VLCtab MBAtab1[16] = {
    {ERROR,0}, {ERROR,0}, {7,5}, {6,5}, {5,4}, {5,4}, {4,4}, {4,4},
    {3,3}, {3,3}, {3,3}, {3,3}, {2,3}, {2,3}, {2,3}, {2,3}
};

/* Table B-1, macroblock_address_increment, codes 00000011000 ... 0000111xxxx */
static VLCtab MBAtab2[104] = {
    {33,11}, {32,11}, {31,11}, {30,11}, {29,11}, {28,11}, {27,11}, {26,11},
    {25,11}, {24,11}, {23,11}, {22,11}, {21,10}, {21,10}, {20,10}, {20,10},
    {19,10}, {19,10}, {18,10}, {18,10}, {17,10}, {17,10}, {16,10}, {16,10},
    {15,8}, {15,8}, {15,8}, {15,8}, {15,8}, {15,8}, {15,8}, {15,8},
    {14,8}, {14,8}, {14,8}, {14,8}, {14,8}, {14,8}, {14,8}, {14,8},
    {13,8}, {13,8}, {13,8}, {13,8}, {13,8}, {13,8}, {13,8}, {13,8},
    {12,8}, {12,8}, {12,8}, {12,8}, {12,8}, {12,8}, {12,8}, {12,8},
    {11,8}, {11,8}, {11,8}, {11,8}, {11,8}, {11,8}, {11,8}, {11,8},
    {10,8}, {10,8}, {10,8}, {10,8}, {10,8}, {10,8}, {10,8}, {10,8},
    {9,7}, {9,7}, {9,7}, {9,7}, {9,7}, {9,7}, {9,7}, {9,7},
    {9,7}, {9,7}, {9,7}, {9,7}, {9,7}, {9,7}, {9,7}, {9,7},
    {8,7}, {8,7}, {8,7}, {8,7}, {8,7}, {8,7}, {8,7}, {8,7},
    {8,7}, {8,7}, {8,7}, {8,7}, {8,7}, {8,7}, {8,7}, {8,7}
};

/* Table B-12, dct_dc_size_luminance, codes 00xxx ... 11110 */
static VLCtab DClumtab0[32] = {
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
    {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}, {ERROR, 0}
};

/* Table B-12, dct_dc_size_luminance, codes 111110xxx ... 111111111 */
static VLCtab DClumtab1[16] = {
    {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6},
    {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10,9}, {11,9}
};

/* Table B-13, dct_dc_size_chrominance, codes 00xxx ... 11110 */
static VLCtab DCchromtab0[32] = {
    {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
    {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
    {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
    {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}, {ERROR, 0}
};

/* Table B-13, dct_dc_size_chrominance, codes 111110xxxx ... 1111111111 */
static VLCtab DCchromtab1[32] = {
    {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
    {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6}, {6, 6},
    {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7}, {7, 7},
    {8, 8}, {8, 8}, {8, 8}, {8, 8}, {9, 9}, {9, 9}, {10,10}, {11,10}
};