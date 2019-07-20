//HEADER_GOES_HERE
#ifndef __RENDER_H__
#define __RENDER_H__

void drawUpperScreen(BYTE *pBuff);
void drawTopArchesLowerScreen(BYTE *pBuff);
void drawBottomArchesLowerScreen(BYTE *pBuff, unsigned int *pMask);
void drawLowerScreen(BYTE *pBuff);

void world_draw_black_tile(BYTE *pBuff);

/* rdata */

extern BYTE *gpBufStart;

#endif /* __RENDER_H__ */
