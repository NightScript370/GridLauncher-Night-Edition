#pragma once
#include <3ds.h>

extern int panelAlphaTop;
extern int panelAlphaBottom;

extern int panelRTop;
extern int panelGTop;
extern int panelBTop;

extern int panelRBottom;
extern int panelGBottom;
extern int panelBBottom;

extern int panelLeftOffsetTop;
extern bool panelsDrawn;


void MAGFXImageWithRGBAndAlphaMask(u8 r, u8 g, u8 b, u8* alphaSourceMask, u8* dest, int width, int height);
void MAGFXApplyAlphaMask(u8* imageSource, u8* alphaSourceMask, u8* dest, int width, int height, bool sourceHasAlpha);
void MAGFXTranslucentRect(int width, int height, int r, int g, int b, int alpha, u8* dest);
void MAGFXDrawPanel(gfxScreen_t screen, bool forceZeroLeftOffset);
void drawDisk(char * text);
