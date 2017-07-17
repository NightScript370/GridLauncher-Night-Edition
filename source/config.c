#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "regionfree.h"
#include "statusbar.h"
#include "water.h"
#include "colours.h"
#include "MAGFX.h"
#include "filesystem.h"
#include "sound.h"
#include "boot.h"
#include "help.h"

#include "settingsIconAlphaSort_bin.h"
#include "settingsIconWaterAnimated_bin.h"
#include "settingsIconWaterVisible_bin.h"
#include "settingsIconThirdRow_bin.h"
#include "settingsIconClock24_bin.h"
#include "settingsIconAppBackgrounds_bin.h"
#include "settingsIconColours_bin.h"
#include "settingsIconWrapScrolling_bin.h"
#include "settingsIconTranslucencyTop_bin.h"
#include "settingsIconTranslucencyBottom_bin.h"
#include "settingsIconLogo_bin.h"
#include "settingsIconPanelTop_bin.h"
#include "settingsIconPanelBottom_bin.h"
#include "settingsIconPreloadTitles_bin.h"
#include "settingsIconTheme_bin.h"
#include "settingsIconDPadControls_bin.h"
#include "settingsIconWater_bin.h"
#include "settingsIconGrid_bin.h"
#include "settingsIconRandomTheme_bin.h"
#include "settingsIconShowTitleID_bin.h"
#include "helpIconIgnoredTitles_bin.h"
#include "settingsIconAnimation_bin.h"
#include "settingsIcon3DSFolder_bin.h"
#include "settingsIconUpdate_bin.h"
#include "settingsIconWaitForSounds_bin.h"
#include "settingsIconHansTitleBooting_bin.h"
#include "settingsIconShowDate_bin.h"

#include "titles.h"
#include "themegfx.h"
#include "folders.h"
#include "logText.h"

menu_s settingsMenu;
bool settingsMenuNeedsInit = true;

menu_s themesMenu;
menu_s waterMenu;
menu_s themeSettingsMenu;
menu_s gridSettingsMenu;

bool showRegionFree = true;
bool sortAlpha = false;
bool showAppBackgrounds = true;
bool wrapScrolling = true;

bool randomTheme = false;

bool themesLoaded = false;

int logoType = logoTypeDefault;

void loadConfigWithType(int configType);

void setTheme(char * themeName) {
	audio_stop();

	//Save the selected theme in main config
	setConfigString("currentTheme", themeName, configTypeMain);

	//Reload theme config
	loadConfigWithType(configTypeTheme);

	//Reload theme variables for menu
	loadThemeConfig();

	loadSplashImages();

	if (themeImageExists(themeImageSplashTop)) {
		drawThemeImage(themeImageSplashTop, GFX_TOP, 0, 0);
	}
	if (themeImageExists(themeImageSplashBottom)) {
		drawThemeImage(themeImageSplashBottom, GFX_BOTTOM, 0, 0);
	}

	gfxFlip();

	playBootSound();

	//Reload theme images
	initThemeImages();

	//Load BGM
	initThemeSounds();

	//Re-initialise colours
	initColours();

	//Force reload of GUI elements
	statusbarNeedsUpdate = true;
	toolbarNeedsUpdate = true;
	alphaImagesDrawn = false;

	waitForSoundToFinishPlaying(&themeSoundBoot);

	startBGM();

	panelsDrawn = false;
	pageControlPanelsDrawn = false;
}

