#pragma once

#include "gfx.h"
#include "scanner.h"
#include "descriptor.h"
#include "help.h"

#ifndef MENU_H
#define MENU_H

extern int menuStatusIcons;
extern int menuStatusSettings;
extern int menuStatusHelp;
extern int menuStatusFolders;
extern int menuStatusFolderChanged;
extern int menuStatusTitleBrowser;
extern int menuStatusOpenHomeMenuApps;
extern int menuStatusHomeMenuApps;
extern int menuStatusColourSettings;
extern int menuStatusColourAdjust;
extern int menuStatusTranslucencyTop;
extern int menuStatusTranslucencyBottom;

extern int menuStatusThemeSelect;
extern int menuStatusWaterSettings;
extern int menuStatusThemeSettings;
extern int menuStatusGridSettings;
extern int menuStatusOpenTitleFiltering;
extern int menuStatusTitleFiltering;
extern int menuStatusSoftwareUpdate;
extern int menuStatusHansMissingError;

extern int menuStatusPanelSettingsTop;
extern int menuStatusPanelSettingsBottom;

extern int menuStatusBootOptions;

extern int menuStatusHBAppOptions;
extern int menuStatusHBAppDeleteConfirmation;

//Checked by main.c so that the menu can force the title browser to close
extern bool killTitleBrowser;

extern int menuStatus;

extern int totalRows;

extern bool alphaImagesDrawn;
extern bool pageControlPanelsDrawn;
extern bool menuReloadRequired;
extern bool dPadNavigation;
extern bool animatedGrids;

extern bool trueBool;

extern int translucencyTopBar;
extern int translucencyBottomBar;
extern int translucencyBarIcons;
extern int translucencyBarIconSymbols;
extern int translucencyAppBackgrounds;
extern int translucencyPageControls;
extern int translucencyWater;
extern int translucencyAppShadow;

extern bool toolbarNeedsUpdate;

extern bool randomiseThemeOnWake;

#endif

#define ENTRY_PATHLENGTH (128)
#define ENTRY_NAMELENGTH (64)
#define ENTRY_DESCLENGTH (128)
#define ENTRY_AUTHORLENGTH (64)
#define ENTRY_ARGLENGTH (64)
#define ENTRY_ICON_WIDTH (48)
#define ENTRY_ICON_HEIGHT (48)
#define ENTRY_ICONSIZE (ENTRY_ICON_WIDTH*ENTRY_ICON_HEIGHT*4)
#define ENTRY_WIDTH (66)
#define ENTRY_WIDTH_SELECTED (72)
#define ENTRY_FWIDTH (63)
#define ENTRY_FWIDTH_SELECTED (63)
#define ENTRY_HEIGHT (294)
#define ENTRY_HEIGHT_SELECTED (298)

#define ENTRY_BGCOLOR (u8[]){246, 252, 255}
#define ENTRY_BGCOLOR_SHADOW (u8[]){34, 153, 183}

#define transitionDirectionLeft 10
#define transitionDirectionRight 20
#define transitionDirectionUp 30
#define transitionDirectionDown 40

typedef struct menuEntry_s
{
	char executablePath[ENTRY_PATHLENGTH+1];
	char bannerImagePath[ENTRY_PATHLENGTH+1];
	char name[ENTRY_NAMELENGTH+1];
	char description[ENTRY_DESCLENGTH+1];
	char author[ENTRY_AUTHORLENGTH+1];
	char arg[ENTRY_ARGLENGTH+1];
	u8 iconData[ENTRY_ICONSIZE];
	descriptor_s descriptor;
	struct menuEntry_s* next;
	int page;
	int row;
	int col;
	int iconX;
	int iconY;
	int iconW;
	int iconH;
	bool hidden;
	bool isRegionFreeEntry;
	bool isTitleEntry;
	u64 title_id;
	u8 mediatype;
	bool * showTick;
	void (*callback)();
	void *callbackObject1;
	void *callbackObject2;
	bool drawFirstLetterOfName;
	bool drawFullTitle;
	bool isShortcut;
	bool bannerIsFullScreen;
	bool hasBanner;
	bool isWithinContainingFolder;
}menuEntry_s;

typedef struct
{
	menuEntry_s* entries;
	u16 numEntries;
	u16 selectedEntry;
	s32 scrollTarget; //10 bit fixed point
	s32 scrollLocation; //10 bit fixed point
	s32 scrollVelocity; //10 bit fixed point
	s32 scrollBarSize;
	s32 currentScrollBarSize;
	s32 scrollBarPos;
	touchPosition previousTouch, firstTouch;
	bool atEquilibrium;
	int totalPages, rowPosition, colPosition, pagePosition; //totalRows
}menu_s;

extern menuEntry_s regionfreeEntry;
extern menu_s menu;
extern menu_s * bootOptionsMenu;

extern bool fullScreenBannerVisible;

//menu stuff
void initMenu(menu_s* m);
void drawMenu(menu_s* m);
void drawGrid(menu_s* m);
bool updateMenu(menu_s* m);
bool updateGrid(menu_s* m);
void gotoFirstIcon(menu_s* m);
void updateMenuIconPositions(menu_s* m);
void addMenuEntry(menu_s* m, menuEntry_s* me);
void addMenuEntryCopy(menu_s* m, menuEntry_s* me);
void clearMenuEntries(menu_s* m);
void createMenuEntry(menu_s* m, char* execPath, char* name, char* description, char* author, u8* iconData);
menuEntry_s* getMenuEntry(menu_s* m, u16 n);
void drawBottomStatusBar(char* title);

//menu entry stuff
void initEmptyMenuEntry(menuEntry_s* me);
void initMenuEntry(menuEntry_s* me, char* execPath, char* name, char* description, char* author, u8* iconData);
int drawMenuEntry(menuEntry_s* me, gfxScreen_t screen, bool selected, menu_s *m, int pageYOffset, int pageXOffset, bool drawTopScreenInfo);

void scanMenuEntry(menuEntry_s* me);

void setMenuStatus(int status);
void menuRegionFreeToggled();
void toggleTranslucency();
void loadThemeConfig();

void handleNonGridToolbarNavigation();

void checkReturnToGrid(menu_s* m);

menuEntry_s * updateMenuTicks(menu_s* m, char * selectedString, bool useExeecutablePath);
void startTransition(int direction, int fromPage, menu_s* fromMenu);
void reloadMenu(menu_s* m);

