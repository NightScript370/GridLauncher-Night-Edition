#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include <math.h>

#include "filesystem.h"
#include "menu.h"
#include "smdh.h"
#include "regionfree.h"
#include "regionfree_bin.h"
#include "folders.h"
#include "water.h"
#include "titles.h"
#include "boot.h"

#include "appshadow_bin.h"
#include "appiconalphamask_bin.h"
#include "appbackgroundalphamask_bin.h"
#include "cartbackgroundalphamask_bin.h"
#include "pageiconalphamask_bin.h"

#include "pageControlPanelLeftAlphaMask_bin.h"
#include "pageControlPanelRightAlphaMask_bin.h"

#include "config.h"
#include "colours.h"
#include "button.h"

#include "MAFontRobotoRegular.h"

#include "statusbar.h"
#include "alert.h"
#include "MAGFX.h"

#include "tick_bin.h"

#include "touchblock.h"

#include "themegfx.h"
#include "sound.h"

#define buttonTagTopLeft 10
#define buttonTagTopRight 20
#define buttonTagBottomLeft 30
#define buttonTagBottomRight 40

bool randomiseThemeOnWake = false;

bool fullScreenBannerVisible = false;

int menuStatusIcons = 0;
int menuStatusSettings = 1;
int menuStatusHelp = 2;
int menuStatusFolders = 3;
int menuStatusFolderChanged = 4;
int menuStatusTitleBrowser = 6;
int menuStatusOpenHomeMenuApps = 7;
int menuStatusHomeMenuApps = 8;
int menuStatusColourSettings = 9;
int menuStatusColourAdjust = 10;
int menuStatusTranslucencyTop = 11;
int menuStatusTranslucencyBottom = 12;

int menuStatusThemeSelect = 14;
int menuStatusWaterSettings = 15;
int menuStatusThemeSettings = 16;
int menuStatusGridSettings = 17;
int menuStatusOpenTitleFiltering = 18;
int menuStatusTitleFiltering = 19;
int menuStatusSoftwareUpdate = 20;
int menuStatusHansMissingError = 21;

int menuStatusPanelSettingsTop = 13;
int menuStatusPanelSettingsBottom = 22;

int menuStatusBootOptions = 23;
int menuStatusHBAppOptions = 24;
int menuStatusHBAppDeleteConfirmation = 25;

bool killTitleBrowser = false;
bool dPadNavigation = true;
bool animatedGrids = true;

int translucencyTopBar = 255;
int translucencyBottomBar = 255;
int translucencyBarIcons = 255;
int translucencyBarIconSymbols = 255;
int translucencyAppBackgrounds = 255;
int translucencyPageControls = 255;
int translucencyWater = 255;
int translucencyAppShadow = 255;

bool toolbarNeedsUpdate = true;

int menuStatus = 0;

int dPadSelectedToolbarButton = -1;

menuEntry_s regionfreeEntry;
menu_s menu;
menu_s * bootOptionsMenu;

int totalCols = 4;
int totalRows = 2;

int touchX = 0;
int touchY = 0;


// See the "REALLY WEIRD BUG ALERT!" in initMenu()
button fakeButton;

button pageArrowLeft;
button pageArrowRight;

buttonList toolbarButtons;

u8 bannerImage[400*222*4];
char bannerImagePath[ENTRY_PATHLENGTH+1];
bool bannerHasAlpha;
bool drawBannerImage = false;
int bannerImageWidth, bannerImageHeight;

bool firstIconHidden(menu_s* m);

int indexOfFirstVisibleMenuEntry(menu_s *m) {
	if (firstIconHidden(m)) {
		return 1;
	}
	return 0;
}

void setMenuStatus(int status) {
	dPadSelectedToolbarButton = -1;
	btnListUnHighlight(&toolbarButtons);

	/*
	 If we try to start blocking touches when we are about to launch the software updater,
	 the touch block thread will be active when the main function tries to deallocate it
	 before launching the updater.

	 In all other instances, a touch up will follow the changing of the status. This will
	 cause the touch block thread function to return and the thread to die, so it can then
	 be safely deallocated by the main function.

	 Before this if statement was implemented, launching the updater using A worked but
	 launching it using the touch screen caused a red screen hang.

	 An if statement can give so much joy.
	 */
	if (status != menuStatusSoftwareUpdate) {
		startBlockingTouches();
	}

	menuStatus = status;
}

void menuRegionFreeToggled() {
	menu.entries[0].hidden = !showRegionFree;
	updateMenuIconPositions(&menu);
	gotoFirstIcon(&menu);
}

bool menuReloadRequired = false;

void addToolbarButton(int buttonType, void (*callback)(), int tag) {
	button aButton;
	btnSetButtonType(&aButton, buttonType);
	aButton.callback = callback;
	aButton.tag = tag;
	btnAddButtonToButtonList(&aButton, &toolbarButtons);
}

void quitSettings(menu_s* m);
void showFolders();
void showHomeMenuApps();

void showSettings() {
	if (settingsMenuNeedsInit) {
		initConfigMenu();
	}

	updateMenuIconPositions(&settingsMenu);
	gotoFirstIcon(&settingsMenu);
	setMenuStatus(menuStatusSettings);

	if (animatedGrids) {
		startTransition(transitionDirectionDown, menu.pagePosition, &menu);
	}
}

void checkReturnToGrid(menu_s* m) {
	if (m->rowPosition == -1) {
		gotoFirstIcon(m);
	}
}

#define menuTopLeftActionSourceKeyB 10
#define menuTopLeftActionSourceTopLeft 20

void handleMenuTopLeftActions(int source) {
	bool playBackSound = true;
	bool playSelectSound = false;

	if (source == menuTopLeftActionSourceTopLeft && menuStatus == menuStatusIcons) {
		playSelectSound = true;
		playBackSound = false;
		showSettings();
	} else if (menuStatus == menuStatusSettings) {
		quitSettings(&menu);
	} else if (menuStatus == menuStatusTitleBrowser || menuStatus == menuStatusHomeMenuApps) {
		killTitleBrowser = true;
		if (titlemenuIsUpdating) {
			pauseTitleLoading();
		}
	} else if (menuStatus == menuStatusFolders) {
		checkReturnToGrid(&menu);

		setMenuStatus(menuStatusIcons);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, foldersMenu.pagePosition, &foldersMenu);
		}
	} else if (menuStatus == menuStatusTitleFiltering) {
		if (titlemenuIsUpdating) {
			cancelTitleLoading();
		} else {
			saveIgnoredTitleIDs();
		}

		titleMenuInitialLoadDone = false;

		setMenuStatus(menuStatusSettings);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, titleMenu.pagePosition, &titleMenu);
		}
	} else if (menuStatus == menuStatusColourAdjust) {
		saveColour(settingsColour, false);
		setMenuStatus(menuStatusColourSettings);
	} else if (menuStatus == menuStatusColourSettings) {
		alphaImagesDrawn = false;
		setMenuStatus(menuStatusThemeSettings);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, colourSelectMenu.pagePosition, &colourSelectMenu);
		}
	} else if (menuStatus == menuStatusHelp) {
		handleHelpBackButton();
	} else if (menuStatus == menuStatusGridSettings) {
		setMenuStatus(menuStatusSettings);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, gridSettingsMenu.pagePosition, &gridSettingsMenu);
		}
	} else if (menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom) {
		setMenuStatus(menuStatusThemeSettings);
	} else if (menuStatus == menuStatusThemeSelect) {
		setMenuStatus(menuStatusThemeSettings);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, themesMenu.pagePosition, &themesMenu);
		}
	} else if (menuStatus == menuStatusThemeSettings) {
		setMenuStatus(menuStatusSettings);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, themeSettingsMenu.pagePosition, &themeSettingsMenu);
		}
	} else if (menuStatus == menuStatusWaterSettings) {
		setMenuStatus(menuStatusThemeSettings);

		if (animatedGrids) {
			startTransition(transitionDirectionUp, waterMenu.pagePosition, &waterMenu);
		}
	} else if (menuStatus == menuStatusBootOptions) {
		setMenuStatus(menuStatusIcons);
	} else if (menuStatus == menuStatusHBAppOptions) {
		setMenuStatus(menuStatusIcons);
	} else {
		playBackSound = false;
	}

	if (playBackSound) audioPlay(&themeSoundBack, false);
	else if (playSelectSound) audioPlay(&themeSoundSelect, false);
}

void toolbarTopLeftAction() {
	handleMenuTopLeftActions(menuTopLeftActionSourceTopLeft);
}

