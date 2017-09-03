#include <stdio.h>
#include <string.h>
#include <time.h>

#include "statusbar.h"
#include "gfx.h"

#include "wifi_full_bin.h"
#include "wifi_none_bin.h"
#include "battery_full_bin.h"
#include "battery_mid_high_bin.h"
#include "battery_mid_low_bin.h"
#include "battery_low_bin.h"
#include "battery_lowest_bin.h"
#include "battery_charging_bin.h"

#include "config.h"

#include "colours.h"
#include "MAFontRobotoRegular.h"

#include "MAGFX.h"
#include "menu.h"
#include "logText.h"

u32 wifiStatus = 0;
u8 batteryLevel = 5;
u8 charging = 0;

u8* batteryLevels[] = {
	(u8*)battery_lowest_bin,
	(u8*)battery_lowest_bin,
	(u8*)battery_low_bin,
	(u8*)battery_mid_low_bin,
	(u8*)battery_mid_high_bin,
	(u8*)battery_full_bin,
};

bool clock24 = false;
bool showDate = true;

u8 topBar[18*400*4];
bool statusbarNeedsUpdate = true;

u64 lastTimeInSeconds = 0; //Used for calculating if the lid has been shut.

void drawStatusBar(bool wifiStatus, bool charging, int batteryLevel) {
	if (statusbarNeedsUpdate) {
		statusbarNeedsUpdate = false;
		rgbColour * rgb = tintColour();
		MAGFXTranslucentRect(18, 400, rgb->r, rgb->g, rgb->b, translucencyTopBar, topBar);
	}

	gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, topBar, 18, 400, 240-18, 0);

	if(lastTimeInSeconds == 0) {
		lastTimeInSeconds = osGetTime() / 1000; //get on boot.
	}
	u64 timeInSeconds = osGetTime() / 1000;

	if(timeInSeconds - lastTimeInSeconds > 5) {
		randomTheme = getConfigBoolForKey("randomTheme", false, configTypeMain);

		if (randomTheme && randomiseThemeOnWake) {
			randomiseTheme();
		}
	}
	lastTimeInSeconds = timeInSeconds;

	u64 convert = ( (70*365+17) * 86400LLU );
	time_t now = timeInSeconds- convert;
	struct tm *ts = localtime(&now);
	char datestring[80];

	int hour = ts->tm_hour;

	char * ampm;

	if (clock24) ampm = "";
	else {
		ampm = (hour > 12) ? " pm" : " am";
		if (hour > 12) hour -= 12;
	}

	if (showDate) {
		int nMonth = ts->tm_mon;
		char * month;
		if (nMonth == 0) month = "Jan";
		else if (nMonth == 1) month = "Feb";
		else if (nMonth == 2) month = "Mar";
		else if (nMonth == 3) month = "Apr";
		else if (nMonth == 4) month = "May";
		else if (nMonth == 5) month = "Jun";
		else if (nMonth == 6) month = "Jul";
		else if (nMonth == 7) month = "Aug";
		else if (nMonth == 8) month = "Sep";
		else if (nMonth == 9) month = "Oct";
		else if (nMonth == 10) month = "Nov";
		else if (nMonth == 11) month = "Dec";
		else month = "";

		sprintf(datestring, "%02d:%02d:%02d%s, %02d %s %d", hour, ts->tm_min, ts->tm_sec, ampm, ts->tm_mday, month, ts->tm_year+1900);
	} else {
		sprintf(datestring, "%02d:%02d:%02d%s", hour, ts->tm_min, ts->tm_sec, ampm);
	}



	rgbColour *light = lightTextColour();

	int textWidth = MATextWidthInPixels(datestring, &MAFontRobotoRegular10);
	MADrawText(GFX_TOP, GFX_LEFT, 240-20, (400/2) - (textWidth/2), datestring, &MAFontRobotoRegular10, light->r, light->g, light->b);

	if(wifiStatus) {
		gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, (u8*)wifi_full_bin, 18, 20, 240 - 18, 2);
	} else {
		gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, (u8*)wifi_none_bin, 18, 20, 240 - 18, 2);
	}

	if(charging) {
		gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, (u8*)battery_charging_bin, 18, 27, 240 - 18, 400 - 29);
	} else {
		gfxDrawSpriteAlphaBlend(GFX_TOP, GFX_LEFT, batteryLevels[batteryLevel], 18, 27, 240 - 18, 400 - 29);
	}
}

