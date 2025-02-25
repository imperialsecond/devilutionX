//HEADER_GOES_HERE
#ifndef __SCROLLRT_H__
#define __SCROLLRT_H__

extern int light_table_index;
extern int PitchTbl[1024];
extern BYTE *gpBufEnd;
extern DWORD level_cel_block;
extern char arch_draw_type;
extern DDSURFACEDESC DDS_desc;
extern int cel_transparency_active; // weak
extern int level_piece_id;
extern int draw_monster_num;

void ClearCursor();
void ClearScreenBuffer();
void DoBlitScreen(DWORD dwX, DWORD dwY, DWORD dwWdt, DWORD dwHgt);
void DrawAndBlit();
void DrawClippedMissile(int x, int y, int sx, int sy, BOOL pre);
void DrawDeadPlayer(int x, int y, int sx, int sy);
void DrawGame(int x, int y);
void DrawMain(int dwHgt, BOOL draw_desc, BOOL draw_hp, BOOL draw_mana, BOOL draw_sbar, BOOL draw_btn);
void DrawView(int StartX, int StartY);
void scrollrt_draw_clipped_dungeon(BYTE *pBuff, int sx, int sy, int dx, int dy, int eflag);
void scrollrt_draw_clipped_e_flag(BYTE *pBuff, int x, int y, int sx, int sy);
void scrollrt_draw_cursor_back_buffer();
void scrollrt_draw_cursor_item();
void scrollrt_draw_game_screen(BOOL draw_cursor);
void scrollrt_draw_lower(int x, int y, int sx, int sy, int chunks, int eflag);

#ifdef _DEBUG
void ScrollView();
void EnableFrameCount();
void DrawFPS();
#endif

/* used in 1.00 debug */
extern char *szMonModeAssert[18];
extern char *szPlrModeAssert[12];

#endif /* __SCROLLRT_H__ */