void toolbarTopRightAction() {
	bool playSelectSound = true;

	if (menuStatus == menuStatusIcons) {
		showHelp();
	} else if (menuStatus == menuStatusFolders) {
		showHelpWithForcedText(foldersHelpTitle, foldersHelpBody);
	} else if (menuStatus == menuStatusThemeSelect) {
		showHelpWithForcedText(themesHelpTitle, themesHelpBody);
	} else {
		playSelectSound = false;
	}

	if (playSelectSound) audioPlay(&themeSoundSelect, false);
}

void toolbarBottomLeftAction() {
	bool playSelectSound = true;

	if (menuStatus == menuStatusIcons) {
		if (titleLoadPaused) {
			resumeTitleLoading();
		}

		setMenuStatus(menuStatusOpenHomeMenuApps);
	} else {
		playSelectSound = false;
	}

	if (playSelectSound) audioPlay(&themeSoundSelect, false);
}

void toolbarBottomRightAction() {
	bool playSelectSound = true;

	if (menuStatus == menuStatusIcons) {
		showFolders();
	} else {
		playSelectSound = false;
	}

	if (playSelectSound) audioPlay(&themeSoundSelect, false);
}

void loadThemeConfig() {
	waterEnabled = getConfigBoolForKey("waterEnabled", true, configTypeTheme);

	panelAlphaTop = getConfigIntForKey("panelAlphaTop", 0, configTypeTheme);
	panelAlphaBottom = getConfigIntForKey("panelAlphaBottom", 0, configTypeTheme);

	panelRTop = getConfigIntForKey("panelRTop", 128, configTypeTheme);
	panelGTop = getConfigIntForKey("panelGTop", 128, configTypeTheme);
	panelBTop = getConfigIntForKey("panelBTop", 128, configTypeTheme);

	panelRBottom = getConfigIntForKey("panelRBottom", 128, configTypeTheme);
	panelGBottom = getConfigIntForKey("panelGBottom", 128, configTypeTheme);
	panelBBottom = getConfigIntForKey("panelBBottom", 128, configTypeTheme);

	translucencyTopBar = getConfigIntForKey("translucencyTopBar", 255, configTypeTheme);
	translucencyBottomBar = getConfigIntForKey("translucencyBottomBar", 255, configTypeTheme);
	translucencyBarIcons = getConfigIntForKey("translucencyBarIcons", 255, configTypeTheme);
	translucencyBarIconSymbols = getConfigIntForKey("translucencyBarIconSymbols", 255, configTypeTheme);
	translucencyAppBackgrounds = getConfigIntForKey("translucencyAppBackgrounds", 255, configTypeTheme);
	translucencyPageControls = getConfigIntForKey("translucencyPageControls", 255, configTypeTheme);
	translucencyWater = getConfigIntForKey("translucencyWater", 255, configTypeTheme);
	translucencyAppShadow = getConfigIntForKey("translucencyAppShadow", 255, configTypeTheme);

	panelLeftOffsetTop = getConfigIntForKey("panelLeftOffsetTop", 0, configTypeTheme);

	waitForSounds = getConfigBoolForKey("waitForSounds", true, configTypeTheme);

	logoType = getConfigIntForKey("logoType", logoTypeDefault, configTypeTheme);
}

void initMenu(menu_s* m) {
	if(!m) {
		return;
	}


//	Initial menu status should always be to show the icons
	setMenuStatus(menuStatusIcons);

//	Configure the toolbar buttons and page select buttons
	addToolbarButton(btnButtonTypeToolbarLeft, &toolbarTopLeftAction, buttonTagTopLeft);
	addToolbarButton(btnButtonTypeToolbarRight, &toolbarTopRightAction, buttonTagTopRight);
	addToolbarButton(btnButtonTypeToolbarBottomRight, &toolbarBottomRightAction, buttonTagBottomRight);
	addToolbarButton(btnButtonTypeToolbarBottomLeft, &toolbarBottomLeftAction, buttonTagBottomLeft);

	btnSetButtonType(&pageArrowLeft, btnButtonTypePageArrowLeft);
	btnSetButtonType(&pageArrowRight, btnButtonTypePageArrowRight);


	/*

	 REALLY WEIRD BUG ALERT!

	 If the last toolbar button to be drawn is not highlighted then the text wrapping when drawing the
	 description of the currently selected app does not work.

	 I simply cannot work out why this is happening, so I have cheated.

	 This fake button is configured exactly the same way as another toolbar button,
	 but it gets drawn off screen. It is set to be highlighted all the time, so this
	 allows the text wrapping to work properly.

	 I'm guessing that this is 'undefined behaviour' due to some bad memory management,
	 so if anyone can figure out what is actually going on then this can be removed.

	 */

	btnSetButtonType(&fakeButton, btnButtonTypeToolbarBottomLeft);
	fakeButton.x = -50;
	fakeButton.y = 100;
	fakeButton.visible = true;
	fakeButton.highlighted = true;

//	If the region free launcher is available, load the setting which determines whether it should be shown in the grid
	if (regionFreeAvailable) {
		showRegionFree = getConfigBoolForKey("showRegionFree", regionFreeAvailable, configTypeMain);
	}


//	Load other settings
	sortAlpha = getConfigBoolForKey("sortAlpha", false, configTypeMain);
	totalRows = getConfigIntForKey("totalRows", 3, configTypeMain);
	clock24 = getConfigBoolForKey("clock24", false, configTypeMain);
	showDate = getConfigBoolForKey("showDate", true, configTypeMain);
	waterAnimated = getConfigBoolForKey("animatedWater", true, configTypeMain);
	showAppBackgrounds = getConfigBoolForKey("showAppBackgrounds", true, configTypeMain);
	wrapScrolling = getConfigBoolForKey("wrapScrolling", true, configTypeMain);
	dPadNavigation = getConfigBoolForKey("dPadNavigation", true, configTypeMain);
	randomiseThemeOnWake = getConfigBoolForKey("randomiseThemeOnWake", false, configTypeMain);
	animatedGrids = getConfigBoolForKey("animatedGrids", true, configTypeMain);
	show3DSFolder = getConfigBoolForKey("show3DSFolder", true, configTypeMain);
	preloadTitles = getConfigBoolForKey("preloadTitles", true, configTypeMain);
	hansTitleBoot = getConfigBoolForKey("hansTitleBoot", false, configTypeMain);

	loadThemeConfig();

//	Menu init
	m->entries=NULL;
	m->numEntries=0;
	m->selectedEntry=0;
	m->scrollLocation=0;
	m->scrollVelocity=0;
	m->scrollBarSize=0;
	m->scrollBarPos=0;
	m->scrollTarget=0;
	m->atEquilibrium=false;

//	Add the region free entry to the menu if applicable
	if(regionFreeAvailable) {
		regionfreeEntry.isRegionFreeEntry = true;
		regionfreeEntry.isShortcut = false;
		regionfreeEntry.hidden = !showRegionFree;

		extractSmdhData((smdh_s*)regionfree_bin, regionfreeEntry.name, regionfreeEntry.description, regionfreeEntry.author, regionfreeEntry.iconData);
		strcpy(regionfreeEntry.executablePath, REGIONFREE_PATH);
		addMenuEntryCopy(m, &regionfreeEntry);
	}
}

u8 bottomBar[18*320*4];

