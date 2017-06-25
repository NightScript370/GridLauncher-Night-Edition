#include "themegfx.h"
#include "pngloader.h"
#include "filesystem.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logText.h"
#include "MAGFX.h"

#include "appbackgroundalphamask_bin.h"
#include "cartbackgroundalphamask_bin.h"

int themeImageAppBackground = 0;
int themeImageAppBackgroundSelected = 1;

int themeImageCartBackground = 2;
int themeImageCartBackgroundSelected = 3;

int themeImageTopWallpaper = 4;
int themeImageBottomWallpaper = 5;

int themeImageTopLeftButton = 6;
int themeImageTopRightButton = 7;
int themeImageBottomLeftButton = 8;
int themeImageBottomRightButton = 9;

int themeImageTopLeftButtonSelected = 10;
int themeImageTopRightButtonSelected = 11;
int themeImageBottomLeftButtonSelected = 12;
int themeImageBottomRightButtonSelected = 13;

int themeImageHelpSymbol = 14;
int themeImageBackSymbol = 15;
int themeImageHomeSymbol = 16;
int themeImageSettingsSymbol = 17;
int themeImageFoldersSymbol = 18;

int themeImageTopWallpaperInfo = 19;

int themeImageBottomWallpaperNonGrid = 20;

int themeImageSplashTop = 21;
int themeImageSplashBottom = 22;

int themeImageTopWallpaperReboot = 23;
int themeImageBottomWallpaperReboot = 24;

int themeImageAppOverlay = 25;
int themeImageAppOverlaySelected = 26;

int themeImageCartOverlay = 27;
int themeImageCartOverlaySelected = 28;

bool themeHasProgressWheel = false;

typedef struct themeImage {
	u8 * spriteData;
	bool exists;
	bool hasAlpha;
	int w, h;
} themeImage;

#define maxThemeImages 29
themeImage themeImages[maxThemeImages];

#define maxProgressWheelImages 16
themeImage progressWheelImages[maxProgressWheelImages];

int numProgressWheelImages = 0;

void freeThemeImagesArray(themeImage images[], int numImages) {
	int i;
	for (i=0; i<numImages; i++) {
		themeImage * aThemeImage = &(images[i]);
		if (aThemeImage->spriteData) {
			free(aThemeImage->spriteData);
			aThemeImage->spriteData = NULL;
		}
	}
}

void freeThemeImages() {
	freeThemeImagesArray(themeImages, maxThemeImages);
	freeThemeImagesArray(progressWheelImages, maxProgressWheelImages);
}

bool loadThemeImage(char * themePath, char * filename, char * description, int expectedWidth, int expectedHeight, u8 * alphaMask, int imageID, themeImage images[]) {
	char path[128];
	sprintf(path, "%s%s", themePath, filename);

	themeImage * aThemeImage = &(images[imageID]);
	aThemeImage->exists = false;

	if (aThemeImage->spriteData) {
		free(aThemeImage->spriteData);
	}

	aThemeImage->spriteData = NULL;

	if (!fileExists(path, &sdmcArchive)) {
		return false;
	}

	bool success = read_png_file(path);

	if (success) {
		if (pngWidth != expectedWidth || pngHeight != expectedHeight) {
			char error[256];
			sprintf(error, "%s must be %dx%d pixels", description, expectedWidth, expectedHeight);
			logText(error);
			return false;
		} else {
			u8 * out = process_png_file();

			if (out) {
				if (alphaMask) {
					u8 * masked = malloc(expectedWidth*expectedHeight*4);
					MAGFXApplyAlphaMask(out, alphaMask, masked, expectedWidth, expectedHeight, (bytesPerPixel==4));
					aThemeImage->spriteData = masked;
					free(out);
					aThemeImage->hasAlpha = true;
				} else {
					aThemeImage->spriteData = out;
					aThemeImage->hasAlpha = (bytesPerPixel==4);
				}


				aThemeImage->exists = true;
				aThemeImage->w = expectedWidth;
				aThemeImage->h = expectedHeight;

				return true;
			} else {
				return false;
			}
		}
	} else {
		return false;
	}
}

