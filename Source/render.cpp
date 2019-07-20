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

// 0x0 (square).
template <typename T>
struct square_texture {
  T transform;
  const uint8_t* src;
  bool operator()(uint8_t* dst) { 
    return transform(*src++, dst);
  }
  void next_row() { transform.next_row(); }
};

// 0x1 (skip)
template <typename T>
struct skip_texture {
  T transform;
  const uint8_t* src;
  int skip = 0;
  int copy = 0;
  bool operator()(uint8_t* dst) {
    if (copy == 0 && skip == 0) {
      int width = (signed char)*src++;
      if (width < 0) {
        skip = -width;
      } else {
        copy = width;
      }
    }
    if (copy > 0) {
      --copy;
      return transform(*src++, dst);
    }
    if (skip > 0) {
      uint8_t dummy;
      transform(0, &dummy);
      --skip;
      return false;
    }
  }
  void next_row() {
    transform.next_row();
  }
};

// Triangular (2: <|, 3: |>).
template <typename T>
struct triangular_texture {
  T transform;
  const uint8_t* src;
  bool aligned;

  bool operator()(uint8_t* dst) {
    return transform(*src++, dst);
  }
  void next_row() {
    transform.next_row();
    aligned = !aligned;
    if (aligned) src += 2;
  }
};

// Trapezoidal 
//  |-|   4
//   \|
//  |-|   5
//  |/
template <typename T>
struct trapezoidal_texture {
  T transform;
  const uint8_t* src;
  bool aligned;
  int num_rows = 0;

  uint8_t operator()(uint8_t* dst) { return transform(*src++, dst); }
  void next_row() {
    transform.next_row();
    if (++num_rows >= 16) return;
    aligned = !aligned;
    if (aligned) src += 2;
  }
};

struct identity {
  bool operator()(uint8_t src, uint8_t* dst) {
    *dst = src;
    return true;
  }
  void next_row() {}
};

struct black {
  uint8_t operator()(uint8_t, uint8_t* dst) {
    *dst = 0;
    return true;
  }
  void next_row() { }
};

template <typename N>
struct masked {
  const uint32_t* mask;
  N next;
  uint32_t current_mask = 0;

  uint8_t operator()(uint8_t src, uint8_t* dst) {
    bool enable = current_mask & 0x80000000;
    current_mask <<= 1;
    if (enable) {
      return next(src, dst);
    } else {
      uint8_t dummy;
      next(src, &dummy);
      return false;
    }
  }

  void next_row() {
    next.next_row();
    current_mask = *mask--;
  }
};

template <typename N>
masked<N> make_masked(const uint32_t* mask, N&& n) { return {mask, n}; }

struct lit {
  const uint8_t* tbl;

  uint8_t operator()(uint8_t src, uint8_t* dst) {
    *dst = tbl[src];
    return true;
  }
  void next_row() { }
};

template <typename N>
struct checkered {
  bool active;
  N next;

  uint8_t operator()(uint8_t src, uint8_t* dst) {
    active = !active;
    if (active) {
      uint8_t dummy;
      next(src, &dummy);
      return false;
    } else {
      return next(src, dst); 
    }
  }

  void next_row() {
    next.next_row();
    active = !active;
  }
};

template <typename N>
checkered<N> make_checkered(bool active, N&& n) { return {active, n}; }

template <typename F>
static void copy_pixels(BYTE*& dst, int width, bool some_flag, F&& f) {
  f.next_row();
  int offset = some_flag ? 0 : (32 - width);
  uint8_t dummy;
  if (gpBufStart <= dst && dst < gpBufEnd) {
    for (int i = 0; i < width; ++i) f(dst + offset + i);
  } else {
    for (int i = 0; i < width; ++i) f(&dummy);
  }
  dst -= 768;
}

template <typename F>
void draw_lower_screen_2_11(BYTE* dst, const BYTE* src, bool some_flag, F&& f) {
  triangular_texture<F> t{f, src, some_flag};
  for (int i = 0; i < 16; ++i) {
    copy_pixels(dst, (i + 1) * 2, some_flag, t);
  }
  for (int i = 30; i > 0; i -= 2) {
    copy_pixels(dst, i, some_flag, t);
  }
}