void drawBottomStatusBar(char* title) {
	int height = 18;

	if (toolbarNeedsUpdate) {
		toolbarNeedsUpdate = false;
		rgbColour * tintCol = tintColour();
		MAGFXTranslucentRect(height, 320, tintCol->r, tintCol->g, tintCol->b, translucencyBottomBar, bottomBar);
	}

	gfxDrawSpriteAlphaBlend(GFX_BOTTOM, GFX_LEFT, bottomBar, height, 320, 240-height, 0);

	rgbColour * light = lightTextColour();

	int stringLength = MATextWidthInPixels(title, &MAFontRobotoRegular10);
	MADrawText(GFX_BOTTOM, GFX_LEFT, 240-height-2, (320/2)-(stringLength/2), title, &MAFontRobotoRegular10, light->r, light->g, light->b);

	button * leftToolbarButton = btnButtonInListWithTag(&toolbarButtons, buttonTagTopLeft);
	button * rightToolbarButton = btnButtonInListWithTag(&toolbarButtons, buttonTagTopRight);
	button * bottomLeftButton = btnButtonInListWithTag(&toolbarButtons, buttonTagBottomLeft);
	button * bottomRightButton = btnButtonInListWithTag(&toolbarButtons, buttonTagBottomRight);

	leftToolbarButton->visible = true;
	rightToolbarButton->visible = false;
	bottomRightButton->visible = false;
	bottomLeftButton->visible = false;

	if (menuStatus == menuStatusIcons) {
		rightToolbarButton->visible = true;
		bottomRightButton->visible = true;
		bottomLeftButton->visible = true;
	} else if (menuStatus == menuStatusFolders || menuStatus == menuStatusThemeSelect) {
		rightToolbarButton->visible = true;
	} else if (menuStatus == menuStatusHomeMenuApps || menuStatus == menuStatusTitleBrowser || menuStatus == menuStatusTitleFiltering) {
		if (titlemenuIsUpdating) {
			bottomLeftButton->visible = true;
		}
	}

	int buttonIconLeft = 0;
	int buttonIconRight = 0;
	int buttonIconBottomLeft = 0;

	if (menuStatus == menuStatusIcons) {
		buttonIconLeft = btnButtonIconSpanner;
		buttonIconRight = btnButtonIconQuestionMark;
		buttonIconBottomLeft = btnButtonIconHome;
	} else if (menuStatus == menuStatusHelp || menuStatus == menuStatusSettings || menuStatus == menuStatusTitleBrowser || menuStatus == menuStatusHomeMenuApps) {
		buttonIconLeft = btnButtonIconBackArrow;
	} else if (menuStatus == menuStatusFolders || menuStatus == menuStatusThemeSelect) {
		buttonIconLeft = btnButtonIconBackArrow;
		buttonIconRight = btnButtonIconQuestionMark;
	} else if (menuStatus == menuStatusColourSettings || menuStatus == menuStatusColourAdjust || menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom || menuStatus == menuStatusWaterSettings || menuStatus == menuStatusThemeSettings || menuStatus == menuStatusGridSettings || menuStatus == menuStatusTitleFiltering) {
		buttonIconLeft = btnButtonIconBackArrow;
	}

	leftToolbarButton->buttonIcon = buttonIconLeft;
	rightToolbarButton->buttonIcon = buttonIconRight;

	bottomLeftButton->buttonIcon = buttonIconBottomLeft;
	bottomRightButton->buttonIcon = btnButtonIconFolder;

	btnDrawButtonList(&toolbarButtons);
	btnDrawButton(&fakeButton);
}

int * coordsForMenuEntry(int row, int col, menu_s *m) {
	int xOffset = 121;
	int yOffset = 33;
	int gap = 15;

	if (totalRows == 3) {
		xOffset += 31;
	} else if (totalRows == 1) {
		xOffset -= 41;
	}

	int x = xOffset - ( row * ENTRY_ICON_HEIGHT ) - (row * gap);
	int y = yOffset + (( col * ENTRY_ICON_WIDTH ) + (col * gap));

	static int  r[2];
	r[0] = x;
	r[1] = y;
	return r;
}

u8 appBackground[56*56*4];
u8 pageSelected[13*13*4];
u8 pageUnselected[13*13*4];
u8 cartBackground[59*59*4];
u8 cartBackgroundSelected[59*59*4];
u8 appBackgroundSelected[56*56*4];
bool alphaImagesDrawn = false;

u8 pageControlPanelLeft[81*36*4];
u8 pageControlPanelRight[81*36*4];
bool pageControlPanelsDrawn = false;

void drawGridWithPage(menu_s* m, int page, int pageYOffset, int pageXOffset, bool gridOnly) {

	if (m && m->selectedEntry > -1) {
		menuEntry_s *selectedEntry = getMenuEntry(m, m->selectedEntry);
		if (selectedEntry && selectedEntry->hidden && m->numEntries > 1) {
			m->selectedEntry = m->selectedEntry + 1;
		}
	}

	rgbColour * inactiveCol = inactiveColour();
	rgbColour * tintCol = tintColour();

	if (!alphaImagesDrawn) { // Prepare translucent images for drawing
		MAGFXImageWithRGBAndAlphaMask(inactiveCol->r, inactiveCol->g, inactiveCol->b, (u8*)appbackgroundalphamask_bin, appBackground, 56, 56);
		MAGFXImageWithRGBAndAlphaMask(inactiveCol->r, inactiveCol->g, inactiveCol->b, (u8*)pageiconalphamask_bin, pageUnselected, 13, 13);
		MAGFXImageWithRGBAndAlphaMask(tintCol->r, tintCol->g, tintCol->b, (u8*)pageiconalphamask_bin, pageSelected, 13, 13);
		MAGFXImageWithRGBAndAlphaMask(tintCol->r, tintCol->g, tintCol->b, (u8*)cartbackgroundalphamask_bin, cartBackgroundSelected, 59, 59);
		MAGFXImageWithRGBAndAlphaMask(inactiveCol->r, inactiveCol->g, inactiveCol->b, (u8*)cartbackgroundalphamask_bin, cartBackground, 59, 59);
		MAGFXImageWithRGBAndAlphaMask(tintCol->r, tintCol->g, tintCol->b, (u8*)appbackgroundalphamask_bin, appBackgroundSelected, 56, 56);

		alphaImagesDrawn = true;
	}

	if (!pageControlPanelsDrawn) { // Prepare page controls for drawing
		MAGFXImageWithRGBAndAlphaMask(panelRBottom, panelGBottom, panelBBottom, (u8*)pageControlPanelLeftAlphaMask_bin, pageControlPanelLeft, 81, 36);
		MAGFXImageWithRGBAndAlphaMask(panelRBottom, panelGBottom, panelBBottom, (u8*)pageControlPanelRightAlphaMask_bin, pageControlPanelRight, 81, 36);

		pageControlPanelsDrawn = true;
	}

	if (!gridOnly) {
		bool drawPanel = true;

		if (m && m->selectedEntry > -1) {
			menuEntry_s *me = getMenuEntry(m, m->selectedEntry);
			if (me && me->hasBanner && me->bannerIsFullScreen) {
				drawPanel = false;
			}
		}

		if (drawPanel)
			MAGFXDrawPanel(GFX_TOP, false);


	}

	int totalDrawn = 0;

	menuEntry_s* me=m->entries;
	int i=0;
	int h=0;
	while(me) {
		if (!me->hidden && me->page == page) {
			h+=drawMenuEntry(me, GFX_BOTTOM, (i==m->selectedEntry && m->rowPosition>-1), m, pageYOffset, pageXOffset, !gridOnly);
			totalDrawn++;
		}

		me=me->next;
		i++;
	}

	if (showAppBackgrounds) {
		int totalSpaces = totalRows * totalCols;
		int numPadding = totalSpaces - totalDrawn;

		int i;
		int r = totalRows-1;
		int c = totalCols-1;
		for (i=0; i<numPadding; i++) {
			int * coords = coordsForMenuEntry(r, c, m);
			int x = coords[0];
			int y = coords[1];

			x += pageXOffset;
			y += pageYOffset;

			if (themeImageExists(themeImageAppBackground)) {
				drawThemeImage(themeImageAppBackground, GFX_BOTTOM, x+3, y+4);
			} else {
				gfxDrawSpriteAlphaBlendFade(GFX_BOTTOM, GFX_LEFT, appBackground, 56, 56, x+3, y+4, translucencyAppBackgrounds);
			}

			c--;
			if (c < 0) {
				c = totalCols-1;
				r--;
			}

		}
	}


	if (!gridOnly) {

//		Draw bottom screen paging arrows and page indicators
		if (m->totalPages > 1) {
			if (page > 0 || wrapScrolling) {
				gfxDrawSpriteAlphaBlendFade(GFX_BOTTOM, GFX_LEFT, (u8*)pageControlPanelLeft, 81, 36, 80, 0, panelAlphaBottom);
				btnDrawButton(&pageArrowLeft);
			}

			if (page < (m->totalPages - 1) || wrapScrolling) {
				gfxDrawSpriteAlphaBlendFade(GFX_BOTTOM, GFX_LEFT, (u8*)pageControlPanelRight, 81, 36, 80, 284, panelAlphaBottom);
				btnDrawButton(&pageArrowRight);
			}

			int totalIndicatorWidth = 13 * m->totalPages;
			int pageIndicatorX = 5;
			int pageIndicatorY = (320-totalIndicatorWidth)/2;

			int count;

			for (count=0; count < page; count++) {
				gfxDrawSpriteAlphaBlendFade(GFX_BOTTOM, GFX_LEFT, pageUnselected, 13, 13, pageIndicatorX, pageIndicatorY, translucencyPageControls);
				pageIndicatorY += 13;
			}

			gfxDrawSpriteAlphaBlend(GFX_BOTTOM, GFX_LEFT, pageSelected, 13, 13, pageIndicatorX, pageIndicatorY);
			pageIndicatorY += 13;

			for (count=0; count<(m->totalPages - page - 1); count++) {
				gfxDrawSpriteAlphaBlendFade(GFX_BOTTOM, GFX_LEFT, pageUnselected, 13, 13, pageIndicatorX, pageIndicatorY, translucencyPageControls);
				pageIndicatorY += 13;
			}
		}
	}
}