void initThemeImages() {
	char * themePath = currentThemePath();


	loadThemeImage(themePath, "appbackground.png",  "App background", 56, 56, (u8*)appbackgroundalphamask_bin, themeImageAppBackground, themeImages);
	loadThemeImage(themePath, "appbackgroundselected.png", "Selected app background", 56, 56, (u8*)appbackgroundalphamask_bin, themeImageAppBackgroundSelected, themeImages);

	loadThemeImage(themePath, "cartbackground.png", "Cart background", 59, 59, (u8*)cartbackgroundalphamask_bin, themeImageCartBackground, themeImages);
	loadThemeImage(themePath, "cartbackgroundselected.png", "Selected cart background", 59, 59, (u8*)cartbackgroundalphamask_bin, themeImageCartBackgroundSelected, themeImages);

	loadThemeImage(themePath, "appoverlay.png", "App overlay", 64, 64, NULL, themeImageAppOverlay, themeImages);
	loadThemeImage(themePath, "appoverlayselected.png", "Selected app overlay", 64, 64, NULL, themeImageAppOverlaySelected, themeImages);

	loadThemeImage(themePath, "cartoverlay.png", "Cart overlay", 64, 64, NULL, themeImageCartOverlay, themeImages);
	loadThemeImage(themePath, "cartoverlayselected.png", "Selected cart overlay", 64, 64, NULL, themeImageCartOverlaySelected, themeImages);

	loadThemeImage(themePath, "wallpapertop.png", "Top wallpaper", 400, 240, NULL, themeImageTopWallpaper, themeImages);
	loadThemeImage(themePath, "wallpapertopinfo.png", "Top info wallpaper", 400, 240, NULL, themeImageTopWallpaperInfo, themeImages);

	loadThemeImage(themePath, "wallpaperbottom.png", "Bottom wallpaper", 320, 240, NULL, themeImageBottomWallpaper, themeImages);
	loadThemeImage(themePath, "wallpaperbottomnongrid.png", "Bottom wallpaper (non grid)", 320, 240, NULL, themeImageBottomWallpaperNonGrid, themeImages);

	loadThemeImage(themePath, "wallpaperbottomreboot.png", "Bottom wallpaper (reboot)", 320, 240, NULL, themeImageBottomWallpaperReboot, themeImages);
	loadThemeImage(themePath, "wallpapertopreboot.png", "Top wallpaper (reboot)", 400, 240, NULL, themeImageTopWallpaperReboot, themeImages);

	loadThemeImage(themePath, "buttontopleft.png", "Top left button", 36, 36, NULL, themeImageTopLeftButton, themeImages);
	loadThemeImage(themePath, "buttontopright.png", "Top right button", 36, 36, NULL, themeImageTopRightButton, themeImages);
	loadThemeImage(themePath, "buttonbottomleft.png", "Bottom left button", 36, 36, NULL, themeImageBottomLeftButton, themeImages);
	loadThemeImage(themePath, "buttonbottomright.png", "Bottom right button", 36, 36, NULL, themeImageBottomRightButton, themeImages);

	loadThemeImage(themePath, "buttontopleftselected.png", "Top left selected button", 36, 36, NULL, themeImageTopLeftButtonSelected, themeImages);
	loadThemeImage(themePath, "buttontoprightselected.png", "Top right selected button", 36, 36, NULL, themeImageTopRightButtonSelected, themeImages);
	loadThemeImage(themePath, "buttonbottomleftselected.png", "Bottom left selected button", 36, 36, NULL, themeImageBottomLeftButtonSelected, themeImages);
	loadThemeImage(themePath, "buttonbottomrightselected.png", "Bottom right selected button", 36, 36, NULL, themeImageBottomRightButtonSelected, themeImages);

	loadThemeImage(themePath, "helpicon.png", "Help icon", 36, 36, NULL, themeImageHelpSymbol, themeImages);
	loadThemeImage(themePath, "backicon.png", "Back icon", 36, 36, NULL, themeImageBackSymbol, themeImages);
	loadThemeImage(themePath, "homeicon.png", "Home icon", 36, 36, NULL, themeImageHomeSymbol, themeImages);
	loadThemeImage(themePath, "settingsicon.png", "Settings icon", 36, 36, NULL, themeImageSettingsSymbol, themeImages);
	loadThemeImage(themePath, "foldersicon.png", "Folders icon", 36, 36, NULL, themeImageFoldersSymbol, themeImages);

	numProgressWheelImages = 0;

	int frame;
	for (frame = 0; frame < maxProgressWheelImages; frame++) {
		char progressWheelFrameFilename[128];
		sprintf(progressWheelFrameFilename, "progressWheelFrame%d.png",frame+1);

		bool loadSuccess = loadThemeImage(themePath, progressWheelFrameFilename, "Progress wheel frame", 36, 36, NULL, frame, progressWheelImages);
		if (loadSuccess) {
			numProgressWheelImages++;
		} else {
			break;
		}
	}

	themeHasProgressWheel = (numProgressWheelImages > 0);

	free(themePath);
}

void loadSplashImages() {
	char * themePath = currentThemePath();

	if(!loadThemeImage(themePath, "splashtop.png", "Top splash screen", 400, 240, NULL, themeImageSplashTop, themeImages)) {
		loadThemeImage(defaultThemePath, "splashtop.png", "Top splash screen", 400, 240, NULL, themeImageSplashTop, themeImages);
	}

	if(!loadThemeImage(themePath, "splashbottom.png", "Bottom splash screen", 320, 240, NULL, themeImageSplashBottom, themeImages)) {
		loadThemeImage(defaultThemePath, "splashbottom.png", "Bottom splash screen", 320, 240, NULL, themeImageSplashBottom, themeImages);
	}

	free(themePath);
}

void drawThemeImageCheckAlpha(u8 * image, gfxScreen_t screen, int x, int y, int w, int h, bool hasAlpha) {
	if (hasAlpha) {
		gfxDrawSpriteAlphaBlend(screen, GFX_LEFT, image, w, h, x, y);
	} else {
		gfxDrawSprite(screen, GFX_LEFT, image, w, h, x, y);
	}
}

void drawThemeImage(int imageID, gfxScreen_t screen, int x, int y) {
	themeImage aThemeImage = themeImages[imageID];
	if (aThemeImage.exists) {
		drawThemeImageCheckAlpha(aThemeImage.spriteData, screen, x, y, aThemeImage.h, aThemeImage.w, aThemeImage.hasAlpha);
	}
}

void drawProgressWheelImage(int frame, gfxScreen_t screen, int x, int y) {
	themeImage wheelFrameImage = progressWheelImages[frame];
	if (wheelFrameImage.exists) {
		drawThemeImageCheckAlpha(wheelFrameImage.spriteData, screen, x, y, wheelFrameImage.h, wheelFrameImage.w, wheelFrameImage.hasAlpha);
	}
}

bool themeImageExists(int imageID) {
	themeImage aThemeImage = themeImages[imageID];
	return aThemeImage.exists;
}