void addThemeToList(char * fullPath, menuEntry_s * me, char * smdhName, int folderPathLen) {
	me->hidden = false;
	me->isTitleEntry = false;
	me->isRegionFreeEntry = false;
	me->isShortcut = false;

	char smdhPath[strlen(fullPath) + strlen(smdhName) + 1];
	strcpy(smdhPath, fullPath);
	strcat(smdhPath, smdhName);

	bool iconNeedsToBeGenerated = true;

	strcpy(me->name, "");
	strcpy(me->description, "");
	strcpy(me->author, "");

	if(fileExists(smdhPath, &sdmcArchive)) {
		static smdh_s tmpSmdh;
		int ret = loadFile(smdhPath, &tmpSmdh, &sdmcArchive, sizeof(smdh_s));

		if (!ret) {
			ret = extractSmdhData(&tmpSmdh, me->name, me->description, me->author, me->iconData);
			if (!ret) {
				iconNeedsToBeGenerated = false;
			}
		}
	}

	if (iconNeedsToBeGenerated) {
		memcpy(me->iconData, settingsIconTheme_bin, 48*48*3);
	}

	if (strlen(me->name) == 0) {
		if (strcmp(fullPath, "/3ds/") == 0) {
			strcpy(me->name, "3ds");
		}
		else {
			strcpy(me->name, fullPath+folderPathLen);
		}
	}

	if (strlen(me->description) == 0) {
		strcpy(me->description, "");
	}

	if (strlen(me->author) == 0) {
		strcpy(me->author, "");
	}

	me->drawFirstLetterOfName = iconNeedsToBeGenerated;
	me->drawFullTitle = true;

	strcpy(me->executablePath, fullPath+folderPathLen);

	int i, l=-1; for(i=0; fullPath[i]; i++) if(fullPath[i]=='.') l=i;

	char bannerPath[128];
	strcpy(bannerPath, "");
	strncat(bannerPath, &fullPath[0], l);
	strcat(bannerPath, "");

	addBannerPathToMenuEntry(me->bannerImagePath, bannerPath, "theme", &me->bannerIsFullScreen, &me->hasBanner);

	addMenuEntryCopy(&themesMenu, me);
//	themesMenu.numEntries = themesMenu.numEntries + 1;
}

void buildThemesList() {
	if (themesLoaded) {
		return;
	}

	themesLoaded = true;

	directoryContents * contents = contentsOfDirectoryAtPath(themesPath, true);

	themesMenu.entries=NULL;
	themesMenu.numEntries=0;
	themesMenu.selectedEntry=0;
	themesMenu.scrollLocation=0;
	themesMenu.scrollVelocity=0;
	themesMenu.scrollBarSize=0;
	themesMenu.scrollBarPos=0;
	themesMenu.scrollTarget=0;
	themesMenu.atEquilibrium=false;

	menuEntry_s randomEntry;
	strcpy(randomEntry.name, "Random theme");
	strcpy(randomEntry.description, "A random theme will be selected each time the launcher boots. Excludes the default theme.");
	strcpy(randomEntry.author, "");
	memcpy(randomEntry.iconData, settingsIconRandomTheme_bin, sizeof(randomEntry.iconData));
	randomEntry.hidden = false;
	randomEntry.isTitleEntry = false;
	randomEntry.isShortcut = false;
	randomEntry.isRegionFreeEntry = false;
	randomEntry.drawFirstLetterOfName = false;
	addMenuEntryCopy(&themesMenu, &randomEntry);

	char * smdhName = "/theme.smdh";
	int folderPathLen = strlen(themesPath);

	int i;
	for (i=0; i<contents->numPaths; i++) {
		char * fullPath = contents->paths[i];
		static menuEntry_s me;
		addThemeToList(fullPath, &me, smdhName, folderPathLen);
	}

	updateMenuIconPositions(&themesMenu);
	free(contents);
}

void randomiseTheme() {
	buildThemesList();
	int minimum_number = 2;
	int max_number = themesMenu.numEntries - 1;
	int r = rand() % (max_number + 1 - minimum_number) + minimum_number;
	menuEntry_s * randomEntry = getMenuEntry(&themesMenu, r);
	setTheme(randomEntry->executablePath);
}

void addSettingsMenuEntry(char * name, char * description, u8 * icon, bool * showTick, menu_s *m,  void (*callback)(), void *callbackObject1, void *callbackObject2) {
	static menuEntry_s settingsMenuEntry;

	settingsMenuEntry.hidden = false;
	settingsMenuEntry.isTitleEntry = false;

	strcpy(settingsMenuEntry.name, name);
	strcpy(settingsMenuEntry.description, description);
	if (icon) {
		int i;
		for (i=0; i<ENTRY_ICONSIZE; i++) {
			settingsMenuEntry.iconData[i] = icon[i];
		}
	} else {
		memset(settingsMenuEntry.iconData, 0, ENTRY_ICONSIZE);
	}
	settingsMenuEntry.showTick = showTick;
	settingsMenuEntry.callback = callback;
	settingsMenuEntry.callbackObject1 = callbackObject1;
	settingsMenuEntry.callbackObject2 = callbackObject2;

	strcpy(settingsMenuEntry.author, "");

	addMenuEntryCopy(m, &settingsMenuEntry);
}