int transitionFromPage = -1;
int transitionOutPixel;
int transitionInPixel;
int transitionSpeed;
int transitionDirection;
menu_s * transitionFromMenu;

void drawGrid(menu_s* m) {
	if (transitionFromPage > -1) {
		int transitionOutPixelX = 0;
		int transitionOutPixelY = 0;

		int transitionInPixelX = 0;
		int transitionInPixelY = 0;

		if (transitionDirection == transitionDirectionLeft || transitionDirection == transitionDirectionRight) {
			transitionOutPixelY = transitionOutPixel;
			transitionInPixelY = transitionInPixel;
		} else if (transitionDirection == transitionDirectionUp || transitionDirection == transitionDirectionDown) {
			transitionOutPixelX = transitionOutPixel;
			transitionInPixelX = transitionInPixel;
		}

		if (transitionFromMenu) {
			drawGridWithPage(transitionFromMenu, transitionFromPage, transitionOutPixelY, transitionOutPixelX, true);
		}

		if (m) {
			drawGridWithPage(m, m->pagePosition, transitionInPixelY, transitionInPixelX, false);
		}

		transitionOutPixel -= transitionSpeed;
		transitionInPixel -= transitionSpeed;

		if (transitionInPixel == 0) {
			transitionFromPage = -1;
		}
	} else {
		drawGridWithPage(m, m->pagePosition, 0, 0, false);
	}
}

void drawMenu(menu_s* m) {
	if(!m) {
		return;
	}

	if (!showRegionFree && m->selectedEntry == 0) {
		m->selectedEntry = 1;
	}


	if (menuStatusIcons == menuStatusIcons) {
		if (m->numEntries == 1) {
			char * emptyFolderText = "Empty folder";
			int len = MATextWidthInPixels(emptyFolderText, &MAFontRobotoRegular16);
			rgbColour * dark = darkTextColour();
			MADrawText(GFX_BOTTOM, GFX_LEFT, 110, (320/2)-(len/2), emptyFolderText, &MAFontRobotoRegular16, dark->r, dark->g, dark->b);

			if (transitionFromPage > -1) {
				drawGrid(NULL);
			}
		} else {
			drawGrid(m);
		}

		char * cfn = currentFolderName();
		char title[strlen(cfn) + strlen("Folder: ")];
		strcpy(title, "Folder: ");
		strcat(title, cfn);
		drawBottomStatusBar(title);
		free(cfn);
	}
}

void updateMenuIconPositions(menu_s* m) {
	int currentColumn = 0;
	int currentRow = 0;
	int currentPage = 0;
	m->totalPages = 0;
	int i=0;

	menuEntry_s* me=m->entries;
	while(me) {
		if (!me->hidden) {
			me->row = currentRow;
			me->col = currentColumn;
			me->page = currentPage;


			if (currentRow == 0 && currentColumn == 0) {
				m->totalPages = m->totalPages + 1;
			}

			currentColumn++;

			if (currentColumn > 3) {
				currentColumn = 0;
				currentRow++;
			}

			if (currentRow >= totalRows) {
				currentRow = 0;
				currentColumn = 0;
				currentPage++;
			}
		}

		me=me->next;
		i++;
	}
}

void addMenuEntry(menu_s* m, menuEntry_s* me) {
	if(!m || !me)return;

	// add to the end of the list
	menuEntry_s** l = &m->entries;
	while(*l)l=&(*l)->next;
	*l = me;
	me->next = NULL;
	m->numEntries++;
}

void addMenuEntryCopy(menu_s* m, menuEntry_s* me) {
	if(!m || !me)return;

	menuEntry_s* me2=malloc(sizeof(menuEntry_s));
	if(!me2)return;

	memcpy(me2, me, sizeof(menuEntry_s));

	addMenuEntry(m, me2);
}

void freeMenuEntry(menuEntry_s* me) {
	if(!me)return;

	freeDescriptor(&me->descriptor);
}

void clearMenuEntries(menu_s* m) {
	if(!m)return;

	m->selectedEntry=0;

	menuEntry_s* me = m->entries;
	menuEntry_s* temp = NULL;
	while(me) {
		temp=me->next;
		me->next = NULL;
		freeMenuEntry(me);
		free(me);
		me = temp;
	}

	m->numEntries = 0;
	m->entries = NULL;

	if(regionFreeAvailable) {
		// should always be available
		addMenuEntryCopy(m, &regionfreeEntry);
	}
}

void createMenuEntry(menu_s* m, char* execPath, char* name, char* description, char* author, u8* iconData) {
	if(!m || !name || !description || !iconData)return;

	menuEntry_s* me=malloc(sizeof(menuEntry_s));
	if(!me)return;

	initMenuEntry(me, execPath, name, description, author, iconData);

	addMenuEntry(m, me);
}

menuEntry_s* getMenuEntry(menu_s* m, u16 n) {
	if(!m || n>=m->numEntries)return NULL;
	menuEntry_s* me=m->entries;
	while(n && me){me=me->next; n--;}
	return me;
}

int indexOfMenuEntryAtPageRowColInMenu(int page, int row, int col, menu_s* m) {
	if (!m) return -1;

	menuEntry_s* me = m->entries;
	int i=0;
	while(me) {
		if (me->page == page && me->row == row && me->col == col) {
			if (me->hidden) {
				i++;
			}

			return i;
		}
		me=me->next;
		i++;
	}

	return -1;
}

int indexOfFirstVisibleMenuEntryOnPage(int page, menu_s* m);

void startTransition(int direction, int fromPage, menu_s* fromMenu) {
	int absTransitionSpeed = 80;

	if (direction == transitionDirectionLeft) {
		transitionInPixel = 320;
		transitionSpeed = absTransitionSpeed;
	} else if (direction == transitionDirectionRight) {
		transitionInPixel = -320;
		transitionSpeed = -absTransitionSpeed;
	} else if (direction == transitionDirectionDown) {
		transitionInPixel = 240;
		transitionSpeed = absTransitionSpeed;
	} else if (direction == transitionDirectionUp) {
		transitionInPixel = -240;
		transitionSpeed = -absTransitionSpeed;
	}

	transitionDirection = direction;
	transitionFromPage = fromPage;
	transitionOutPixel = 0;
	transitionFromMenu = fromMenu;
}

void checkGotoNextPage(menu_s* m, s8 *move, bool preserveCursorPosition) {
	if (m->totalPages < 2) {
		return;
	}

	int previousPage = m->pagePosition;

	m->pagePosition = m->pagePosition + 1;
	bool pageChanged = false;

	if (m->pagePosition > (m->totalPages - 1)) {
		if (wrapScrolling) {
			m->pagePosition = 0;
			pageChanged = true;
		} else {
			m->pagePosition = m->pagePosition - 1;
			pageChanged = false;
		}
	} else {
		pageChanged = true;
	}

	if (pageChanged) {
		audioPlay(&themeSoundMove, false);

		if (animatedGrids) {
			startTransition(transitionDirectionLeft, previousPage, m);
		}

		if (dPadSelectedToolbarButton == -1) {

			if (preserveCursorPosition) {
				int firstIndex = indexOfFirstVisibleMenuEntryOnPage(m->pagePosition, m);
				int indexOffset = (m->rowPosition * totalCols) + m->colPosition;
				int newIndex = firstIndex + indexOffset;
				*move += (newIndex - m->selectedEntry);
			} else {
				int oldRowPosition = m->rowPosition;
				m->rowPosition = 0;
				m->colPosition = 0;
				int newSelectedIndex = indexOfMenuEntryAtPageRowColInMenu(m->pagePosition, 0, 0, m);
				m->selectedEntry = newSelectedIndex;

				if ((m->selectedEntry + (4*oldRowPosition)) <= (m->numEntries - 1)) {
					*move += (4*oldRowPosition);
					m->colPosition = 0;
					m->rowPosition = oldRowPosition;
				}
			}
		}
	}
}

