static __inline void asm_cel_light_edge(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline void asm_cel_light_transform(unsigned char w, BYTE *tbl, BYTE *dst, BYTE *src);
static __inline void asm_cel_light_square(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline void asm_trans_light_cel_0_2(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline void asm_trans_light_square_0_2(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline void asm_trans_light_cel_1_3(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline void asm_trans_light_edge_1_3(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline void asm_trans_light_square_1_3(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src);
static __inline unsigned int asm_trans_light_mask(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src, unsigned int mask);

static __inline void asm_cel_light_transform(unsigned char w, BYTE *tbl, BYTE *dst, BYTE *src)
{
  for (int i = 0; i < w; ++i) dst[i] = tbl[src[i]];
}

static __inline void asm_cel_light_edge(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src)
{
  asm_cel_light_transform(w, tbl, *dst, *src);
  *src += w;
  *dst += w;
}

static __inline void asm_cel_light_square(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src)
{
  asm_cel_light_edge(w * 4, tbl, dst, src);
}

static __inline void asm_trans_light_cel_0_2(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src)
{
  for (int i = 1; i < w; i += 2) (*dst)[i] = tbl[(*src)[i]];
  (*dst) += w;
  (*src) += w;
}

static __inline void asm_trans_light_square_0_2(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src)
{
  asm_trans_light_cel_1_3(w * 4, tbl, dst, src);
}

static __inline void asm_trans_light_cel_1_3(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src)
{
  for (int i = 0; i < w; i += 2) (*dst)[i] = tbl[(*src)[i]];
  (*dst) += w;
  (*src) += w;
}

static __inline void asm_trans_light_square_1_3(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src)
{
  asm_trans_light_cel_0_2(w * 4, tbl, dst, src);
}

static __inline unsigned int asm_trans_light_mask(unsigned char w, BYTE *tbl, BYTE **dst, BYTE **src, unsigned int mask)
{
	for (; w; --w, (*src)++, (*dst)++, mask *= 2) {
		if (mask & 0x80000000)
			(*dst)[0] = tbl[(*src)[0]];
	}

	return mask;
}