void settingsToggleBool(bool * aBool) {
	*aBool = !(*aBool);
}

void settingsToggleRegionFree() {
	showRegionFree = !showRegionFree;
	menuRegionFreeToggled();
}

void settingsToggleShow3DS() {
	show3DSFolder = !show3DSFolder;
	if (foldersMenu.numEntries > 0) {
		foldersMenu.entries[0].hidden = !show3DSFolder;
		updateMenuIconPositions(&foldersMenu);
	}
}

void settingsToggleSortAlpha() {
	sortAlpha = !sortAlpha;
	menuReloadRequired = true;
}

void settingsShowColours() {
	if (colourSelectMenuNeedsInit) {
		initColourSelectMenu();
	}

	updateMenuIconPositions(&colourSelectMenu);
	gotoFirstIcon(&colourSelectMenu);
	setMenuStatus(menuStatusColourSettings);

	if (animatedGrids) {
		startTransition(transitionDirectionDown, themeSettingsMenu.pagePosition, &themeSettingsMenu);
	}
}

void settingsSetMenuStatus(int * status);

bool waterMenuNeedsInit = true;

void initWaterMenu() {
	if (!waterMenuNeedsInit) {
		return;
	}

	waterMenuNeedsInit = false;

	waterMenu.entries=NULL;
	waterMenu.numEntries=0;
	waterMenu.selectedEntry=0;
	waterMenu.scrollLocation=0;
	waterMenu.scrollVelocity=0;
	waterMenu.scrollBarSize=0;
	waterMenu.scrollBarPos=0;
	waterMenu.scrollTarget=0;
	waterMenu.atEquilibrium=false;

	addSettingsMenuEntry("Top screen water", "Toggle the visibility of the water on the top screen", (u8*)settingsIconWaterVisible_bin, &waterEnabled, &waterMenu, &settingsToggleBool, &waterEnabled, NULL);

	addSettingsMenuEntry("Animated water", "Toggle the animated water effect", (u8*)settingsIconWaterAnimated_bin, &waterAnimated, &waterMenu, &settingsToggleBool, &waterAnimated, NULL);
}

void cycleRows() {
	int maxRows = 3;
	int minRows = 1;

	totalRows++;

	if (totalRows > maxRows) {
		totalRows = minRows;
	}

	updateMenuIconPositions(&colourSelectMenu);
	updateMenuIconPositions(&foldersMenu);
	updateMenuIconPositions(&gridSettingsMenu);
	updateMenuIconPositions(&settingsMenu);
	updateMenuIconPositions(&themeSettingsMenu);
	updateMenuIconPositions(&themesMenu);
	updateMenuIconPositions(&titleMenu);
	updateMenuIconPositions(&waterMenu);
}

bool gridSettingsMenuNeedsInit = true;

void initGridSettingsMenu() {
	if (!gridSettingsMenuNeedsInit) {
		return;
	}

	gridSettingsMenuNeedsInit = false;

	gridSettingsMenu.entries=NULL;
	gridSettingsMenu.numEntries=0;
	gridSettingsMenu.selectedEntry=0;
	gridSettingsMenu.scrollLocation=0;
	gridSettingsMenu.scrollVelocity=0;
	gridSettingsMenu.scrollBarSize=0;
	gridSettingsMenu.scrollBarPos=0;
	gridSettingsMenu.scrollTarget=0;
	gridSettingsMenu.atEquilibrium=false;

	addSettingsMenuEntry("Rows", "Select the number of rows to be shown on the launcher grids", (u8*)settingsIconThirdRow_bin, NULL, &gridSettingsMenu, &cycleRows, NULL, NULL);

	addSettingsMenuEntry("Empty icon backgrounds", "Choose whether empty icon positions should show a background", (u8*)settingsIconAppBackgrounds_bin, &showAppBackgrounds, &gridSettingsMenu, &settingsToggleBool, &showAppBackgrounds, NULL);

	addSettingsMenuEntry("Wraparound scrolling", "Choose whether page scrolling should wrap around at the beginning and of the grid", (u8*)settingsIconWrapScrolling_bin, &wrapScrolling, &gridSettingsMenu, &settingsToggleBool, &wrapScrolling, NULL);

	addSettingsMenuEntry("D-Pad Navigation", "Allows the use of the corner icons on the bottom screen using the D-Pad", (u8*)settingsIconDPadControls_bin, &dPadNavigation, &gridSettingsMenu, &settingsToggleBool, &dPadNavigation, NULL);

	addSettingsMenuEntry("Grid animation", "Choose whether moving between pages in grids should be animated", (u8*)settingsIconAnimation_bin, &animatedGrids, &gridSettingsMenu, &settingsToggleBool, &animatedGrids, NULL);

	addSettingsMenuEntry("Show 3DS folder", "Choose whether the default /3ds/ folder should be shown in the folders list", (u8*)settingsIcon3DSFolder_bin, &show3DSFolder, &gridSettingsMenu, &settingsToggleShow3DS, NULL, NULL);


}