void checkGotoPreviousPage(menu_s* m, s8 *move, bool preserveCursorPosition) {
	if (m->totalPages < 2) {
		return;
	}

	int previousPage = m->pagePosition;

	m->pagePosition = m->pagePosition - 1;
	bool pageChanged = false;

	if (m->pagePosition < 0) {
		if (wrapScrolling) {
			m->pagePosition = (m->totalPages - 1);
			pageChanged = true;
		} else {
			m->pagePosition = m->pagePosition + 1;
			pageChanged = false;
		}
	} else {
		pageChanged = true;
	}

	if (pageChanged) {
		audioPlay(&themeSoundMove, false);

		if (animatedGrids) {
			startTransition(transitionDirectionRight, previousPage, m);
		}

		if (dPadSelectedToolbarButton == -1) {

			if (preserveCursorPosition) {
				int firstIndex = indexOfFirstVisibleMenuEntryOnPage(m->pagePosition, m);
				int indexOffset = (m->rowPosition * totalCols) + m->colPosition;
				int newIndex = firstIndex + indexOffset;
				*move += (newIndex - m->selectedEntry);
			} else {
				//Store the old row which was selected
				int oldRowPosition = m->rowPosition;

				//Move the selection to the first icon on this page
				m->rowPosition = 0;
				m->colPosition = 0;
				int newSelectedIndex = indexOfMenuEntryAtPageRowColInMenu(m->pagePosition, 0, 0, m);
				m->selectedEntry = newSelectedIndex;

				//Move the selection to the same position on the new page
				*move += (3 + (4*oldRowPosition));
				m->colPosition = 3;
				m->rowPosition = oldRowPosition;

				//If we moved back to the last page
				if (m->pagePosition == (m->totalPages - 1)) {

					//If the current position on the last page is not occupied by an app icon
					if (*move + m->selectedEntry >= m->numEntries) {

						//While the current position on the page is still not occupied by an icon
						while (*move + m->selectedEntry >= m->numEntries) {
							//Go left by one icon
							*move -= 1;
							m->colPosition = m->colPosition - 1;

							//If we have got to the beginning of the row, go to the end of the next row up
							if (m->colPosition < 0) {
								m->colPosition = 3;
								m->rowPosition = m->rowPosition - 1;
							}

							//If we have dropped off the left and edge of the first row then we must be on an empty page
							//This really shouldn't ever happen, but just in case, this code will reset back to the first
							//icon on the page and break out of the while loop so we don't hang here
							if (m->rowPosition < 0) {
								m->colPosition = 0;
								m->rowPosition = 0;
								int newSelectedIndex = indexOfMenuEntryAtPageRowColInMenu(m->pagePosition, 0, 0, m);
								m->selectedEntry = newSelectedIndex;
								break;
							}
						}
					}
				}
			}
		}
	}
}

bool touchWithin(int touchX, int touchY, int targetX, int targetY, int targetW, int targetH) {
	if (touchX > targetY && touchX < (targetY+targetW) && (240-touchY) > targetX && (240-touchY) < (targetX+targetH)) {
		return true;
	}

	return false;
}

void gotoFirstIcon(menu_s* m) {
	m->rowPosition = 0;
	m->colPosition = 0;
	m->pagePosition = 0;

	int selectedEntry = 0;
	if (m->entries[0].hidden) {
		selectedEntry++;
	}

	m->selectedEntry = selectedEntry;
}

void reloadMenu(menu_s* m) {
	clearMenuEntries(m);
	scanHomebrewDirectory(m, currentFolder());
	m->entries[0].hidden = !showRegionFree;
	updateMenuIconPositions(m);
	gotoFirstIcon(m);
	menuReloadRequired = false;
}

void quitSettings(menu_s* m) {
	setConfigBool("showRegionFree", showRegionFree, configTypeMain);
	setConfigBool("sortAlpha", sortAlpha, configTypeMain);
	setConfigInt("totalRows", totalRows, configTypeMain);
	setConfigBool("clock24", clock24, configTypeMain);
	setConfigBool("showDate", showDate, configTypeMain);
	setConfigBool("animatedWater", waterAnimated, configTypeMain);
	setConfigBool("showAppBackgrounds", showAppBackgrounds, configTypeMain);
	setConfigBool("wrapScrolling", wrapScrolling, configTypeMain);
	setConfigBool("dPadNavigation", dPadNavigation, configTypeMain);
	setConfigBool("randomTheme", randomTheme, configTypeMain);
	setConfigBool("randomiseThemeOnWake", randomiseThemeOnWake, configTypeMain);
	setConfigBool("animatedGrids", animatedGrids, configTypeMain);
	setConfigBool("show3DSFolder", show3DSFolder, configTypeMain);
	setConfigBool("hansTitleBoot", hansTitleBoot, configTypeMain);

	setConfigBool("waterEnabled", waterEnabled, configTypeTheme);
	setConfigInt("logoType", logoType, configTypeTheme);

	setConfigInt("panelAlphaTop", panelAlphaTop, configTypeTheme);
	setConfigInt("panelAlphaBottom", panelAlphaBottom, configTypeTheme);

	setConfigInt("panelRTop", panelRTop, configTypeTheme);
	setConfigInt("panelGTop", panelGTop, configTypeTheme);
	setConfigInt("panelBTop", panelBTop, configTypeTheme);

	setConfigInt("panelRBottom", panelRBottom, configTypeTheme);
	setConfigInt("panelGBottom", panelGBottom, configTypeTheme);
	setConfigInt("panelBBottom", panelBBottom, configTypeTheme);

	setConfigInt("translucencyTopBar", translucencyTopBar, configTypeTheme);
	setConfigInt("translucencyWater", translucencyWater, configTypeTheme);
	setConfigInt("translucencyAppShadow", translucencyAppShadow, configTypeTheme);

	setConfigInt("translucencyBottomBar", translucencyBottomBar, configTypeTheme);
	setConfigInt("translucencyBarIcons", translucencyBarIcons, configTypeTheme);
	setConfigInt("translucencyBarIconSymbols", translucencyBarIconSymbols, configTypeTheme);
	setConfigInt("translucencyAppBackgrounds", translucencyAppBackgrounds, configTypeTheme);
	setConfigInt("translucencyPageControls", translucencyPageControls, configTypeTheme);

	setConfigInt("preloadTitles", preloadTitles, configTypeMain);

	setConfigInt("panelLeftOffsetTop", panelLeftOffsetTop, configTypeTheme);

	setConfigBool("waitForSounds", waitForSounds, configTypeTheme);

	saveConfig();

	if (menuReloadRequired) {
		reloadMenu(m);
	}

	updateMenuIconPositions(m);
	setMenuStatus(menuStatusIcons);
	checkReturnToGrid(m);

	if (animatedGrids) {
		startTransition(transitionDirectionUp, settingsMenu.pagePosition, &settingsMenu);
	}
}

bool trueBool = true;

menuEntry_s * updateMenuTicks(menu_s* m, char * selectedString, bool useExeecutablePath) {
	menuEntry_s * tickedItem = NULL;

	menuEntry_s* me = m->entries;

	if (m == &themesMenu && randomTheme) {
		bool first = true;
		while (me) {
			if (first) {
				me->showTick = &trueBool;
				tickedItem = me;
				first = false;
			}
			else {
				me->showTick = NULL;
			}

			me=me->next;
		}
	} else {
		while(me) {
			if ((!useExeecutablePath &&  strcmp(me->name, selectedString) == 0) || (useExeecutablePath &&  strcmp(me->executablePath, selectedString) == 0)) {
				me->showTick = &trueBool;
				tickedItem = me;
			}
			else {
				me->showTick = NULL;
			}

			me=me->next;
		}
	}

	return tickedItem;
}

void setPositionsToCurrentMenuSelection(menu_s* m);

void showFolders() {
	buildFoldersList();

	if (foldersMenu.numEntries == 1) {
		menuEntry_s * menuEntry3DSFolder = getMenuEntry(&foldersMenu, 0);
		menuEntry3DSFolder->hidden = false;
		foldersMenu.selectedEntry = 0;
	} else {
		char * cfn = currentFolderName();
		menuEntry_s *tickedEntry = updateMenuTicks(&foldersMenu, cfn, false);
		free(cfn);
		int tickedIndex = indexOfMenuEntryAtPageRowColInMenu(tickedEntry->page, tickedEntry->row, tickedEntry->col, &foldersMenu);
		if (tickedIndex == -1) tickedIndex = 0;
		foldersMenu.selectedEntry = tickedIndex;
	}

	setPositionsToCurrentMenuSelection(&foldersMenu);
	setMenuStatus(menuStatusFolders);

	if (animatedGrids) {
		if (menu.numEntries > 1) {
			startTransition(transitionDirectionDown, menu.pagePosition, &menu);
		} else {
			startTransition(transitionDirectionDown, 0, NULL);
		}
	}
}

