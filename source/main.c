#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <3ds.h>
#include <sys/stat.h>
#include <malloc.h>

#include "gfx.h"
#include "menu.h"
#include "water.h"
#include "statusbar.h"
#include "filesystem.h"
#include "netloader.h"
#include "regionfree.h"
#include "boot.h"
#include "titles.h"
#include "mmap.h"
#include "folders.h"
#include "logodefault_bin.h"
#include "logocompact_bin.h"
#include "logoclassic_bin.h"
#include "MAFontRobotoRegular.h"
#include "alert.h"
#include "logText.h"
#include "colours.h"

#include "config.h"

#include "MAGFX.h"

#include "help.h"
#include "touchblock.h"
#include "folders.h"
#include "themegfx.h"
#include "version.h"
#include "sound.h"

bool sdmcCurrent = 0;
u64 nextSdCheck = 0;

bool die = false;
bool dieImmediately = false;
bool menuret = false;
bool showRebootMenu = false;
bool startRebootProcess = false;

char HansArg[ENTRY_PATHLENGTH+1];

u32 menuret_enabled = 0;

static enum {
	HBMENU_DEFAULT,
	HBMENU_TITLESELECT,
	HBMENU_TITLETARGET_ERROR,
	HBMENU_NETLOADER_ACTIVE,
	HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2,
	HBMENU_NETLOADER_ERROR,
} hbmenu_state = HBMENU_DEFAULT;

int debugValues[100];

extern void closeReboot() {
	showRebootMenu = false;
}

extern void doReboot() {
	if (!menuret_enabled) {
		APT_HardwareResetAsync();
	} else {
		menuret = true;
		die = true;
	}
}

void launchSVDTFromTitleMenu() {
	menuEntry_s* me = getMenuEntry(&titleMenu, titleMenu.selectedEntry);

	if (me) {
		if (me->title_id) {
			if (me->title_id > 0) {
				titleInfo_s target_title; 
				createTitleInfoFromTitleID(me->title_id, me->mediatype, &target_title);
				bootSetTargetTitle(target_title);

				die = true;
			}
		}
	}
}

void exitServices() {
	if (titlemenuIsUpdating) {
		//Stop the title menu loading process, causing the thread to exit
		cancelTitleLoading();

		//Wait a little bit (two seconds) longer to allow the thread to actually terminate
		svcSleepThread(2000000000ULL);
	}

	if (titleThreadNeedsRelease) {
		releaseTitleThread();
	}

	// cleanup whatever we have to cleanup
	audio_stop();
	csndExit();

	freeThemeImages();
	netloader_exit();
	titlesExit();
	ptmuExit();
	acExit();
	hidExit();
	gfxExit();
	closeSDArchive();
	exitFilesystem();
	aptExit();
	srvExit();
}

void launchTitleFromMenu(menu_s* m) {
	menuEntry_s* me = getMenuEntry(m, m->selectedEntry);

	if (me) {
		if (me->title_id) {
			if (me->title_id > 0) {
				titleInfo_s target_title; 
				createTitleInfoFromTitleID(me->title_id, me->mediatype, &target_title);
				bootSetTargetTitle(target_title);

					exitServices();
					dieImmediately = true;

					if (hansTitleBoot) {
						bootApp("/gridlauncher/hans/hans.3dsx", NULL, HansArg);
					} else {
						regionFreeRun2(me->title_id & 0xffffffff, (me->title_id >> 32) & 0xffffffff, me->mediatype, 0x1);
					}
			}
		} else {
			die = true;
		}
	}
}

void putTitleMenu(char * barTitle) {
	drawGrid(&titleMenu);
	drawBottomStatusBar(barTitle);
}

#include "progresswheel.h"

void handleMenuSelection();