void cycleLogoType() {
	logoType++;
	if (logoType > logoTypeNone)
		logoType = logoTypeDefault;
}

bool themeSettingsMenuNeedsInit = true;

void initThemeSettingsMenu() {
	if (!themeSettingsMenuNeedsInit) {
		return;
	}

	themeSettingsMenuNeedsInit = false;

	themeSettingsMenu.entries=NULL;
	themeSettingsMenu.numEntries=0;
	themeSettingsMenu.selectedEntry=0;
	themeSettingsMenu.scrollLocation=0;
	themeSettingsMenu.scrollVelocity=0;
	themeSettingsMenu.scrollBarSize=0;
	themeSettingsMenu.scrollBarPos=0;
	themeSettingsMenu.scrollTarget=0;
	themeSettingsMenu.atEquilibrium=false;

	addSettingsMenuEntry("Water", "Various settings for the water on the top screen", (u8*)settingsIconWater_bin, false, &themeSettingsMenu, &settingsSetMenuStatus, &menuStatusWaterSettings, NULL);

	addSettingsMenuEntry("Theme colours", "Adjust the colour scheme of the launcher", (u8*)settingsIconColours_bin, NULL, &themeSettingsMenu, &settingsShowColours, NULL, NULL);

	addSettingsMenuEntry("Translucency (top screen)", "Draw the user interface with translucency - useful if using custom wallpapers", (u8*)settingsIconTranslucencyTop_bin, false, &themeSettingsMenu, &settingsSetMenuStatus, &menuStatusTranslucencyTop, NULL);

	addSettingsMenuEntry("Translucency (bottom screen)", "Draw the user interface with translucency - useful if using custom wallpapers", (u8*)settingsIconTranslucencyBottom_bin, false, &themeSettingsMenu, &settingsSetMenuStatus, &menuStatusTranslucencyBottom, NULL);

	addSettingsMenuEntry("Top panels", "Show panels behind text to make it easier to read against wallpaper", (u8*)settingsIconPanelTop_bin, NULL, &themeSettingsMenu, &settingsSetMenuStatus, &menuStatusPanelSettingsTop, NULL);
	addSettingsMenuEntry("Bottom panels", "Show panels behind text to make it easier to read against wallpaper", (u8*)settingsIconPanelBottom_bin, NULL, &themeSettingsMenu, &settingsSetMenuStatus, &menuStatusPanelSettingsBottom, NULL);

	addSettingsMenuEntry("Logo", "Show a logo at the bottom of the screen. Tap to cycle between Gridlauncher default, Gridlauncher compact, HBL classic and None.", (u8*)settingsIconLogo_bin, NULL, &themeSettingsMenu, &cycleLogoType, NULL, NULL);

	addSettingsMenuEntry("Theme", "Select which theme to use in the launcher", (u8*)settingsIconTheme_bin, NULL, &themeSettingsMenu, &settingsSetMenuStatus, &menuStatusThemeSelect, NULL);

	addSettingsMenuEntry("Random theme on wake", "Randomizes the theme when the 3DS has been closed for an extended period of time.", (u8*)settingsIconRandomTheme_bin, &randomiseThemeOnWake, &themeSettingsMenu, &settingsToggleBool, &randomiseThemeOnWake, NULL);

	addSettingsMenuEntry("Wait for sounds", "Wait until the boot sound has finished playing before showing the menu, and the select sound before starting an app", (u8*)settingsIconWaitForSounds_bin, &waitForSounds, &themeSettingsMenu, &settingsToggleBool, &waitForSounds, NULL);
}