void keyBAction() {
	handleMenuTopLeftActions(menuTopLeftActionSourceKeyB);
}

void switchToolbarButtons() {
	int newSelectedToolbarButton = dPadSelectedToolbarButton;

	if (dPadSelectedToolbarButton == 0) {
		newSelectedToolbarButton = 1;
	} else if (dPadSelectedToolbarButton == 1) {
		newSelectedToolbarButton = 0;
	} else if (dPadSelectedToolbarButton == 2) {
		newSelectedToolbarButton = 3;
	} else if (dPadSelectedToolbarButton == 3) {
		newSelectedToolbarButton = 2;
	}

	if (toolbarButtons.buttons[newSelectedToolbarButton]->visible) {
		btnListUnHighlight(&toolbarButtons);
		dPadSelectedToolbarButton = newSelectedToolbarButton;
	}
}

void handleDPadToolbarActions(menu_s* m) {
	if (dPadSelectedToolbarButton == 0) {
		toolbarTopLeftAction();
	} else if (dPadSelectedToolbarButton == 1) {
		toolbarTopRightAction();
	} else if (dPadSelectedToolbarButton == 2) {
		toolbarBottomRightAction();
	} else if (dPadSelectedToolbarButton == 3) {
		toolbarBottomLeftAction();
	}
}

bool firstIconHidden(menu_s* m) {
	if (m && m->numEntries > 0) {
		menuEntry_s *me = getMenuEntry(m, 0);
		if (me && me->hidden) {
			return true;
		}
	}

	return false;
}

int indexOfFirstVisibleMenuEntryOnPage(int page, menu_s* m) {
	int first = totalRows * totalCols * page;

	if (firstIconHidden(m)) {
		first++;
	}

	else if (!show3DSFolder && menuStatus == menuStatusFolders) {
		first++;
	}

	if (first >= m->numEntries) {
		first = m->numEntries - 1;
	}

	return first;
}

int indexOfLastVisibleMenuEntryOnPage(int page, menu_s* m) {
	int last = totalRows * totalCols * (page+1);
	last--;

	if (firstIconHidden(m)) {
		last++;
	}

	if (last >= m->numEntries) {
		last = m->numEntries - 1;
	}

	return last;
}


void setPositionsToCurrentMenuSelection(menu_s* m) {
	if (m->rowPosition == -1) {
		return;
	}

	menuEntry_s *me = getMenuEntry(m, m->selectedEntry);
	m->rowPosition = me->row;
	m->colPosition = me->col;
}

bool touchWithinMenuEntryIcon(menu_s * m, touchPosition *touch, int * i, int * newRow, int * newCol) {
	menuEntry_s* me=m->entries;
	bool gotAppMatch = false;

	while(me) {
		if (me->page == m->pagePosition) {
			if (newRow != NULL) {
				*newRow = me->row;
			}

			if (newCol != NULL) {
				*newCol = me->col;
			}

			int iconX = me->iconX;
			int iconY = me->iconY;
			int iconW = me->iconW;
			int iconH = me->iconH;

			if (touchWithin(touch->px, touch->py, iconX, iconY, iconW, iconH)) {
				gotAppMatch = true;
				break;
			}
		}

		me=me->next;
		*i = *i + 1;
	}

	return gotAppMatch;
}

void enterCornerButtons(menu_s* m) {
	m->rowPosition = -1;
	m->selectedEntry = -1;
}

void checkPlaySelectSound(menu_s *m) {
	bool playSelectSound = false;

	menuEntry_s *me = getMenuEntry(m, m->selectedEntry);

	if (me->isRegionFreeEntry) {
		if (regionFreeGamecardIn)
			playSelectSound = true;
	} else {
		playSelectSound = true;
	}

	if (playSelectSound && themeSoundSelect.loaded) {
		audioPlay(&themeSoundSelect, false);
		waitForSoundToFinishPlaying(&themeSoundSelect);
	}
}

bool updateGrid(menu_s* m) {
	if (transitionFromPage > -1) {
		return false;
	}

	if(!m) {
		return false;
	}
	if(!m->numEntries) {
		return false;
	}

	s8 move=0;
	touchPosition touch;
	hidTouchRead(&touch);

	touchX = touch.px;
	touchY = touch.py;

	if (hidKeysDown()&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN)) {
	   audioPlay(&themeSoundMove, false);
	}

	if (dPadSelectedToolbarButton > -1) {
		toolbarButtons.buttons[dPadSelectedToolbarButton]->highlighted = true;
	}

	if (hidKeysDown()&KEY_B) {
		keyBAction();
		return false;
	}

	if(hidKeysDown()&KEY_RIGHT) {
		if (m->rowPosition == -1) {
			switchToolbarButtons();
		}
		else if (m->colPosition < 3 && m->selectedEntry < (m->numEntries-1)) {
			move++;
			m->colPosition = m->colPosition + 1;
		}
		else if (m->colPosition == 3) {
			checkGotoNextPage(m, &move, false);
		}
		else if (m->selectedEntry == m->numEntries-1) {
			if (wrapScrolling) {
				checkGotoNextPage(m, &move, false);
			}
		}
	}

	if(hidKeysDown()&KEY_LEFT) {
		if (m->rowPosition == -1) {
			switchToolbarButtons();
		}
		else if (m->colPosition > 0) {
			move--;
			m->colPosition = m->colPosition - 1;
		}
		else if (m->colPosition == 0) {
			checkGotoPreviousPage(m, &move, false);
		}
	}
	if(hidKeysDown()&KEY_UP) {
		if (dPadNavigation && m->rowPosition == 0) {
			if (toolbarButtons.buttons[1]->visible && m->colPosition > 1) {
				dPadSelectedToolbarButton = 1;
				enterCornerButtons(m);
			}
			else {
				dPadSelectedToolbarButton = 0;
				enterCornerButtons(m);
			}

			return false;
		}

		else if (dPadNavigation && m->rowPosition == -1) {
			int last = indexOfLastVisibleMenuEntryOnPage(m->pagePosition, m);
			bool reselectGrid = false;

			if (dPadSelectedToolbarButton == 2) {
				m->selectedEntry = last;
				reselectGrid = true;
			}
			else if (dPadSelectedToolbarButton == 3) {
				menuEntry_s *lastMenuEntry = getMenuEntry(m, last);
				last -= lastMenuEntry->col;
				m->selectedEntry = last;
				reselectGrid = true;
			}

			if (reselectGrid) {
				menuEntry_s *currentMenuEntry = getMenuEntry(m, m->selectedEntry);
				m->rowPosition = currentMenuEntry->row;
				m->colPosition = currentMenuEntry->col;

				dPadSelectedToolbarButton = -1;
				btnListUnHighlight(&toolbarButtons);
			}

			return false;
		}

		else if (m->rowPosition > 0) {
			move-=4;
			m->rowPosition = m->rowPosition - 1;
		}
	}

	if(hidKeysDown()&KEY_DOWN) {
		if (dPadNavigation && m->rowPosition == -1) {
			bool reselectGrid = false;

			if (dPadSelectedToolbarButton == 0) {
				m->selectedEntry = indexOfFirstVisibleMenuEntryOnPage(m->pagePosition, m);
				reselectGrid = true;
			}
			else if (dPadSelectedToolbarButton == 1) {
				int first = indexOfFirstVisibleMenuEntryOnPage(m->pagePosition, m);
				first += 3;
				if (first >= m->numEntries) {
					first = m->numEntries-1;
				}

				m->selectedEntry = first;

				reselectGrid = true;
			}

			if (reselectGrid) {
				menuEntry_s *currentMenuEntry = getMenuEntry(m, m->selectedEntry);
				m->rowPosition = currentMenuEntry->row;
				m->colPosition = currentMenuEntry->col;

				dPadSelectedToolbarButton = -1;
				btnListUnHighlight(&toolbarButtons);
			}

			return false;
		}
		else if (dPadNavigation && (m->rowPosition == totalRows-1 || (m->selectedEntry + totalCols) >= m->numEntries)) {


			if (toolbarButtons.buttons[2]->visible && m->colPosition > 1) {
				dPadSelectedToolbarButton = 2;
				enterCornerButtons(m);
			}
			if (toolbarButtons.buttons[3]->visible && m->colPosition < 2) {
				dPadSelectedToolbarButton = 3;
				enterCornerButtons(m);
			}

			return false;
		}

		if (m->rowPosition < (totalRows - 1) && m->selectedEntry < (m->numEntries-4)) {
			move+=4;
			m->rowPosition = m->rowPosition + 1;
		}
	}

	if(hidKeysDown()&KEY_R) {
		checkGotoNextPage(m, &move, true);
	}

	if(hidKeysDown()&KEY_L) {
		checkGotoPreviousPage(m, &move, true);
	}

	if(hidKeysDown()&KEY_SELECT) {
		if (menuStatus == menuStatusIcons || menuStatus == menuStatusHomeMenuApps) {
			if (dPadSelectedToolbarButton == -1) {
				menuEntry_s *me = getMenuEntry(m, m->selectedEntry);
				bootOptionsMenu = m;
				alertSelectedButton = 0;

				if (me->title_id > 0 || me->isRegionFreeEntry || me->isShortcut) {
					setMenuStatus(menuStatusBootOptions);
				}
			}
		}
	}

	u16 oldEntry=m->selectedEntry;

	if (hidKeysDown()&KEY_TOUCH && !touchesAreBlocked) {
		m->firstTouch=touch;

		int newRow = 0;
		int newCol = 0;
		int i=0;

		bool gotAppMatch = touchWithinMenuEntryIcon(m, &touch, &i, &newRow, &newCol);

		if (gotAppMatch) {
			btnListUnHighlight(&toolbarButtons);
			dPadSelectedToolbarButton = -1;

			if(m->selectedEntry==i) {
				checkPlaySelectSound(m);
				return true;
			} else {
				audioPlay(&themeSoundMove, false);
				m->selectedEntry=i;
				m->rowPosition = newRow;
				m->colPosition = newCol;

				startBlockingTouches();
			}
		} else {
			if (btnTouchWithin(touchX, touchY, &pageArrowLeft)) {
				checkGotoPreviousPage(m, &move, true);
			} else if (btnTouchWithin(touchX, touchY, &pageArrowRight)) {
				checkGotoNextPage(m, &move, true);
			}

			btnListCheckHighlight(&toolbarButtons, touchX, touchY);
		}
	} else if(hidKeysHeld()&KEY_TOUCH && !touchesAreBlocked){
		btnListCheckHighlight(&toolbarButtons, touchX, touchY);
	} else if (hidKeysUp()&KEY_TOUCH && !touchesAreBlocked) {
		btnListCheckHighlight(&toolbarButtons, touchX, touchY);
		btnListCheckRunCallback(&toolbarButtons, m->previousTouch.px, m->previousTouch.py);
	}

	if (move + m->selectedEntry < 0) {
		m->selectedEntry=indexOfFirstVisibleMenuEntry(m);
		setPositionsToCurrentMenuSelection(m);
	} else if (move + m->selectedEntry >= m->numEntries) {
		m->selectedEntry=m->numEntries-1;
		setPositionsToCurrentMenuSelection(m);
	} else {
		m->selectedEntry+=move;
	}

	if(m->selectedEntry!=oldEntry)m->atEquilibrium=false;

	if(hidKeysDown()&KEY_A) {
		/*
		 We're about to launch a title from the A button

		 We need to re-scan the buttons to clear the A button's down state.
		 If we don't do this, then the first title on the title menu
		 will launch straight away.

		 */

		hidScanInput();

		if (m->rowPosition == -1) {
			handleDPadToolbarActions(m);
			return false;
		}

		if (m->numEntries == 0) {
			return false;
		}

		if (m->numEntries == 1 && menuStatus == menuStatusIcons) {
			return false;
		}

		checkPlaySelectSound(m);

		return true;
	}

	m->previousTouch=touch;

	return false;
}