void renderFrame() {
	// background stuff

	rgbColour * bgc = backgroundColour();

	gfxFillColor(GFX_BOTTOM, GFX_LEFT, (u8[]){bgc->r, bgc->g, bgc->b});
	gfxFillColor(GFX_TOP, GFX_LEFT, (u8[]){bgc->r, bgc->g, bgc->b});

	//Wallpaper
	if (themeImageExists(themeImageTopWallpaperInfo) && ((menuStatus == menuStatusHelp && showingHelpDetails) || (menuStatus == menuStatusIcons && fullScreenBannerVisible) || menuStatus == menuStatusColourAdjust || menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom || menuStatus == menuStatusHansMissingError || menuStatus == menuStatusBootOptions || hbmenu_state == HBMENU_NETLOADER_ACTIVE || hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2 || hbmenu_state == HBMENU_TITLETARGET_ERROR || showRebootMenu || !sdmcCurrent)) {
		drawThemeImage(themeImageTopWallpaperInfo, GFX_TOP, 0, 0);
	} else if (themeImageExists(themeImageTopWallpaper)) {
		drawThemeImage(themeImageTopWallpaper, GFX_TOP, 0, 0);
	}

	if (themeImageExists(themeImageBottomWallpaperNonGrid) && ((menuStatus == menuStatusHelp && showingHelpDetails) || menuStatus == menuStatusColourAdjust || menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom || menuStatus == menuStatusHansMissingError || menuStatus == menuStatusBootOptions || hbmenu_state == HBMENU_NETLOADER_ACTIVE || hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2 || hbmenu_state == HBMENU_TITLETARGET_ERROR || showRebootMenu || !sdmcCurrent)) {
		drawThemeImage(themeImageBottomWallpaperNonGrid, GFX_BOTTOM, 0, 0);
	} else if (themeImageExists(themeImageBottomWallpaper)) {
		drawThemeImage(themeImageBottomWallpaper, GFX_BOTTOM, 0, 0);
	}

	drawWater();


		//menu stuff
		if (showRebootMenu) {
			//about to reboot
			char buttonTitles[3][32];

			bool drawRebootAlert = true;

			if (startRebootProcess) {
				if (themeImageExists(themeImageTopWallpaperReboot) && themeImageExists(themeImageBottomWallpaperReboot)) {
					drawThemeImage(themeImageTopWallpaperReboot, GFX_TOP, 0, 0);
					drawThemeImage(themeImageBottomWallpaperReboot, GFX_BOTTOM, 0, 0);
					drawRebootAlert = false;
				}

				if (!menuret_enabled) {
					strcpy(buttonTitles[0], "Rebooting...");
				} else {
					strcpy(buttonTitles[0], "Exiting...");
				}
			} else {
				if (!menuret_enabled) {
					strcpy(buttonTitles[0], "Reboot");
				} else {
					strcpy(buttonTitles[0], "Exit");
				}
			}

			strcpy(buttonTitles[1], "Power off");
			strcpy(buttonTitles[2], "Back");

			int alertResult = -1;

			if (drawRebootAlert) {
				if (!menuret_enabled) {
					alertResult = drawAlert("Power options", "Reboot:\nGo back to the system Home Menu\n\nPower off:\nShut down your 3DS", NULL, 3, buttonTitles);
				} else {
					alertResult = drawAlert("Power options", "Exit:\nGo back to the system Home Menu\n\nPower off:\nShut down your 3DS", NULL, 3, buttonTitles);
				}
			}


			if (startRebootProcess) {
				doReboot();
			} else {
				if (alertResult == 0) {
					startRebootProcess = true;
				} else if (alertResult == 1) {
					ptmSysmInit(); 
					PTMSYSM_ShutdownAsync(0); 
					ptmSysmExit();
				} else if (alertResult == 2 || alertResult == alertButtonKeyB) {
					closeReboot();
				}
			}

		} else if(!sdmcCurrent) {
			//no SD
			drawAlert("No SD detected", "It looks like your 3DS doesn't have an SD inserted into it. Please insert an SD card for optimal homebrew launcher performance!", NULL, 0, NULL);
		} else if(sdmcCurrent<0) {
			//SD error
			drawAlert("SD Error", "Something unexpected happened when trying to mount your SD card. Try taking it out and putting it back in. If that doesn't work, please try again with another SD card.", NULL, 0, NULL);

		} else if(hbmenu_state == HBMENU_NETLOADER_ACTIVE) {
			char bof[256];
			u32 ip = gethostid();
			sprintf(bof,
				"NetLoader Active - waiting for 3dslink connection\n\nIP: %lu.%lu.%lu.%lu, Port: %d\n\nB : Cancel\n",
				ip & 0xFF, (ip>>8)&0xFF, (ip>>16)&0xFF, (ip>>24)&0xFF, NETLOADER_PORT);

			drawAlert("NetLoader", bof, NULL, 0, NULL);
		} else if(hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2) {
			drawAlert("NetLoader", "The NetLoader is currently unavailable. :( This might be normal and fixable. Try and enable it ?\n\nA : Yes\nB : No\n", NULL, 0, NULL);
		} else if(hbmenu_state == HBMENU_TITLESELECT) {

			if (updateGrid(&titleMenu)) {
				launchSVDTFromTitleMenu();
			} else {
				putTitleMenu("Select Title");

				if (titlemenuIsUpdating) {
					drawProgressWheel(GFX_BOTTOM, GFX_LEFT, 0, 0);
				}
			}
		} else if(hbmenu_state == HBMENU_TITLETARGET_ERROR) {
			drawAlert("Missing target title", "The application you are trying to run requested a specific target title.\nPlease make sure you have that title !\n\nB : Back\n", NULL, 0, NULL);
		} else if(hbmenu_state == HBMENU_NETLOADER_ERROR) {
			netloader_draw_error();
		} else {
			//got SD

			if (menuStatus == menuStatusHomeMenuApps) {
				putTitleMenu("Select Title to Launch");

				if (titlemenuIsUpdating) {
					drawProgressWheel(GFX_BOTTOM, GFX_LEFT, 0, 0);
				}
			} else if (menuStatus == menuStatusFolders) {
				drawGrid(&foldersMenu);
				drawBottomStatusBar("Select folder");
			} else if (menuStatus == menuStatusTitleFiltering) {
				putTitleMenu("Tap titles to show or hide them");

				if (titlemenuIsUpdating) {
					drawProgressWheel(GFX_BOTTOM, GFX_LEFT, 0, 0);
				}
			} else if (menuStatus == menuStatusSettings) {
				drawGrid(&settingsMenu);
				drawBottomStatusBar("Settings");
			} else if (menuStatus == menuStatusGridSettings) {
				drawGrid(&gridSettingsMenu);
				drawBottomStatusBar("Grid settings");
			} else if (menuStatus == menuStatusWaterSettings) {
				drawGrid(&waterMenu);
				drawBottomStatusBar("Water settings");
			} else if (menuStatus == menuStatusThemeSettings) {
				drawGrid(&themeSettingsMenu);
				drawBottomStatusBar("Theme settings");
			} else if (menuStatus == menuStatusThemeSelect) {
				drawGrid(&themesMenu);
				drawBottomStatusBar("Select theme");
			} else if (menuStatus == menuStatusColourSettings) {
				drawGrid(&colourSelectMenu);
				drawBottomStatusBar("Colours");
			} else if (menuStatus == menuStatusHelp) {
				drawHelp();
			} else if (menuStatus == menuStatusColourAdjust) {
				drawColourAdjuster();
				drawBottomStatusBar("Colour adjustment");
			} else if (menuStatus == menuStatusTranslucencyTop) {
				drawTranslucencyAdjust(GFX_TOP);
				drawBottomStatusBar("Top screen translucency");
			} else if (menuStatus == menuStatusTranslucencyBottom) {
				drawTranslucencyAdjust(GFX_BOTTOM);
				drawBottomStatusBar("Bottom screen translucency");
			} else if (menuStatus == menuStatusPanelSettingsTop) {
				drawPanelTranslucencyAdjust(GFX_TOP);
				drawBottomStatusBar("Top panel settings");
			} else if (menuStatus == menuStatusPanelSettingsBottom) {
				drawPanelTranslucencyAdjust(GFX_BOTTOM);
				drawBottomStatusBar("Bottom panel settings");
			} else if (menuStatus == menuStatusHansMissingError) {
				char buttonTitles[3][32];
				strcpy(buttonTitles[0], "OK");
				alertSelectedButton = 0;
				int selectedButton = drawAlert("Hans isn't here...", "Please copy hans.3dsx and hans.xml in /gridlauncher/hans/ on your 3ds SD card.", NULL, 1, buttonTitles);
				if (selectedButton == 0 || selectedButton == alertButtonKeyB) {
					setMenuStatus(menuStatusSettings);
				}
			} else if (menuStatus == menuStatusBootOptions) {
				char buttonTitles[3][32];
				strcpy(buttonTitles[0], "HANS");
				strcpy(buttonTitles[1], "Region4");
				strcpy(buttonTitles[2], "Cancel");

				menuEntry_s *me = getMenuEntry(bootOptionsMenu, bootOptionsMenu->selectedEntry);
				char text[128];
				sprintf(text, "Please choose how you want to boot the app.\n\nTitle ID:\n%llu", me->title_id);

				int selectedButton = drawAlert("Select boot method", text, NULL, 3, buttonTitles);

				if (selectedButton == 0) {
					hansTitleBoot = true;

					if(bootOptionsMenu == &titleMenu) {
						//HANS boot from title menu
						launchTitleFromMenu(bootOptionsMenu);
					} else {
						//HANS boot from main menu
						handleMenuSelection();
					}
				} else if (selectedButton == 1) {
					hansTitleBoot = false;

					if(bootOptionsMenu == &titleMenu) {
						//R4 boot from title menu
						launchTitleFromMenu(bootOptionsMenu);
					} else {
						//R4 boot from main menu
						handleMenuSelection();
					}
				} else if (selectedButton == 2 || selectedButton == alertButtonKeyB) {
					if(bootOptionsMenu == &titleMenu) {
						setMenuStatus(menuStatusHomeMenuApps);
					} else {
						setMenuStatus(menuStatusIcons);
					}
				}
			} else if (menuStatus == menuStatusHBAppOptions) {
				char buttonTitles[2][32];
				strcpy(buttonTitles[0], "Delete app");
				strcpy(buttonTitles[1], "Cancel");

				menuEntry_s *me = getMenuEntry(bootOptionsMenu, bootOptionsMenu->selectedEntry);

				int selectedButton = drawAlert("Homebrew App Options", me->name, NULL, 2, buttonTitles);

				if (selectedButton == 0) {
					alertSelectedButton = 1;
					setMenuStatus(menuStatusHBAppDeleteConfirmation);
				}
				else if (selectedButton == 1 || selectedButton == alertButtonKeyB) {
					setMenuStatus(menuStatusIcons);
				}
			} else if (menuStatus == menuStatusHBAppDeleteConfirmation) {
				char buttonTitles[2][32];
				strcpy(buttonTitles[0], "Delete");
				strcpy(buttonTitles[1], "Cancel");

				menuEntry_s *me = getMenuEntry(bootOptionsMenu, bootOptionsMenu->selectedEntry);

				char deletePath[128];

				if (me->isWithinContainingFolder) {
					int i, l=-1; for(i=0; me->executablePath[i]; i++) if(me->executablePath[i]=='/')l=i;
					strncpy(deletePath, me->executablePath, l);
				} else {
					strcpy(deletePath, me->executablePath);
				}

				int selectedButton = drawAlert("Are you sure you want to delete:", deletePath, NULL, 2, buttonTitles);

				if (selectedButton == 0) {
					//Delete the app
					setMenuStatus(menuStatusIcons);
				} else if (selectedButton == 1 || selectedButton == alertButtonKeyB) {
					alertSelectedButton = 0;
					setMenuStatus(menuStatusHBAppOptions);
				}
			} else {
				drawMenu(&menu);
			}
		}

	u8 * logoImage = NULL;
	int logoWidth = 0;
	int logoHeight = 0;

	if (logoType == logoTypeDefault) {
		logoImage = (u8*)logodefault_bin;
		logoWidth = 54;
		logoHeight = 161;
	} else if (logoType == logoTypeCompact) {
		logoImage = (u8*)logocompact_bin;
		logoWidth = 35;
		logoHeight = 203;
	} else if (logoType == logoTypeClassic) {
		logoImage = (u8*)logoclassic_bin;
		logoWidth = 25;
		logoHeight = 223;
	}

	if (logoImage) {
		gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, logoImage, logoWidth, logoHeight, 0, 400-logoHeight);
	}

	drawStatusBar(wifiStatus, charging, batteryLevel);
}

