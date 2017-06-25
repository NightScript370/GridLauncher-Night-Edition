#include "help.h"
#include "menu.h"
#include "config.h"
#include "alert.h"
#include "string.h"
#include "water.h"
#include "settingsIconTheme_bin.h"
#include "helpIconAbout_bin.h"
#include "helpIconFolders_bin.h"
#include "helpIconIgnoredTitles_bin.h"
#include "touchblock.h"
#include "folders.h"
#include "version.h"
#include "stdio.h"
#include "stdlib.h"

bool showingHelpDetails = false;
bool skipHelpMenu = false;

menu_s helpMenu;
char * helpTitleText;
char * helpBodyText;

char * foldersHelpTitle = "Gridlauncher folders";
char * foldersHelpBody = "Add folders to /gridlauncher/folders/ on your SD card.\n\nThe normal 3ds folder must stay in the root directory of the SD card.";

char * themesHelpTitle = "Gridlauncher themes";
char * themesHelpBody = "Create theme folders in /gridlauncher/themes/ on your SD card.\n\nInside place wallpapertop.png (400x240 pixels) and wallpaperbottom.png (320x240).\n\nThen select the theme and adjust the colours and transparencies.";

int previousMenuStatus = 0;

void handleHelpBackButton() {
	if (skipHelpMenu || !showingHelpDetails) {
		hideWaves = false;
		setMenuStatus(previousMenuStatus);

		if (previousMenuStatus == menuStatusIcons) {
			checkReturnToGrid(&menu);

			if (animatedGrids) {
				startTransition(transitionDirectionUp, helpMenu.pagePosition, &helpMenu);
			}
		} else if (previousMenuStatus == menuStatusFolders) {
			checkReturnToGrid(&foldersMenu);
		} else if (previousMenuStatus == menuStatusThemeSelect) {
			checkReturnToGrid(&themesMenu);
		}
	} else {
		showingHelpDetails = false;
	}
}

void showHelpDetails(char * title, char * body) {
	startBlockingTouches();
	helpTitleText = title;
	helpBodyText = body;
	showingHelpDetails = true;
}

bool helpNeedsInit = true;

char helpButtonTitles[3][32];

void initHelpMenu() {
	helpNeedsInit = false;

	strcpy(helpButtonTitles[0], "Back");

	helpMenu.entries=NULL;
	helpMenu.numEntries=0;
	helpMenu.selectedEntry=0;
	helpMenu.scrollLocation=0;
	helpMenu.scrollVelocity=0;
	helpMenu.scrollBarSize=0;
	helpMenu.scrollBarPos=0;
	helpMenu.scrollTarget=0;
	helpMenu.atEquilibrium=false;

	char * titleTextFull = malloc(64);
	sprintf(titleTextFull, "Homebrew Grid Launcher %s", currentversion);

	addSettingsMenuEntry("About", "Details of the Homebrew Launcher and the Grid Launcher.", (u8*)helpIconAbout_bin, false, &helpMenu, &showHelpDetails, titleTextFull, "Original homebrew launcher by smealum.\nhttp://smealum.github.io/3ds/\n\nThis grid layout version by mashers.\nhttps://gbatemp.net/members/mashers.366855/");

	addSettingsMenuEntry("Folders", "How to set up subfolders in the launcher.", (u8*)helpIconFolders_bin, false, &helpMenu, &showHelpDetails, foldersHelpTitle, foldersHelpBody);

	addSettingsMenuEntry("Themes", "How to set up themes in the launcher.", (u8*)settingsIconTheme_bin, false, &helpMenu, &showHelpDetails, themesHelpTitle, themesHelpBody);

	addSettingsMenuEntry("Ignoring titles", "How to exclude titles from the title launcher.", (u8*)helpIconIgnoredTitles_bin, false, &helpMenu, &showHelpDetails, "Ignoring titles", "Select 'Title filtering' from the settings. Select a title and press A or touch it again to toggle its visibility in the title launcher and save manager.");
}

void showHelpCommon() {
	if (helpNeedsInit) {
		initHelpMenu();
	}

	hideWaves = true;
	previousMenuStatus = menuStatus;
	setMenuStatus(menuStatusHelp);
}

void showHelp() {
	showHelpCommon();
	skipHelpMenu = false;
	showingHelpDetails = false;
	updateMenuIconPositions(&helpMenu);
	gotoFirstIcon(&helpMenu);

	if (animatedGrids) {
		startTransition(transitionDirectionDown, menu.pagePosition, &menu);
	}
}

void showHelpWithForcedText(char * title, char * body) {
	showHelpCommon();
	skipHelpMenu = true;
	showingHelpDetails = true;
	helpTitleText = title;
	helpBodyText = body;
}

void drawHelp() {
	if (showingHelpDetails) {
		int alertResult = drawAlert(helpTitleText, helpBodyText, NULL, 1, helpButtonTitles);
		if (alertResult == 0 || alertResult == alertButtonKeyB) {
			handleHelpBackButton();
		}
	} else {
		drawGrid(&helpMenu);
		drawBottomStatusBar("Help");
	}
}

void handleHelpMenuSelection() {
	int selectedEntry = helpMenu.selectedEntry;
	menuEntry_s * selectedMenuEntry = getMenuEntry(&helpMenu, selectedEntry);
	(selectedMenuEntry->callback)(selectedMenuEntry->callbackObject1, selectedMenuEntry->callbackObject2);
}

void updateHelp() {
	if (!showingHelpDetails && updateGrid(&helpMenu)) {
		handleHelpMenuSelection();
	}
}