void settingsSetMenuStatus(int * status) {
	if (*status == menuStatusThemeSelect) {
		buildThemesList();
		char * currentThemeName = getConfigStringForKey("currentTheme", "Default", configTypeMain);
		updateMenuTicks(&themesMenu, currentThemeName, true);
		checkReturnToGrid(&themesMenu);
		updateMenuIconPositions(&themesMenu);

		if (animatedGrids) {
			startTransition(transitionDirectionDown, themeSettingsMenu.pagePosition, &themeSettingsMenu);
		}
	} else if (*status == menuStatusWaterSettings) {
		initWaterMenu();
		updateMenuIconPositions(&waterMenu);
		gotoFirstIcon(&waterMenu);

		if (animatedGrids) {
			startTransition(transitionDirectionDown, settingsMenu.pagePosition, &settingsMenu);
		}
	} else if (*status == menuStatusThemeSettings) {
		initThemeSettingsMenu();
		updateMenuIconPositions(&themeSettingsMenu);
		gotoFirstIcon(&themeSettingsMenu);

		if (animatedGrids) {
			startTransition(transitionDirectionDown, settingsMenu.pagePosition, &settingsMenu);
		}
	} else if (*status == menuStatusGridSettings) {
		initGridSettingsMenu();
		updateMenuIconPositions(&gridSettingsMenu);
		gotoFirstIcon(&gridSettingsMenu);

		if (animatedGrids) {
			startTransition(transitionDirectionDown, settingsMenu.pagePosition, &settingsMenu);
		}
	} else if (*status == menuStatusOpenTitleFiltering) {
		if (titleLoadPaused) {
			cancelTitleLoading();
		}
	}

	setMenuStatus(*status);
}

void startUpdate() {
	setMenuStatus(menuStatusSoftwareUpdate);
}

void toggleHansBoot() {
	if (fileExists("/gridlauncher/hans/hans.3dsx", &sdmcArchive) && fileExists("/gridlauncher/hans/hans.xml", &sdmcArchive)) {
		hansTitleBoot = !hansTitleBoot;
	} else {
		setMenuStatus(menuStatusHansMissingError);
	}
}

void initConfigMenu() {
	settingsMenuNeedsInit = false;

	settingsMenu.entries=NULL;
	settingsMenu.numEntries=0;
	settingsMenu.selectedEntry=0;
	settingsMenu.scrollLocation=0;
	settingsMenu.scrollVelocity=0;
	settingsMenu.scrollBarSize=0;
	settingsMenu.scrollBarPos=0;
	settingsMenu.scrollTarget=0;
	settingsMenu.atEquilibrium=false;

	if (regionFreeAvailable) {
		addSettingsMenuEntry("Region free loader", "Toggle the region free loader in the main menu grid", (u8*)regionfreeEntry.iconData, &showRegionFree, &settingsMenu, &settingsToggleRegionFree, NULL, NULL);
	}

	addSettingsMenuEntry("Grids", "Various settings to control how the launcher's grids function", (u8*)settingsIconGrid_bin, NULL, &settingsMenu, &settingsSetMenuStatus, &menuStatusGridSettings, NULL);

	addSettingsMenuEntry("App sorting", "Toggle alphabetic sorting in the main menu grid", (u8*)settingsIconAlphaSort_bin, &sortAlpha, &settingsMenu, &settingsToggleSortAlpha, NULL, NULL);

	addSettingsMenuEntry("24 hour clock", "Displays the clock in 24 hour format", (u8*)settingsIconClock24_bin, &clock24, &settingsMenu, &settingsToggleBool, &clock24, NULL);

	addSettingsMenuEntry("Show date", "Displays the date on the status bar", (u8*)settingsIconShowDate_bin, &showDate, &settingsMenu, &settingsToggleBool, &showDate, NULL);

	addSettingsMenuEntry("Background title loading", "Load title menu entries in the background. Loading begins when the title menu is opened. May be unstable with many titles", (u8*)settingsIconPreloadTitles_bin, &preloadTitles, &settingsMenu, &settingsToggleBool, &preloadTitles, NULL);

	addSettingsMenuEntry("Theme settings", "Configure the theme for the launcher", (u8*)settingsIconTheme_bin, false, &settingsMenu, &settingsSetMenuStatus, &menuStatusThemeSettings, NULL);

	addSettingsMenuEntry("Title filtering", "Show or hide system titles from the title launcher and save manager", (u8*)helpIconIgnoredTitles_bin, false, &settingsMenu, &settingsSetMenuStatus, &menuStatusOpenTitleFiltering, NULL);

	if (fileExists("/gridlauncher/update/mglupdate.3dsx", &sdmcArchive) && fileExists("/gridlauncher/update/index.lua", &sdmcArchive)) {
		addSettingsMenuEntry("Software update", "Update Gridlauncher to the latest version", (u8*)settingsIconUpdate_bin, false, &settingsMenu, &startUpdate, NULL, NULL);

		menuEntry_s *updateEntry = getMenuEntry(&settingsMenu, settingsMenu.numEntries-1);

		strcpy(updateEntry->author, "By ihaveamac on GBATemp");
	}

	addSettingsMenuEntry("HANS title booting", "Boot titles using HANS rather than Region4. Press SELECT with an app highlighted to manually override this setting.", (u8*)settingsIconHansTitleBooting_bin, &hansTitleBoot, &settingsMenu, &toggleHansBoot, NULL, NULL);
}

