#include "gtest/gtest.h"

#include "devilution.h"

using Buffer = uint8_t[600][768];

void BufferInit(Buffer* buffer) {
  memset(*buffer, 255, sizeof(Buffer)); 
}

TEST(RenderTest, TestWorldDrawBlackTile) {
  Buffer buffer;
  BufferInit(&buffer);
  dvl::world_draw_black_tile(&buffer[30][0]);

  for (int y = 0; y < 31; ++y) {
    int dy = abs(15 - y);
    int startx = dy * 2;
    int endx = 64 - startx;
    for (int x = 0; x < 64; ++x) {
      if (x >= startx && x < endx) {
        EXPECT_EQ(buffer[y][x], 0) << x << "/" << y << " should be black.";
      } else {
        EXPECT_EQ(buffer[y][x], 255) << x << "/" << y << " should be white.";
      }
    }
  }
}

TEST(RenderTest, TestWorldCopySquare) {
  Buffer buffer;
  BufferInit(&buffer);

  dvl::gpBufEnd = &buffer[600][0];

  uint8_t source[32][32];
  for (int x = 0; x < 32; ++x) {
    for (int y = 0; y < 32; ++y) {
      source[y][x] = x+y;
    }
  }
  dvl::world_copy_block(&buffer[31][0], &source[0][0], 32);

  for (int y = 0; y < 64; ++y) {
    for (int x = 0; x < 64; ++x) {
      if (x < 32 && y < 32) {
        // src is upside down relative to dst.
        EXPECT_EQ(buffer[y][x], x+(31-y)) << x << "/" << y;
      } else {
        EXPECT_EQ(buffer[y][x], 255) << x << "/" << y;
      }
    }
  }
}

namespace dvl {
#include "_asm.cpp"
}

class AsmTest : public testing::Test {
 public:
  AsmTest() {
    for (int i = 0; i < 256; ++i) {
      tbl[i] = 255 - i;
    }
  }
 
  void test(std::function<void(int)> fn, int stride = 1) {
    for (int w = 0; w < 64 / stride; ++w) {
      init();
      fn(w);
      done(w * stride);
    }
  }

  void init() {
    src_p = src;
    dst_p = dst;
    for (int i = 0; i < 64; ++i) {
      src[i] = i;
      dst[i] = i + 128;
    }
  }

  void done(int w) {
    EXPECT_EQ(std::distance(src, src_p), w);
    EXPECT_EQ(std::distance(dst, dst_p), w);
  }

  uint8_t tbl[256];
  uint8_t src[64];
  uint8_t dst[64];

  uint8_t* src_p;
  uint8_t* dst_p;
};

TEST_F(AsmTest, Test_asm_cel_light_edge) {
  test([this](int w) {
    dvl::asm_cel_light_edge(w, tbl, &dst_p, &src_p);
    for (int j = 0; j < w; ++j) {
      EXPECT_EQ(dst[j], 255 - j);
    }
  });
}

TEST_F(AsmTest, Test_asm_cel_light_square) {
  test([this](int w) {
    dvl::asm_cel_light_square(w, tbl, &dst_p, &src_p);
    for (int j = 0; j < w * 4; ++j) {
      EXPECT_EQ(dst[j], 255 - j);
    }
  }, 4);
}

TEST_F(AsmTest, Test_asm_trans_light_cel_0_2) {
  test([this](int w) {
    dvl::asm_trans_light_cel_0_2(w, tbl, &dst_p, &src_p);
    for (int j = 0; j < w; ++j) {
      if (j & 1) EXPECT_EQ(dst[j], 255 - j);
      else EXPECT_EQ(dst[j], 128 + j);
    }
  });
}

TEST_F(AsmTest, Test_asm_trans_light_cel_1_3) {
  test([this](int w) {
    dvl::asm_trans_light_cel_1_3(w, tbl, &dst_p, &src_p);
    for (int j = 0; j < w; ++j) {
      if (!(j & 1)) EXPECT_EQ(dst[j], 255 - j);
      else EXPECT_EQ(dst[j], 128 + j);
    }
  });
}

TEST_F(AsmTest, Test_asm_trans_light_square_0_2) {
  test([this](int w) {
    dvl::asm_trans_light_square_0_2(w, tbl, &dst_p, &src_p);
    for (int j = 0; j < w * 4; ++j) {
      if (!(j & 1)) EXPECT_EQ(dst[j], 255 - j);
      else EXPECT_EQ(dst[j], 128 + j);
    }
  }, 4);
}

TEST_F(AsmTest, Test_asm_trans_light_square_1_3) {
  test([this](int w) {
    dvl::asm_trans_light_square_1_3(w, tbl, &dst_p, &src_p);
    for (int j = 0; j < w * 4; ++j) {
      if ((j & 1)) EXPECT_EQ(dst[j], 255 - j);
      else EXPECT_EQ(dst[j], 128 + j);
    }
  }, 4);
}
