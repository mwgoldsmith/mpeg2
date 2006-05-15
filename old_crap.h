#if 0
void Initialize_Buffer(void); 
int Get_Byte(void);
int Get_Word(void);
unsigned int Show_Bits(int N);
unsigned int Get_Bits1(void);
void Flush_Buffer(int N);
unsigned int Get_Bits(int N);
void Next_Packet(void);
void Flush_Buffer32(void);
unsigned int Get_Bits32(void);
int Get_Long(void);
#endif

int Get_macroblock_type(int picture_coding_type);
int Get_motion_code();
int Get_dmvector();
int Get_coded_block_pattern();
int Get_macroblock_address_increment();
int Get_Luma_DC_dct_diff();
int Get_Chroma_DC_dct_diff();
void motion_vectors(int PMV[2][2][2],int dmvector[2], int motion_vertical_field_select[2][2],int s,int motion_vector_count,int mv_format,int h_r_size,int v_r_size,int dmv,int mvscale);
