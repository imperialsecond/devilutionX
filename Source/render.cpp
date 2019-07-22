#include "diablo.h"
#include <algorithm>

DEVILUTION_BEGIN_NAMESPACE

BYTE *gpBufStart;

unsigned int RightMask[32] = {
	0xEAAAAAAA, 0xF5555555,
	0xFEAAAAAA, 0xFF555555,
	0xFFEAAAAA, 0xFFF55555,
	0xFFFEAAAA, 0xFFFF5555,
	0xFFFFEAAA, 0xFFFFF555,
	0xFFFFFEAA, 0xFFFFFF55,
	0xFFFFFFEA, 0xFFFFFFF5,
	0xFFFFFFFE, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF
};

unsigned int LeftMask[32] = {
	0xAAAAAAAB, 0x5555555F,
	0xAAAAAABF, 0x555555FF,
	0xAAAAABFF, 0x55555FFF,
	0xAAAABFFF, 0x5555FFFF,
	0xAAABFFFF, 0x555FFFFF,
	0xAABFFFFF, 0x55FFFFFF,
	0xABFFFFFF, 0x5FFFFFFF,
	0xBFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF,
	0xFFFFFFFF, 0xFFFFFFFF
};

// Textures:
// 0x0 (square).
struct square_texture {
  const uint8_t* src;
  bool operator()(uint8_t* dst) { *dst = *src++; return true; }
  void next_row() {}
};

// 0x1 (partially transparent)
struct skip_texture {
  const uint8_t* src;
  int n = 0;
  bool operator()(uint8_t* dst) {
    if (!n) n = (signed char)*src++;
    if (n < 0) {
      ++n;
      return false;
    }
    --n;
    *dst = *src++;
    return true;
  }
  void next_row() {}
};

// Texture with transparency and RLE.
struct rle_texture {
  const uint8_t* src;
  char fill;
  int fillbytes = 0;
  int skipbytes = 0;
  int copybytes = 0;

  bool operator()(uint8_t* dst) {
    if (fillbytes == 0 && skipbytes == 0 && copybytes == 0) {
      int width = (signed char)*src++;
      if (width > 0) {
        skipbytes = width;
      } else {
        width = -width;
        if (width > 65) {
          fill = *src++;
          fillbytes = width - 65;
        } else {
          copybytes = width;
        } 
      }
    }
    if (skipbytes > 0) {
      --skipbytes;
      return false;
    }
    if (fillbytes > 0) {
      --fillbytes;
      *dst = fill;
    } else {
      --copybytes;
      *dst = *src++;
    }
    return true;
  }
  void next_row() {}

};

// Triangular (2: <|, 3: |>).
struct triangular_texture {
  const uint8_t* src;
  bool aligned;

  bool operator()(uint8_t* dst) { *dst = *src++; return true; }
  void next_row() {
    aligned = !aligned;
    if (aligned) src += 2;
  }
};

// Trapezoidal 
//  |-|   4
//   \|
//  |-|   5
//  |/
struct trapezoidal_texture {
  const uint8_t* src;
  bool aligned;
  int num_rows = 0;

  bool operator()(uint8_t* dst) { *dst = *src++; return true; }
  void next_row() {
    if (++num_rows >= 16) return;
    aligned = !aligned;
    if (aligned) src += 2;
  }
};

// Masks:
struct masked {
  const uint32_t* mask;
  uint32_t current_mask = 0;

  bool operator()() {
    bool enable = current_mask & 0x80000000;
    current_mask <<= 1;
    return enable;
  }

  void next_row() { current_mask = *mask--; }
};

struct solid {
  bool operator()() { return true; }
  void next_row() {}
};

struct checkered {
  bool active = false;
  bool operator()() { return active = !active; }
  void next_row() { active = !active; }
};

// Color transforms:
uint8_t identity(uint8_t src) { return src; }
uint8_t black(uint8_t) { return 0; }

struct lit {
  const uint8_t* tbl;
  uint8_t operator()(uint8_t src) { return tbl[src]; }
};

template <typename Texture, typename Mask, typename Transform>
static void drawRow(BYTE*& dst, int width, bool left_aligned,
    Texture&& texture, Mask&& mask, Transform&& transform, bool bounds_check = true) {
  texture.next_row();
  mask.next_row();

  int offset = left_aligned ? 0 : (32 - width);
  uint8_t col;
  if (!bounds_check || (gpBufStart <= dst && dst < gpBufEnd)) {
    for (int i = 0; i < width; ++i) {
      // NO short-circuit.
      if (mask() & texture(&col)) {
        dst[offset+i] = transform(col);
      }
    }
  } else {
    for (int i = 0; i < width; ++i) {
      texture(&col);
    }
  }
  dst -= 768;
}