// handled in main
// doing it in main is preferred because execution ends in launching another 3dsx
void __appInit()
{
}

// same
void __appExit()
{
}

void showTitleMenu(titleBrowser_s * aTitleBrowser, menu_s * aTitleMenu, int newMenuStatus, bool filter, bool forceHideRegionFree, bool setFilterTicks) {
	if (!titleMenuInitialLoadDone && !titlemenuIsUpdating) {
		updateTitleMenu(&titleBrowser, &titleMenu, "Loading titles", filter, forceHideRegionFree, setFilterTicks);
	}

	setMenuStatus(newMenuStatus);
}

void showSVDTTitleSelect() {
	showTitleMenu(&titleBrowser, &titleMenu, menuStatusTitleBrowser, true, false, false);
	hbmenu_state = HBMENU_TITLESELECT;

	if (animatedGrids) {
		startTransition(transitionDirectionUp, menu.pagePosition, &menu);
	}
}

void showHomeMenuTitleSelect() {
	checkReturnToGrid(&titleMenu);

	showTitleMenu(&titleBrowser, &titleMenu, menuStatusHomeMenuApps, true, false, false);

	if (animatedGrids) {
		startTransition(transitionDirectionUp, menu.pagePosition, &menu);
	}
}

void showFilterTitleSelect() {
	titleMenuInitialLoadDone = false;
	showTitleMenu(&titleBrowser, &titleMenu, menuStatusTitleFiltering, false, true, true);
	if (animatedGrids) {
		startTransition(transitionDirectionDown, settingsMenu.pagePosition, &settingsMenu);
	}
}