void handleSettingsMenuSelection(menu_s *m) {
	int selectedEntry = m->selectedEntry;
	menuEntry_s * selectedMenuEntry = getMenuEntry(m, selectedEntry);
	(selectedMenuEntry->callback)(selectedMenuEntry->callbackObject1);
}

typedef struct {
	char keys[64][64];
	char values[64][64];
	int numConfigEntries;
} configData;

configData mainData;
configData themeData;

bool configInitialLoad = false;

char * configPathForType(int configType) {
	if (configType == configTypeMain) {
		char * path = malloc(strlen(configFilePath)+1);
		strcpy(path, configFilePath);
		return path;
	} else if (configType == configTypeTheme) {
		char * themePath = currentThemePath();
		char * filename = malloc(128);
		sprintf(filename, "%shbltheme.cfg", themePath);
		free(themePath);
		return filename;
	} else {
		return NULL;
	}
}

configData * configDataForType(int configType) {
	if (configType == configTypeMain) {
		return &mainData;
	} else if (configType == configTypeTheme) {
		return &themeData;
	} else {
		return NULL;
	}
}

void loadConfigWithType(int configType) {
	configData *data = configDataForType(configType);
	if (!data) {
		return;
	}

	char * configPath = configPathForType(configType);
	if (configPath == NULL) {
		return;
	}

	FILE* fp = fopen( configPath, "r" );
	char configText[8192];
	int count = 0;

	if (fp != NULL) {
		fgets(configText, 8192, fp);

		char lines[64][64];

		char * split1;
		split1 = strtok(configText, "|");
		while (split1 != NULL) {
			strcpy(lines[count], split1);

			count++;
			split1 = strtok(NULL, "|");
		}

		int i;

		for (i=0; i<count; i++) {
			char * line = lines[i];
			char * split2;

			split2 = strtok(line, "=");

			if (strcmp(split2, "translucencyLevel") == 0) {
				count--;
			} else {
				strcpy(data->keys[i], split2);

				split2 = strtok(NULL, "=");
				strcpy(data->values[i], split2);
			}
		}
	}
	fclose(fp);

	data->numConfigEntries = count;

	free(configPath);
}

void loadConfig() {
	loadConfigWithType(configTypeMain);


	/*
	 This flag states that the initial load of the config has been completed.
	 It MUST go after loading the main config and before loading the theme config.
	 This is because of the following:
	 - The functions which retrieve data from the config check whether configInitialLoad is false
	 - If it is false, they call loadConfig() (this function)
	 - Loading the theme config requires retrieving the currentTheme setting from the main config
	 - If loadConfigWithType(configTypeTheme) is called while configInitialLoad is false, then
	   retrieving the value of currentTheme will trigger loadConfig() to be called again, and we'll
	   enter an infinite loop
	 - To prevent this, we load the main config (so we know that the value of currentTheme will be
	   available), then we set configInitialLoad to true to prevent further calls to loadConfig(),
	   and only then call loadConfigWithType(configTypeTheme).
	 */

	configInitialLoad = true;


	loadConfigWithType(configTypeTheme);
}