// warning Try to get rid of this if possible
void handleNonGridToolbarNavigation() {
	btnListUnHighlight(&toolbarButtons);

	if (hidKeysDown()&KEY_UP && toolbarButtons.buttons[0]->visible == true) {
		if (dPadSelectedToolbarButton == -1) {
			dPadSelectedToolbarButton = 0;
		}
	}

	if (hidKeysDown()&KEY_DOWN) {
		dPadSelectedToolbarButton = -1;
	}

	if (hidKeysDown()&KEY_LEFT || hidKeysDown()&KEY_RIGHT) {
		if (dPadSelectedToolbarButton == 0 && toolbarButtons.buttons[1]->visible == true) {
			dPadSelectedToolbarButton = 1;
		} else if (dPadSelectedToolbarButton == 1 && toolbarButtons.buttons[0]->visible == true) {
			dPadSelectedToolbarButton = 0;
		}
	}

	if (hidKeysDown()&KEY_A) {
		if (dPadSelectedToolbarButton == 0) {
			hidScanInput();
			toolbarTopLeftAction();
		} else if (dPadSelectedToolbarButton == 1) {
			hidScanInput();
			toolbarTopRightAction();
		}
	}

	if (hidKeysDown()&KEY_B) {
		keyBAction();
	}

	if (dPadSelectedToolbarButton > -1) {
		toolbarButtons.buttons[dPadSelectedToolbarButton]->highlighted = true;
	}

	touchPosition touch;
	hidTouchRead(&touch);
	touchX = touch.px;
	touchY = touch.py;

	if (hidKeysDown()&KEY_TOUCH) {
		btnListCheckHighlight(&toolbarButtons, touchX, touchY);
	} else if(hidKeysHeld()&KEY_TOUCH){
		btnListCheckHighlight(&toolbarButtons, touchX, touchY);
	} else if (hidKeysUp()&KEY_TOUCH) {
		btnListCheckHighlight(&toolbarButtons, touchX, touchY);
		btnListCheckRunCallback(&toolbarButtons, menu.previousTouch.px, menu.previousTouch.py);
	}

	menu.previousTouch.px = touchX;
	menu.previousTouch.py = touchY;
}

bool updateMenu(menu_s* m) {
	if (menuStatus == menuStatusIcons) {
		return updateGrid(m);
	} else if (menuStatus == menuStatusFolderChanged) {
		drawDisk("Loading folder");
		gfxFlip();
		reloadMenu(m);
		gotoFirstIcon(m);
		setMenuStatus(menuStatusIcons);
		return false;
	} else {
		updateGrid(m);
		return false;
	}
}

void initEmptyMenuEntry(menuEntry_s* me) {
	if(!me)return;

	me->name[0]=0x00;
	me->description[0]=0x00;
	me->executablePath[0]=0x00;
	me->author[0]=0x00;
	me->arg[0]=0x00;
	me->bannerImagePath[0]=0x00;

	initDescriptor(&me->descriptor);

	me->next=NULL;
}

void initMenuEntry(menuEntry_s* me, char* execPath, char* name, char* description, char* author, u8* iconData) {
	if(!me)return;

	initEmptyMenuEntry(me);

	strncpy(me->executablePath, execPath, ENTRY_PATHLENGTH);
	strncpy(me->name, name, ENTRY_NAMELENGTH);
	strncpy(me->description, description, ENTRY_DESCLENGTH);
	strncpy(me->author, author, ENTRY_AUTHORLENGTH);
	if(iconData)memcpy(me->iconData, iconData, ENTRY_ICONSIZE);

	initDescriptor(&me->descriptor);

	me->title_id = 0;
}

#include "pngloader.h"
#include "logText.h"

void loadBannerImage(menuEntry_s * me) {
	strcpy(bannerImagePath, me->bannerImagePath);
	drawBannerImage = false;

	bool success = read_png_file(bannerImagePath);

	if (success) {
		if (pngWidth > 400 || pngHeight > 222) {
			logText("App banners must be 400x222 pixels maximum");
		}
		else {
			bannerImageWidth = pngWidth;
			bannerImageHeight = pngHeight;

			u8 * out = process_png_file();

			if (out) {
				drawBannerImage = true;
				bannerHasAlpha = (bytesPerPixel == 4);
				memcpy(&bannerImage, out, sizeof(bannerImage));
			}
		}
	}
}

