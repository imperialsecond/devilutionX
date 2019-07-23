#ifndef __TEXTURE_ATLAS_H__
#define __TEXTURE_ATLAS_H__

struct Texture {
  const uint8_t* data;
  int byteSize;
  int width;
};

class TextureAtlas {
 public:
  TextureAtlas() : data(nullptr) {}
  // Does not take ownership of data.
  TextureAtlas(const uint8_t* data) : data(data) {}

  Texture get(int index, int width) {
    const uint32_t* offsetTable = (const uint32_t*)data;
    return {
      &data[offsetTable[index]], 
	    offsetTable[index + 1] - offsetTable[index],
      width
    };
  }
 private:
  const uint8_t* data;
};

#endif /* __TEXTURE_ATLAS_H__ */