void saveConfigWithType(int configType) {
	//This doesn't actually need to log the type
	//It's only here because there's a weird bug with saving the config,
	//which adding a log seems to resolve
	logTextP("Saving config", "/gridlauncher/c.txt", false);

	configData *data = configDataForType(configType);
	if (!data) {
		return;
	}

	//Start size at zero bytes
	int size = 0;

	//For each config entry, add the size of the value, key and delimeters to the overall file size
	int i;
	for (i=0; i<data->numConfigEntries; i++) {
		size += strlen(data->keys[i]);
		size += strlen(data->values[i]);
		size += 2;
	}

	//Create a string big enough to accomodate all of the keys, values and delimeters
	char out[size];

	strcpy(out, "");

	//For each config entry, add its key, value and delimeters to the output string
	for (i=0; i<data->numConfigEntries; i++) {
		strncat(out, data->keys[i], strlen(data->keys[i]));
		strncat(out, "=", 1);
		strncat(out, data->values[i], strlen(data->values[i]));
		strncat(out, "|", 1);
	}

	char * configPath = configPathForType(configType);
	if (configPath == NULL) {
		return;
	}

	//Open the config file path for writing and save the output string to it
	FILE* fp = fopen( configPath, "w" );
	if (fp != NULL) {
		fputs(out, fp);
	}

	fclose(fp);
	free(configPath);
}

void saveConfig() {
	saveConfigWithType(configTypeMain);
	saveConfigWithType(configTypeTheme);
}

void setConfigString(char* key, char* value, int configType) {
	if (!configInitialLoad) {
		loadConfig();
	}

	configData *data = configDataForType(configType);
	if (!data) {
		return;
	}

	//Assume that the config entry does not currently exist
	int currentIndex = -1;

	//Loop through the existing entries
	int i;
	for (i=0; i<data->numConfigEntries; i++) {
		if (strcmp(key, data->keys[i]) == 0) {
			//If the key of the current entry matches the key being added, then the config entry already exists
			//Set current index to the index of the existing entry and break
			currentIndex = i;
			break;
		}
	}

	if (currentIndex > -1) { //If the entry already exists in the array, overwrite it
		memset(&(data->keys[currentIndex][0]), 0, 64);
		memset(&(data->values[currentIndex][0]), 0, 64);

		strcpy(data->keys[currentIndex], key);
		strcpy(data->values[currentIndex], value);
	} else { //If the entry does not yet exist in the array, add it
		strcpy(data->keys[data->numConfigEntries], key);
		strcpy(data->values[data->numConfigEntries], value);
		data->numConfigEntries = data->numConfigEntries + 1;
	}
}

void setConfigBool(char* key, bool value, int configType) {
	setConfigString(key, value ? "1" : "0", configType);
}

void setConfigInt(char* key, int value, int configType) {
	char sValue[sizeof(int)+1];
	sprintf(sValue, "%d", value);
	setConfigString(key, sValue, configType);
}

char * getConfigValueForKey(char * key, int configType) {
	if (!configInitialLoad) {
		loadConfig();
	}

	configData *data = configDataForType(configType);
	if (!data) {
		return NULL;
	}

	int i;

	for (i=0; i<data->numConfigEntries; i++) {
		char * check = data->keys[i];
		if (strcmp(check, key) == 0) {
			return data->values[i];
		}
	}

	return NULL;
}

bool getConfigBoolForKey(char* key, bool defaultValue, int configType) {
	char * sValue = getConfigValueForKey(key, configType);

	if (sValue) {
		if (strcmp(sValue, "1") == 0) {
			return true;
		} else {
			return false;
		}
	} else {
		return defaultValue;
	}
}

int getConfigIntForKey(char* key, int defaultValue, int configType) {
	char * sValue = getConfigValueForKey(key, configType);

	if (sValue) {
		int iValue = atoi(sValue);
		return iValue;
	} else {
		return defaultValue;
	}
}

char * getConfigStringForKey(char* key, char* defaultValue, int configType) {
	char * sValue = getConfigValueForKey(key, configType);

	if (sValue) {
		return sValue;
	} else {
		return defaultValue;
	}
}
