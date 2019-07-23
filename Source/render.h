//HEADER_GOES_HERE
#ifndef __RENDER_H__
#define __RENDER_H__

void drawUpperScreen(BYTE *pBuff);
void drawLowerScreen(BYTE *pBuff);

void world_draw_black_tile(BYTE *pBuff);

void CelDecDatLightOnly(uint8_t *dst, const uint8_t *pRLEBytes, int nDataSize, int texWidth, const uint8_t* tbl);
void CelDecDatLightTrans(uint8_t *dst, const uint8_t *pRLEBytes, int nDataSize, int texWidth, const uint8_t* tbl);
void CopyRect(uint8_t *dst, const uint8_t* src, int source_width, int copy_width, int num_rows);

/* rdata */

extern BYTE *gpBufStart;

#endif /* __RENDER_H__ */