template <typename Mask, typename Transform>
static void drawTile(BYTE* dst, Mask&& mask, Transform&& transform) {
	const uint8_t* src = (unsigned char *)pDungeonCels + *((DWORD *)pDungeonCels + (level_cel_block & 0xFFF));
  int cel_type_16 = (level_cel_block >> 12) & 7;
  bool left_aligned = cel_type_16 & 1;
	switch (cel_type_16 & 7) {
    case 0: {
      square_texture t{src};
      for (int i = 0; i < 32; ++i) {
        drawRow(dst, 32, true, t, mask, transform);
      }
      break;
    }
    case 1: {
      skip_texture tex{src};
      for (int y = 0; y < 32; ++y) {
        drawRow(dst, 32, true, tex, mask, transform);
      }
      break;
    }
    case 2:
    case 3: {
      triangular_texture t{src, left_aligned};
      for (int i = 0; i < 16; ++i) {
        drawRow(dst, (i + 1) * 2, left_aligned, t, mask, transform);
      }
      for (int i = 30; i > 0; i -= 2) {
        drawRow(dst, i, left_aligned, t, mask, transform);
      }
      break;
    }
    default: {
      trapezoidal_texture t{src, left_aligned};
      for (int i = 0; i < 16; ++i) {
        drawRow(dst, 2 * (i+1), left_aligned, t, mask, transform);
      }
      for (int i = 0; i < 16; ++i) {
        drawRow(dst, 32, left_aligned, t, mask, transform);
      }
      break;
    }
	}
}

template <typename Mask>
void applyLighting(BYTE *pBuff, Mask&& mask) {
  if (light_table_index == lightmax) {
    drawTile(pBuff, mask, black);
  } else if (light_table_index) {
	  unsigned char* tbl = &pLightTbl[256 * light_table_index];
    drawTile(pBuff, mask, lit{tbl});
	} else {
    drawTile(pBuff, mask, identity);
  }
}

void drawUpperScreen(BYTE *pBuff)
{
  drawLowerScreen(pBuff);
}

void drawLowerScreen(BYTE *pBuff)
{
	if (cel_transparency_active) {
		if (!arch_draw_type) {
      applyLighting(pBuff, checkered{});
			return;
		} else if (arch_draw_type == 1) {
			if (block_lvid[level_piece_id] == 1 || block_lvid[level_piece_id] == 3) {
        applyLighting(pBuff, masked{&LeftMask[31]});
				return;
			}
		} else if (arch_draw_type == 2) {
			if (block_lvid[level_piece_id] == 2 || block_lvid[level_piece_id] == 3) {
        applyLighting(pBuff, masked{&RightMask[31]});
				return;
			}
		}
	}
  applyLighting(pBuff, solid{});
}

void world_draw_black_tile(BYTE *pBuff)
{
	unsigned char *dst = pBuff;
  for (int y = 0; y < 31; ++y) {
    int dy = abs(y - 15);
    int startx = 2 * dy;
    int width = 64 - 4 * dy;
    memset(dst + 2 * dy, 0, width);
    dst -= 768;
  }
}

void Cel2DecDatOnly(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int texWidth)
{
  skip_texture tex{pRLEBytes};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, texWidth, true, tex, solid{}, identity);
  }
}

void CelDrawDatOnly(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int texWidth)
{
  skip_texture tex{pRLEBytes};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, texWidth, true, tex, solid{}, identity, false /* bounds check */);
  }
}

void Cel2DecDatLightOnly(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int texWidth)
{
	BYTE *tbl = &pLightTbl[light_table_index * 256];
  skip_texture tex{pRLEBytes};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, texWidth, true, tex, solid{}, lit{tbl});
  }
}

void CelDecDatLightOnly(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int texWidth)
{
	BYTE *tbl = &pLightTbl[light_table_index * 256];
  skip_texture tex{pRLEBytes};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, texWidth, true, tex, solid{}, lit{tbl}, false /* bounds check */);
  }
}

void Cel2DecDatLightTrans(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int texWidth)
{
	BYTE *tbl = &pLightTbl[light_table_index * 256];
  skip_texture tex{pRLEBytes};
  checkered mask{};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, texWidth, true, tex, mask, lit{tbl});
  }
}

void CelDecDatLightTrans(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int texWidth)
{
	BYTE *tbl = &pLightTbl[light_table_index * 256];
  skip_texture tex{pRLEBytes};
  checkered mask{};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, texWidth, true, tex, mask, lit{tbl}, false /* bounds check */);
  }
}

void Cl2DecDatLightTbl2(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int nWidth, BYTE *pTable)
{
  rle_texture tex{pRLEBytes};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, nWidth, true, tex, solid{}, lit{pTable});
  }
}

void Cl2DecDatFrm4(BYTE *dst, BYTE *pRLEBytes, int nDataSize, int nWidth)
{
  rle_texture tex{pRLEBytes};
  while (tex.src < pRLEBytes + nDataSize) {
    drawRow(dst, nWidth, true, tex, solid{}, identity);
  }
}

DEVILUTION_END_NAMESPACE