template <typename F>
void draw_lower_screen_default(BYTE* dst, const BYTE* src, bool some_flag, F&& f) {
  trapezoidal_texture<F> t{f, src, some_flag};
  for (int i = 0; i < 16; ++i) {
    copy_pixels(dst, 2 * (i+1), some_flag, t);
  }
  for (int i = 0; i < 16; ++i) {
    copy_pixels(dst, 32, some_flag, t);
  }
}

template <typename F>
void draw_lower_screen_9(BYTE* dst, const BYTE* src, F&& f) {
  skip_texture<F> tex{f, src};
  for (int y = 0; y < 32; ++y) {
    copy_pixels(dst, 32, true, tex);
  }
}

template <typename F>
static void dispatch_draw_for_cel_type(BYTE* dst, F&& f) {
	const uint8_t* src = (unsigned char *)pDungeonCels + *((DWORD *)pDungeonCels + (level_cel_block & 0xFFF));
  int cel_type_16 = level_cel_block >> 12;
	switch (cel_type_16 & 7) {
    case 0: {
      square_texture<F> t{f, src};
      for (int i = 0; i < 32; ++i) {
        copy_pixels(dst, 32, false, t);
      }
      break;
    }
    case 1:
      draw_lower_screen_9(dst, src, f);
      break;
    case 2:
      draw_lower_screen_2_11(dst, src, false, f);
      break;
    case 3:
      draw_lower_screen_2_11(dst, src, true, f);
      break;
    case 4:
      draw_lower_screen_default(dst, src, false, f);
      break;
    default:
      draw_lower_screen_default(dst, src, true, f);
      break;
	}
}

void drawUpperScreen(BYTE *pBuff)
{
  drawLowerScreen(pBuff);
}

void drawTopArchesLowerScreen(BYTE *pBuff)
{
	unsigned char *tbl;

  if (light_table_index == lightmax) {
    dispatch_draw_for_cel_type(pBuff, make_checkered(true, black{}));
    return;
  }
	if (light_table_index) {
    tbl = &pLightTbl[256 * light_table_index];
    dispatch_draw_for_cel_type(pBuff, make_checkered(true, lit{tbl}));
    return;
	}
  dispatch_draw_for_cel_type(pBuff, make_checkered(true, identity{}));
}

void drawBottomArchesLowerScreen(BYTE *pBuff, unsigned int *pMask)
{
	unsigned char *tbl;

  if (light_table_index == lightmax) {
    dispatch_draw_for_cel_type(pBuff, make_masked(pMask, black{}));
    return;
  }
	if (light_table_index) {
    tbl = &pLightTbl[256 * light_table_index];
    dispatch_draw_for_cel_type(pBuff, make_masked(pMask, lit{tbl}));
    return;
	}
  dispatch_draw_for_cel_type(pBuff, make_masked(pMask, identity{}));
}

void drawLowerScreen(BYTE *pBuff)
{
	unsigned char *src;        // esi MAPDST
	unsigned char *tbl;        // ebx

	if (cel_transparency_active) {
		if (!arch_draw_type) {
			drawTopArchesLowerScreen(pBuff);
			return;
		}
		if (arch_draw_type == 1) {
			if (block_lvid[level_piece_id] == 1 || block_lvid[level_piece_id] == 3) {
				drawBottomArchesLowerScreen(pBuff, &LeftMask[31]);
				return;
			}
		}
		if (arch_draw_type == 2) {
			if (block_lvid[level_piece_id] == 2 || block_lvid[level_piece_id] == 3) {
				drawBottomArchesLowerScreen(pBuff, &RightMask[31]);
				return;
			}
		}
	}
  if (light_table_index == lightmax) {
    dispatch_draw_for_cel_type(pBuff, black{});
    return;
  }
	if (light_table_index) {
    tbl = &pLightTbl[256 * light_table_index];
    dispatch_draw_for_cel_type(pBuff, lit{tbl});
    return;
	}
  dispatch_draw_for_cel_type(pBuff, identity{});
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

DEVILUTION_END_NAMESPACE