void closeTitleBrowser() {
	setMenuStatus(menuStatusIcons);
	checkReturnToGrid(&menu);
	hbmenu_state = HBMENU_DEFAULT;

	if (animatedGrids) {
		startTransition(transitionDirectionDown, titleMenu.pagePosition, &titleMenu);
	}
}

bool gamecardWasIn;
bool gamecardStatusChanged;

void handleMenuSelection() {

	menuEntry_s* me = getMenuEntry(&menu, menu.selectedEntry);

	if(me && !strcmp(me->executablePath, REGIONFREE_PATH) && regionFreeAvailable && !netloader_boot) {
		regionFreeUpdate();

		if (regionFreeGamecardIn) {
			die = true;
		}
	} else {
		// if appropriate, look for specified titles in list
		if(me->descriptor.numTargetTitles) {

			// go through target title list in order so that first ones on list have priority
			int i;
			titleInfo_s* ret = NULL;
			for(i=0; i<me->descriptor.numTargetTitles; i++) {
				ret = findTitleBrowser(&titleBrowser, me->descriptor.targetTitles[i].mediatype, me->descriptor.targetTitles[i].tid);
				if(ret)break;
			}

			if(ret) {
				bootSetTargetTitle(*ret);
				die = true;
				return;
			}

			// if we get here, we aint found shit
			// if appropriate, let user select target title
			if(me->descriptor.selectTargetProcess) {
				showSVDTTitleSelect();
			} else hbmenu_state = HBMENU_TITLETARGET_ERROR;
		} else {
			if(me->descriptor.selectTargetProcess) {
				showSVDTTitleSelect();
			} else {
				die = true;
			}
		}


	}
}

