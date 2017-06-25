#include "MAGFX.h"
#include "string.h"
#include "gfx.h"
#include "colours.h"
#include "diskalphamask_bin.h"
#include "MAFontRobotoRegular.h"
#include "config.h"

int panelAlphaTop;
int panelAlphaBottom;

int panelRTop;
int panelGTop;
int panelBTop;

int panelRBottom;
int panelGBottom;
int panelBBottom;

bool panelsDrawn = false;

void MAGFXImageWithRGBAndAlphaMask(u8 r, u8 g, u8 b, u8* alphaSourceMask, u8* dest, int width, int height) {
	int alphaSourceByte = 0;
	int destByte = 0;
	int len = width*height;

	while (alphaSourceByte < len) {
		memcpy(&dest[destByte], &b, 1); destByte++;
		memcpy(&dest[destByte], &g, 1); destByte++;
		memcpy(&dest[destByte], &r, 1); destByte++;
		memcpy(&dest[destByte], &alphaSourceMask[alphaSourceByte], 1); destByte++;

		alphaSourceByte ++;
	}
}

void MAGFXApplyAlphaMask(u8* imageSource, u8* alphaSourceMask, u8* dest, int width, int height, bool sourceHasAlpha) {
	int GRBSourceByte = 0;
	int alphaSourceByte = 0;
	int destByte = 0;
	int bytesPerPixel = sourceHasAlpha ? 4 : 3;
	int len = width*height*bytesPerPixel;

	while (GRBSourceByte < len) {
		memcpy(&dest[destByte], &imageSource[GRBSourceByte], 3);
		destByte += 3;
		GRBSourceByte += bytesPerPixel;

		memcpy(&dest[destByte], &alphaSourceMask[alphaSourceByte], 1);
		destByte += 1;
		alphaSourceByte += 1;
	}
}

void MAGFXTranslucentRect(int width, int height, int r, int g, int b, int a, u8* dest) {
	int len = width * height * 4;
	int byte = 0;

	while (byte < len) {
		memcpy(&dest[byte], &b, 1); byte++;
		memcpy(&dest[byte], &g, 1); byte++;
		memcpy(&dest[byte], &r, 1); byte++;
		memcpy(&dest[byte], &a, 1); byte++;
	}
}

int panelEdgeOffset = 20;
int panelLeftOffsetTop = 0;
int panelHeightTop = 360;
int panelHeightBottom = 280;
int panelWidthTop = 182;
int panelWidthBottom = 200;
u8 panelPixel[4]; // 360 * 182 * 4

void MAGFXDrawPanel(gfxScreen_t screen, bool forceZeroLeftOffset) {
	if (!panelsDrawn) {
		int panelAlpha = (screen == GFX_TOP) ? panelAlphaTop : panelAlphaBottom;

		int panelR = (screen == GFX_TOP) ? panelRTop : panelRBottom;
		int panelG = (screen == GFX_TOP) ? panelGTop : panelGBottom;
		int panelB = (screen == GFX_TOP) ? panelBTop : panelBBottom;

		panelPixel[0] = panelB;
		panelPixel[1] = panelG;
		panelPixel[2] = panelR;
		panelPixel[3] = panelAlpha;
	}

	if (screen == GFX_TOP && panelAlphaTop == 0) {
		return;
	}

	if (screen == GFX_BOTTOM && panelAlphaBottom == 0) {
		return;
	}

	int drawWidth;
	int drawHeight;
	int leftOffset = 0;

	if (screen == GFX_TOP) {
		drawWidth = panelWidthTop;
		drawHeight = panelHeightTop;

		if (!forceZeroLeftOffset) {
			leftOffset = panelLeftOffsetTop;
		}
	} else {
		drawWidth = panelWidthBottom;
		drawHeight = panelHeightBottom;
	}

	drawHeight -= leftOffset;

	int x, y;

	int totalWidth = drawWidth + panelEdgeOffset;
	int totalHeight = drawHeight + panelEdgeOffset;

	for (y = panelEdgeOffset; y < totalHeight; y++) {
		for (x = panelEdgeOffset; x < totalWidth; x++) {
			gfxDrawSpriteAlphaBlend(screen, GFX_LEFT, panelPixel, 1, 1, x, y+leftOffset);
		}
	}
}

u8 disk[64*64*4];
bool diskDrawn = false;

void drawDisk(char * text) {
	u8 dimmer[4];
	dimmer[0] = 128;
	dimmer[1] = 128;
	dimmer[2] = 128;
	dimmer[3] = 200;

	int x, y;

	int totalWidth = 240;
	int totalHeight = 320;

	for (y = 0; y < totalHeight; y++) {
		for (x = 0; x < totalWidth; x++) {
			gfxDrawSpriteAlphaBlend(GFX_BOTTOM, GFX_LEFT, dimmer, 1, 1, x, y);
		}
	}

	int diskX = (240-64)/2;

	if (!diskDrawn) {
		diskDrawn = true;
		rgbColour * tint = tintColour();
		MAGFXImageWithRGBAndAlphaMask(tint->r, tint->g, tint->b, (u8*)diskalphamask_bin, disk, 64, 64);
	}

	gfxDrawSpriteAlphaBlend(GFX_BOTTOM, GFX_LEFT, disk, 64, 64, diskX, (320-64)/2);

	int len = MATextWidthInPixels(text, &MAFontRobotoRegular16);
	rgbColour * dark = darkTextColour();
	MADrawText(GFX_BOTTOM, GFX_LEFT, diskX - 32, (320/2)-(len/2), text, &MAFontRobotoRegular16, dark->r, dark->g, dark->b);
}
