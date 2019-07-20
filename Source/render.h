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
extern int WorldBoolFlag;
extern unsigned int gdwCurrentMask;
extern unsigned int *gpDrawMask;
extern unsigned int RightMask[32];
extern unsigned int LeftMask[32];
extern unsigned int WallMask[32];
extern int WorldTbl3x16[48];
extern int WorldTbl17_1[17];
extern int WorldTbl17_2[17];

#endif /* __RENDER_H__ */