void enterNetloader() {
	if(netloader_activate() == 0) hbmenu_state = HBMENU_NETLOADER_ACTIVE;
	else if(isNinjhax2()) hbmenu_state = HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2;
}

int main(int argc, char *argv[]) {
	srvInit();
	aptInit();
	gfxInit(GSP_BGR8_OES, GSP_BGR8_OES, false);
	hidInit();

	hidScanInput();

	u8* framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	u8* framebuf_bot = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	memset(framebuf_top, 0, 400 * 240 * 3); //clear the screen to black
	memset(framebuf_bot, 0, 320 * 240 * 3); //ensures no graphical glitching shows.

	gfxFlip();

	initFilesystem();

	openSDArchive();

	// Moved this here as rand() is used for choosing a random theme
	srand(svcGetSystemTick());

	int startMs = 0;
	int endMs = 0;
	int delayMs = 0;
	unsigned long long int delayNs = 0;

	if (!netloaderShortcut) {
		Result r = csndInit(); //start Audio Lib
		audioActive = (r == 0);

		int bootAttempts = getConfigIntForKey("bootAttempts", 0, configTypeMain);

		if (!audioActive) {
			if (bootAttempts < 5) {
				bootAttempts++;
				setConfigInt("bootAttempts", bootAttempts, configTypeMain);
				saveConfigWithType(configTypeMain);

				menuEntry_s* me = malloc(sizeof(menuEntry_s));
				initMenuEntry(me, "/boot.3dsx", "launcher", "", "", NULL);
				scanMenuEntry(me);
				exitServices();
				regionFreeExit();
				return bootApp(me->executablePath, &me->descriptor.executableMetadata, me->arg);
			}
		}

		if (bootAttempts > 0) {
			setConfigInt("bootAttempts", 0, configTypeMain);
			saveConfigWithType(configTypeMain);
		}

		randomTheme = getConfigBoolForKey("randomTheme", false, configTypeMain);

		if (randomTheme) {
			randomiseTheme();
		} else {
			audio_stop();

			loadSplashImages();

			if (themeImageExists(themeImageSplashTop)) {
				drawThemeImage(themeImageSplashTop, GFX_TOP, 0, 0);
			}

			if (themeImageExists(themeImageSplashBottom)) {
				drawThemeImage(themeImageSplashBottom, GFX_BOTTOM, 0, 0);
			}

			gfxFlip();

			startMs = osGetTime();
			playBootSound();

			initThemeImages();
			initThemeSounds();
			initColours();
		}

		mkdir(rootPath, 777);
		mkdir(themesPath, 777);
		mkdir(foldersPath, 777);
		mkdir(defaultThemePath, 777);
		mkdir(titleBannersPath, 777);

		initWater();
	}

	startMs = osGetTime();

	acInit();
	ptmuInit();
	titlesInit();
	regionFreeInit();
	netloader_init();

	osSetSpeedupEnable(true);

	// offset potential issues caused by homebrew that just ran 
	APT_SetAppCpuTimeLimit(0);

	if (!netloaderShortcut) {
		initMenu(&menu);
		initTitleBrowser(&titleBrowser, NULL);
	}

	u8 sdmcPrevious = 0;
	FSUSER_IsSdmcDetected(&sdmcCurrent);
	if(sdmcCurrent == 1 && !netloaderShortcut){
		scanHomebrewDirectory(&menu, currentFolder());
	}

	sdmcPrevious = sdmcCurrent;
	nextSdCheck = osGetTime()+250;

	//Negate this to force an update of the cart title id on first boot
	gamecardWasIn = !regionFreeGamecardIn;

	int frameRate = 60;
	int frameMs = 1000 / frameRate;

	if (netloaderShortcut) {
		enterNetloader();
	} else {
		char * glInfo = (char*)malloc(1024);

		if (argc > 0) {
			sprintf(glInfo, "%s|%s", argv[0], currentversion);
		} else {
			sprintf(glInfo, "sdmc:/boot.3dsx|%s", currentversion);
		}

		logTextP(glInfo, "/gridlauncher/glinfo.txt", false);
		free(glInfo);

		waitForSoundToFinishPlaying(&themeSoundBoot);

		startBGM();
	}

	Handle kill=0;
	if(srvGetServiceHandle(&kill, "hb:kill")==0)menuret_enabled = 1;

	while(aptMainLoop()) {
		if (die || dieImmediately) {
			break;
		}

		startMs = osGetTime();

		if (gamecardWasIn != regionFreeGamecardIn) {
			gamecardWasIn = regionFreeGamecardIn;

			u64 currentTitleID = 0;

			if (regionFreeGamecardIn) {
				u32 num = 1;
				u64* tmp = (u64*)malloc(sizeof(u64) * num);
				u8 mediatype = 2;
				u32 num32;
				Result ret = AM_GetTitleList(&num32, mediatype, num, tmp);

				if (ret) {
					logText("Error getting title");
				} else {
					currentTitleID = tmp[0];
				}
			}

			menuEntry_s *me = getMenuEntry(&menu, 0);
			if (me) {
				me->title_id = currentTitleID;
				me->mediatype = 2;

				me->bannerImagePath[0] = '\0';

				if (currentTitleID > 0) {
					addTitleBannerImagePathToMenuEntry(me, currentTitleID);
				}
			}

			if (titleMenuInitialLoadDone && titleMenu.numEntries > 0) {
				menuEntry_s * gcme = getMenuEntry(&titleMenu, 0);
				gcme->hidden = !regionFreeGamecardIn;
				gcme->title_id = currentTitleID;
				gcme->mediatype = 2;
				updateMenuIconPositions(&titleMenu);
				gotoFirstIcon(&titleMenu);

				gcme->bannerImagePath[0] = '\0';

				if (currentTitleID > 0) {
					addTitleBannerImagePathToMenuEntry(gcme, currentTitleID);
				}
			}
		}

		if (killTitleBrowser) {
			killTitleBrowser = false;
			closeTitleBrowser();
		}

		if (menuStatus == menuStatusOpenHomeMenuApps) {
			showHomeMenuTitleSelect();
		} else if (menuStatus == menuStatusOpenTitleFiltering) {
			showFilterTitleSelect();
		}

		if (nextSdCheck < osGetTime()) {
			regionFreeUpdate();

			FSUSER_IsSdmcDetected(&sdmcCurrent);

			if(sdmcCurrent == 1 && (sdmcPrevious == 0 || sdmcPrevious < 0)) {
				closeSDArchive();
				openSDArchive();
				scanHomebrewDirectory(&menu, currentFolder());
			} else if(sdmcCurrent < 1 && sdmcPrevious == 1) {
				clearMenuEntries(&menu);
			}
			sdmcPrevious = sdmcCurrent;
			nextSdCheck = osGetTime()+250;
		}

		ACU_GetWifiStatus(&wifiStatus);
		PTMU_GetBatteryLevel(&batteryLevel);
		PTMU_GetBatteryChargeState(&charging);
		hidScanInput();

		updateWater();

		if(hbmenu_state == HBMENU_NETLOADER_ACTIVE){
			if(hidKeysDown()&KEY_B) {
				netloader_deactivate();
				hbmenu_state = HBMENU_DEFAULT;
			} else {
				int rc = netloader_loop();
				if(rc > 0) {
					netloader_boot = true;
					break;
				} else if(rc < 0) {
					hbmenu_state = HBMENU_NETLOADER_ERROR;
				}
			}
		} else if(hbmenu_state == HBMENU_NETLOADER_UNAVAILABLE_NINJHAX2) {
			if(hidKeysDown()&KEY_B) {
				hbmenu_state = HBMENU_DEFAULT;
			} else if(hidKeysDown()&KEY_A) {
				if(isNinjhax2()) {
					// basically just relaunch boot.3dsx w/ scanning in hopes of getting netloader capabilities
					static char hbmenuPath[] = "/boot.3dsx";
					netloadedPath = hbmenuPath; // fine since it's static
					netloader_boot = true;
					break;
				}
			}
		} else if(hbmenu_state == HBMENU_TITLETARGET_ERROR) {
			if(hidKeysDown()&KEY_B){
				hbmenu_state = HBMENU_DEFAULT;
			}
		} else if(hbmenu_state == HBMENU_TITLESELECT) {
			if(hidKeysDown()&KEY_A && titleBrowser.selected) {
				launchSVDTFromTitleMenu();
			} else if(hidKeysDown()&KEY_B) {
				closeTitleBrowser();
			}
		} else if(hbmenu_state == HBMENU_NETLOADER_ERROR) {
			if(hidKeysDown()&KEY_B)
				hbmenu_state = HBMENU_DEFAULT;
		} else if (!showRebootMenu) {

			if(hidKeysDown()&KEY_START) {
				alertSelectedButton = 0;
				showRebootMenu = true;
			}
			if(hidKeysDown()&KEY_Y) {
				enterNetloader();
			}

			if (menuStatus == menuStatusHomeMenuApps) {
				if (updateGrid(&titleMenu)) {
					launchTitleFromMenu(&titleMenu);
				}
			} else if (menuStatus == menuStatusTitleFiltering) {
				if (updateGrid(&titleMenu)) {
					menuEntry_s* me = getMenuEntry(&titleMenu, titleMenu.selectedEntry);
					toggleTitleFilter(me, &titleMenu);
				}
			} else if (menuStatus == menuStatusFolders) {
				if (updateGrid(&foldersMenu)) {
					menuEntry_s* me = getMenuEntry(&foldersMenu, foldersMenu.selectedEntry);

					if (me->showTick == NULL) {
						setFolder(me->name);
					}
				}
			} else if (menuStatus == menuStatusSettings) {
				if (updateGrid(&settingsMenu)) {
					handleSettingsMenuSelection(&settingsMenu);

					if (menuStatus == menuStatusSoftwareUpdate) {
						break;
					}
				}
			} else if (menuStatus == menuStatusGridSettings) {
				if (updateGrid(&gridSettingsMenu)) {
					handleSettingsMenuSelection(&gridSettingsMenu);
				}
			} else if (menuStatus == menuStatusWaterSettings) {
				if (updateGrid(&waterMenu)) {
					handleSettingsMenuSelection(&waterMenu);
				}
			} else if (menuStatus == menuStatusThemeSettings) {
				if (updateGrid(&themeSettingsMenu)) {
					handleSettingsMenuSelection(&themeSettingsMenu);
				}
			} else if (menuStatus == menuStatusThemeSelect) {
				if (updateGrid(&themesMenu)) {
					if (themesMenu.selectedEntry == 0) {
						if (!randomTheme) {
							randomTheme = true;
							updateMenuTicks(&themesMenu, NULL, true);
						}
					} else {
						menuEntry_s* me = getMenuEntry(&themesMenu, themesMenu.selectedEntry);

						if (me->showTick == NULL) {
							randomTheme = false;
							setTheme(me->executablePath);
							char * currentThemeName = getConfigStringForKey("currentTheme", "Default", configTypeMain);
							updateMenuTicks(&themesMenu, currentThemeName, true);
						}
					}

				}
			} else if (menuStatus == menuStatusHelp) {
				updateHelp();
			} else if (menuStatus == menuStatusColourAdjust || menuStatus == menuStatusPanelSettingsTop || menuStatus == menuStatusPanelSettingsBottom || menuStatus == menuStatusTranslucencyTop || menuStatus == menuStatusTranslucencyBottom) {
				handleNonGridToolbarNavigation();
			} else if (menuStatus == menuStatusColourSettings) {
				if (updateGrid(&colourSelectMenu)) {
					handleColourSelectMenuSelection();
				}
			} else if (menuStatus == menuStatusHansMissingError ||
					menuStatus == menuStatusBootOptions ||
					menuStatus == menuStatusHBAppOptions ||
					menuStatus == menuStatusHBAppDeleteConfirmation) {

			} else if(updateMenu(&menu)) {
				handleMenuSelection();
			}
		}

		renderFrame();
		gfxFlip();

		endMs = osGetTime();
		delayMs = frameMs - (endMs - startMs);
		delayNs = delayMs * 1000000;
		svcSleepThread(delayNs);
	}

	if (dieImmediately) {
		return 0;
	}

	if (menuret) {
		exitServices();
		svcSignalEvent(kill);
		svcExitProcess();
	}

	menuEntry_s* me;

	if(netloader_boot) {
		me = malloc(sizeof(menuEntry_s));
		initMenuEntry(me, netloadedPath, "netloaded app", "", "", NULL);
	} else if (menuStatus == menuStatusSoftwareUpdate) {
		me = malloc(sizeof(menuEntry_s));
		initMenuEntry(me, "/gridlauncher/update/mglupdate.3dsx", "updater", "", "", NULL);
	} else {
		me = getMenuEntry(&menu, menu.selectedEntry);
	}

	scanMenuEntry(me);

	if (touchThreadNeedsRelease) {
		releaseTouchThread();
	}

	if(!strcmp(me->executablePath, REGIONFREE_PATH) && regionFreeAvailable && !netloader_boot) {
		if (hansTitleBoot) {
			titleInfo_s target_title; 
			createTitleInfoFromTitleID(me->title_id, me->mediatype, &target_title);
			bootSetTargetTitle(target_title); 

			regionFreeExit();
			exitServices();
			return bootApp("/gridlauncher/hans/hans.3dsx", NULL, HansArg);
		} else {
			exitServices();
			return regionFreeRun2(0x00000000, 0x00000000, 0x2, 0x1);
		}
	}

	regionFreeExit();
	exitServices();
	return bootApp(me->executablePath, &me->descriptor.executableMetadata, me->arg);
}