int drawMenuEntry(menuEntry_s* me, gfxScreen_t screen, bool selected, menu_s *m, int pageYOffset, int pageXOffset, bool drawTopScreenInfo) {
	if(!me) {
		return 0;
	}

	const int totalWidth=selected?ENTRY_WIDTH_SELECTED:ENTRY_WIDTH;

	int * coords = coordsForMenuEntry(me->row, me->col, m);
	int x = coords[0];
	int y = coords[1];

	x += pageXOffset;
	y += pageYOffset;

	me->iconX = x;
	me->iconY = y;
	me->iconW = ENTRY_ICON_WIDTH;
	me->iconH = ENTRY_ICON_HEIGHT;

	//Draw backgrounds for the icons if they haven't been drawn by drawMenu
	//(i.e. if showing app backgrounds for empty slots has been disabled)

	bool entryIsCart = false;

	// Draw the icon on the bottom screen (if necessary)

	if (me != &gamecardMenuEntry && !me->isTitleEntry) {
		if (me->isRegionFreeEntry) {
			entryIsCart = true;

			if (selected) {
				if (themeImageExists(themeImageCartBackgroundSelected)) {
					drawThemeImage(themeImageCartBackgroundSelected, screen, x+3, y+4);
				} else {
					gfxDrawSpriteAlphaBlend(screen, GFX_LEFT, (u8*)cartBackgroundSelected, 59, 59, x+3, y+4);
				}
			} else {
				if (themeImageExists(themeImageCartBackground)) {
					drawThemeImage(themeImageCartBackground, screen, x+3, y+4);
				} else {
					gfxDrawSpriteAlphaBlendFade(screen, GFX_LEFT, (u8*)cartBackground, 59, 59, x+3, y+4, translucencyAppBackgrounds);
				}
			}
		} else {
			if (selected) {
				if (themeImageExists(themeImageAppBackgroundSelected)) {
					drawThemeImage(themeImageAppBackgroundSelected, screen, x+3, y+4);
				} else {
					gfxDrawSpriteAlphaBlend(screen, GFX_LEFT, appBackgroundSelected, 56, 56, x+3, y+4);
				}
			} else {
				if (themeImageExists(themeImageAppBackground)) {
					drawThemeImage(themeImageAppBackground, screen, x+3, y+4);
				} else {
					gfxDrawSpriteAlphaBlendFade(screen, GFX_LEFT, appBackground, 56, 56, x+3, y+4, translucencyAppBackgrounds);
				}

			}
		}

		//Highlight the icon if it is selected


		//This is where the masked image will be put (rounding the corners)
		u8 transparentIcon[48*48*4];

		//Mask whichever icon is going to be drawn (either the game card icon or a homebrew app's icon
		if (me->isRegionFreeEntry && regionFreeGamecardIn) {
			MAGFXApplyAlphaMask(gamecardMenuEntry.iconData, (u8*)appiconalphamask_bin, transparentIcon, 48, 48, false);
		} else {
			MAGFXApplyAlphaMask(me->iconData, (u8*)appiconalphamask_bin, transparentIcon, 48, 48, false);
		}

		//Draw the icon on the bottom screen
		gfxDrawSpriteAlphaBlend(screen, GFX_LEFT, transparentIcon, ENTRY_ICON_WIDTH, ENTRY_ICON_HEIGHT, x+7, y+8);

		if (me->drawFirstLetterOfName) {
			char firstLetter[2];
			memcpy(firstLetter, &me->name, 1);
			firstLetter[1] = '\0';

			rgbColour * light = lightTextColour();

			MADrawText(screen, GFX_LEFT, x+9, y+27, firstLetter, &MAFontRobotoRegular16, light->r, light->g, light->b);
		}
	}

	/*
	 Draw a tick on the icon if it is ticked (e.g. for settings)
	 */
	if (me->showTick != NULL && *(me->showTick)) {
		gfxDrawSpriteAlphaBlend(screen, GFX_LEFT, (u8*)tick_bin, 48, 48, x+7, y+8);
	}

	if (entryIsCart) {
		if (selected) {
			if (themeImageExists(themeImageCartOverlaySelected)) {
				drawThemeImage(themeImageCartOverlaySelected, screen, x, y);
			}
		} else {
			if (themeImageExists(themeImageCartOverlay)) {
				drawThemeImage(themeImageCartOverlay, screen, x, y);
			}
		}
	} else {
		if (selected) {
			if (themeImageExists(themeImageAppOverlaySelected)) {
				drawThemeImage(themeImageAppOverlaySelected, screen, x, y);
			}
		} else {
			if (themeImageExists(themeImageAppOverlay)) {
				drawThemeImage(themeImageAppOverlay, screen, x, y);
			}
		}
	}

	/*
	 Top screen stuff for the selected item
	 */
	if (selected && drawTopScreenInfo) {
		rgbColour * dark = darkTextColour();

		int top = 240-50-18;

		int xPos = 30;

		int displayIconX = (240-ENTRY_ICON_HEIGHT)/2;
		int displayIconY = 0;


		/*
			Draw app banner image (if it exists
		*/
		if (me->hasBanner) {
			if (strcmp(bannerImagePath, me->bannerImagePath) != 0) {
				loadBannerImage(me);
			}

			if (drawBannerImage) {
				int bannerImageY = 0;
				int bannerImageX = 0;

				if (me->bannerIsFullScreen) {
					bannerImageY = (400/2) - (bannerImageWidth/2);
					bannerImageX = (222/2) - (bannerImageHeight/2);
				}

				if (bannerHasAlpha) {
					gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, (u8*)bannerImage, bannerImageHeight, bannerImageWidth, bannerImageX, bannerImageY);
				} else {
					gfxDrawSprite(GFX_TOP, GFX_LEFT, (u8*)bannerImage, bannerImageHeight, bannerImageWidth, bannerImageX, bannerImageY);
				}
			}
		}

		// Draw app icon image if no banner was drawn
		if (!me->hasBanner || !drawBannerImage) {
			/*
			 Draw the shadow
			 */
			gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, (u8*)appshadow_bin, 27, 161, displayIconX-16, displayIconY-3, translucencyAppShadow);

			displayIconY += (156-ENTRY_ICON_HEIGHT)/2;

			//Draw the app icon
			u8 transparentIcon[48*48*4];

			if (me->isRegionFreeEntry && regionFreeGamecardIn) {
				MAGFXApplyAlphaMask(gamecardMenuEntry.iconData, (u8*)appiconalphamask_bin, transparentIcon, 48, 48, false);
			} else {
				MAGFXApplyAlphaMask(me->iconData, (u8*)appiconalphamask_bin, transparentIcon, 48, 48, false);
			}

			gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, transparentIcon, ENTRY_ICON_WIDTH, ENTRY_ICON_HEIGHT, displayIconX, displayIconY);
		}

		xPos += (ENTRY_ICON_WIDTH*2)+30;

		int yAdjust = 20;

		int maximumTextWidth = 400-xPos-20;

		fullScreenBannerVisible = (me->hasBanner && me->bannerIsFullScreen);

		if (!me->hasBanner || !me->bannerIsFullScreen) {
			/*
			 Draw the app title
			 */

			rgbColour *titleColour = titleTextColour();

			int maxLines = (me->drawFullTitle) ? 100 : 1;
			int numTitleLines;

			if (me == &gamecardMenuEntry || (me->isRegionFreeEntry && regionFreeGamecardIn)) {
				numTitleLines = MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, gamecardMenuEntry.name, &MAFontRobotoRegular14, titleColour->r, titleColour->g, titleColour->b, maximumTextWidth, maxLines);
			} else {
				numTitleLines = MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, me->name, &MAFontRobotoRegular14, titleColour->r, titleColour->g, titleColour->b, maximumTextWidth, maxLines);
			}

			/*
			 Draw the app author
			 */

			if (me->isRegionFreeEntry && regionFreeGamecardIn) {
				yAdjust += (numTitleLines * 25);
				MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, gamecardMenuEntry.author, &MAFontRobotoRegular12, dark->r, dark->g, dark->b, maximumTextWidth, 1);
			} else if (strlen(me->author) > 0) {
				yAdjust += (numTitleLines * 25);
				MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, me->author, &MAFontRobotoRegular12, dark->r, dark->g, dark->b, maximumTextWidth, 1);
			}

			yAdjust += 25;

			/*
			 Draw the app description
			 */

			int descriptionMaxWidth = 400-xPos-20;
			int descriptionMaxLines = 4;

			if (me->isRegionFreeEntry && regionFreeGamecardIn) {
				MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, gamecardMenuEntry.description, &MAFontRobotoRegular10, dark->r, dark->g, dark->b, descriptionMaxWidth, descriptionMaxLines);
			} else {
				if (me->isRegionFreeEntry) {
					char * insert = "Insert a game cart to play region free";
					MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, insert, &MAFontRobotoRegular10, dark->r, dark->g, dark->b, descriptionMaxWidth, descriptionMaxLines);
				} else {
					MADrawTextWrap(GFX_TOP, GFX_LEFT, top-yAdjust, xPos, me->description, &MAFontRobotoRegular10, dark->r, dark->g, dark->b, descriptionMaxWidth, descriptionMaxLines);
				}
			}
		}
	}

	return totalWidth;
}
