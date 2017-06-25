#include "colours.h"
#include "stdio.h"
#include "string.h"
#include "config.h"
#include "stdlib.h"
#include "MAFontRobotoRegular.h"
#include "button.h"
#include "statusbar.h"
#include "water.h"
#include "MAGFX.h"
#include "touchblock.h"

#include "colourIconBackground_bin.h"
#include "colourIconDarkText_bin.h"
#include "colourIconInactive_bin.h"
#include "colourIconLightText_bin.h"
#include "colourIconTint_bin.h"
#include "colourIconTitleText_bin.h"
#include "colourIconWaterBottom_bin.h"
#include "colourIconWaterTop_bin.h"

menu_s colourSelectMenu;
bool colourSelectMenuNeedsInit = true;

rgbColour defaultColour = {0, 0, 0, 0, 0, 0, "defaultColour"};
rgbColour * settingsColour;

rgbColour colours[16];
int numColours;

bool coloursNeedInit = true;

void initColourSelectMenu() {
	colourSelectMenuNeedsInit = false;

	colourSelectMenu.entries=NULL;
	colourSelectMenu.numEntries=0;
	colourSelectMenu.selectedEntry=0;
	colourSelectMenu.scrollLocation=0;
	colourSelectMenu.scrollVelocity=0;
	colourSelectMenu.scrollBarSize=0;
	colourSelectMenu.scrollBarPos=0;
	colourSelectMenu.scrollTarget=0;
	colourSelectMenu.atEquilibrium=false;

	addSettingsMenuEntry("Tint colour", "Title bar and selected buttons.", (u8*)colourIconTint_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Inactive colour", "App backgrounds and unselected buttons.", (u8*)colourIconInactive_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Background colour", "Top and bottom screen background colours.", (u8*)colourIconBackground_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Water colour", "The colour of the main part of the water.", (u8*)colourIconWaterBottom_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Water border", "The colour of the border at the top of the water.", (u8*)colourIconWaterTop_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Dark text", "The colour of text usually drawn dark, such as app details.", (u8*)colourIconDarkText_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Light text", "The colour of text usually drawn light, such as bar text and button labels.", (u8*)colourIconLightText_bin, false, &colourSelectMenu, NULL, NULL, NULL);
	addSettingsMenuEntry("Title text", "The colour of text used as a heading, such as the names of apps.", (u8*)colourIconTitleText_bin, false, &colourSelectMenu, NULL, NULL, NULL);
}

void handleColourSelectMenuSelection() {
	int selectedEntry = colourSelectMenu.selectedEntry;

	if (selectedEntry == 0) {
		settingsColour = tintColour();
	} else if (selectedEntry == 1) {
		settingsColour = inactiveColour();
	} else if (selectedEntry == 2) {
		settingsColour = backgroundColour();
	} else if (selectedEntry == 3) {
		settingsColour = waterBottomColour();
	} else if (selectedEntry == 4) {
		settingsColour = waterTopColour();
	} else if (selectedEntry == 5) {
		settingsColour = darkTextColour();
	} else if (selectedEntry == 6) {
		settingsColour = lightTextColour();
	} else if (selectedEntry == 7) {
		settingsColour = titleTextColour();
	}

	setMenuStatus(menuStatusColourAdjust);
}

void loadCFGColour(rgbColour * colour) {
	char * key = colour->key;
	int len = strlen(key) + 2;

	char redKey[len];
	sprintf(redKey, "%sR", key);

	char greenKey[len];
	sprintf(greenKey, "%sG", key);

	char blueKey[len];
	sprintf(blueKey, "%sB", key);

	int configR = getConfigIntForKey(redKey, -1, configTypeTheme);
	int configG = getConfigIntForKey(greenKey, -1, configTypeTheme);
	int configB = getConfigIntForKey(blueKey, -1, configTypeTheme);

	if (configR > -1 && configG > -1 && configB > -1) {
		colour->r = configR;
		colour->g = configG;
		colour->b = configB;
	}
}

void addColour(int r, int g, int b, char * key, char * description) {
	rgbColour colour = {r, g, b, r, g, b};
	strcpy(colour.key, key);
	strcpy(colour.description, description);
	loadCFGColour(&colour);
	colours[numColours] = colour;
	numColours++;
}

void initColours() {
	numColours = 0;

	addColour(255, 255, 255, "bgColour", "Background colour");
	addColour(66,  185, 255, "tintColour", "Tint colour");
	addColour(225, 225, 225, "inactiveColour", "Inactive colour");
	addColour(66,  163, 255, "waterBottomColour", "Water  colour");
	addColour(66,  224, 255, "waterTopColour", "Water border colour");
	addColour(255, 255, 255, "lightTextColour", "Light text colour");
	addColour(0,   0,   0,   "darkTextColour", "Dark text colour");
	addColour(66,  185, 255, "titleTextColour", "Title text colour");
}

rgbColour * colourForKey(char * key) {
	if (coloursNeedInit) {
		initColours();
		coloursNeedInit = false;
	}

	int i;
	for (i=0; i<numColours; i++) {
		rgbColour *colour = &colours[i];
		if (strcmp(colour->key, key) == 0) {
			return colour;
		}
	}

	return &defaultColour;
}

rgbColour * tintColour() {
	return colourForKey("tintColour");
}

rgbColour * inactiveColour() {
	return colourForKey("inactiveColour");
}

rgbColour * backgroundColour() {
	return colourForKey("bgColour");
}

rgbColour * waterBottomColour() {
	return colourForKey("waterBottomColour");
}

rgbColour * waterTopColour() {
	return colourForKey("waterTopColour");
}

rgbColour * lightTextColour() {
	return colourForKey("lightTextColour");
}

rgbColour * darkTextColour() {
	return colourForKey("darkTextColour");
}

rgbColour * titleTextColour() {
	return colourForKey("titleTextColour");
}

void saveColour(rgbColour * colour, bool setToDefault) {
	char * key = colour->key;
	int len = strlen(key) + 2;

	char redKey[len];
	sprintf(redKey, "%sR", key);

	char greenKey[len];
	sprintf(greenKey, "%sG", key);

	char blueKey[len];
	sprintf(blueKey, "%sB", key);

	if (setToDefault) {
		colour->r = -1;
		colour->g = -1;
		colour->b = -1;
	}

	setConfigInt(redKey, colour->r, configTypeTheme);
	setConfigInt(greenKey, colour->g, configTypeTheme);
	setConfigInt(blueKey, colour->b, configTypeTheme);

	if (setToDefault) {
		colour->r = colour->defR;
		colour->g = colour->defG;
		colour->b = colour->defB;
	}
}

void checkRedrawGraphics(rgbColour * changedColour) {
	if (changedColour == tintColour()) {
		toolbarNeedsUpdate = true;
		statusbarNeedsUpdate = true;
		alphaImagesDrawn = false;
	} else if (changedColour == waterTopColour() || changedColour == waterBottomColour()) {
		staticWaterDrawn = false;
	} else if (changedColour == inactiveColour()) {
		alphaImagesDrawn = false;
	}
}

void addSlider(int xPos, char * shortText1, buttonList *aButtonList) {
	button aSlider;
	btnSetButtonType(&aSlider, btnButtonTypeSlider);
	aSlider.x = xPos;
	aSlider.y = 32;
	strcpy(aSlider.shortText1, shortText1);
	btnAddButtonToButtonList(&aSlider, aButtonList);
}

buttonList colourSliders;
button defaultButton;
bool colourButtonsMade = false;

int touchX, touchY, previousTouchX, previousTouchY;

void drawColourAdjuster() {
	if (!colourButtonsMade) {
		colourButtonsMade = true;

		addSlider(90, "Red", &colourSliders);
		addSlider(50, "Green", &colourSliders);
		addSlider(10, "Blue", &colourSliders);

		btnSetButtonType(&defaultButton, btnButtonTypeRect);
		defaultButton.x = 130;
		defaultButton.y = 32;
		strcpy(defaultButton.longText, "Default");
	}

	MAGFXDrawPanel(GFX_TOP, true);

	rgbColour *tint = titleTextColour();
	rgbColour *dark = darkTextColour();

	button * rSlider = colourSliders.buttons[0];
	button * gSlider = colourSliders.buttons[1];
	button * bSlider = colourSliders.buttons[2];

	rSlider->value = settingsColour->r;
	gSlider->value = settingsColour->g;
	bSlider->value = settingsColour->b;

	if (!touchesAreBlocked && (hidKeysDown()&KEY_TOUCH || hidKeysHeld()&KEY_TOUCH || hidKeysUp()&KEY_TOUCH)) {
		touchPosition touch;
		hidTouchRead(&touch);
		touchX = touch.px;
		touchY = touch.py;

		if (!(hidKeysUp()&KEY_TOUCH)) {
			previousTouchX = touchX;
			previousTouchY = touchY;
		}

		int previousR = rSlider->value;
		int previousG = gSlider->value;
		int previousB = bSlider->value;

		int r = updateSliderValue(touchX, touchY, rSlider);
		int g = updateSliderValue(touchX, touchY, gSlider);
		int b = updateSliderValue(touchX, touchY, bSlider);

		if (previousR != r || previousG != g || previousB != b) {
			// The colour has changed
			checkRedrawGraphics(settingsColour);
		}

		settingsColour->r = r;
		settingsColour->g = g;
		settingsColour->b = b;

		btnCheckHighlight(touchX, touchY, &defaultButton);

		if (hidKeysUp()&KEY_TOUCH) {
			if (btnTouchWithin(previousTouchX, previousTouchY, &defaultButton)) {
				defaultButton.highlighted = false;
				saveColour(settingsColour, true);
				checkRedrawGraphics(settingsColour);
			}
		}
	}

	btnDrawButtonList(&colourSliders);
	btnDrawButton(&defaultButton);

	MADrawText(GFX_TOP, GFX_LEFT, 160, 40, settingsColour->description, &MAFontRobotoRegular16, tint->r, tint->g, tint->b);

	MADrawTextWrap(GFX_TOP, GFX_LEFT, 130, 40, "Red:\nGreen:\nBlue:", &MAFontRobotoRegular12, dark->r, dark->g, dark->b, 500, 0);

	char rgbText[32];
	sprintf(rgbText, "%03d\n%03d\n%03d", settingsColour->r, settingsColour->g, settingsColour->b);
	MADrawTextWrap(GFX_TOP, GFX_LEFT, 130, 90, rgbText, &MAFontRobotoRegular12, dark->r, dark->g, dark->b, 500, 0);

	int previewRectSize = 100;
	int previewRectBorderSize = 5;

	u8 rectColour[3];

	rectColour[0] = inactiveColour()->r;
	rectColour[1] = inactiveColour()->g;
	rectColour[2] = inactiveColour()->b;
	gfxDrawRectangle(GFX_TOP, GFX_LEFT, rectColour, (160-previewRectSize) + MAFontRobotoRegular16.lineHeight, (400-40-previewRectSize), previewRectSize, previewRectSize);

	rectColour[0] = settingsColour->r;
	rectColour[1] = settingsColour->g;
	rectColour[2] = settingsColour->b;
	gfxDrawRectangle(GFX_TOP, GFX_LEFT, rectColour, (160-previewRectSize) + MAFontRobotoRegular16.lineHeight + previewRectBorderSize, (400-40-previewRectSize)+previewRectBorderSize, previewRectSize-(previewRectBorderSize*2), previewRectSize-(previewRectBorderSize*2));
}

void addValueToSlider(button * slider, int value) {
	char sValue[8];
	sprintf(sValue, "%d", value);
	strcpy(slider->shortText2, sValue);
}

bool translucencySlidersMade = false;
buttonList translucencySlidersTop;
buttonList translucencySlidersBottom;

void drawTranslucencyAdjust(gfxScreen_t screen) {
	if (!translucencySlidersMade) {
		translucencySlidersMade = true;

		addSlider(160, "Top bar", &translucencySlidersTop);
		addSlider(120, "Water", &translucencySlidersTop);
		addSlider(80, "App icon shadow", &translucencySlidersTop);

		addSlider(160, "Bottom bar", &translucencySlidersBottom);
		addSlider(125, "Toolbar icon backgrounds", &translucencySlidersBottom);
		addSlider(90, "App backgrounds", &translucencySlidersBottom);
		addSlider(55, "Paging controls", &translucencySlidersBottom);
		addSlider(20, "Toolbar icon symbols", &translucencySlidersBottom);
	}

	buttonList * aButtonList;

	if (screen == GFX_TOP) {
		aButtonList = &translucencySlidersTop;
	} else {
		aButtonList = &translucencySlidersBottom;
	}

	button * slider0 = aButtonList->buttons[0];
	button * slider1 = aButtonList->buttons[1];
	button * slider2 = aButtonList->buttons[2];
	button * slider3;
	button * slider4;

	if (screen == GFX_TOP) {
		slider0->value = translucencyTopBar;
		slider1->value = translucencyWater;
		slider2->value = translucencyAppShadow;

		addValueToSlider(slider0, translucencyTopBar);
		addValueToSlider(slider1, translucencyWater);
		addValueToSlider(slider2, translucencyAppShadow);
	} else {
		slider3 = aButtonList->buttons[3];
		slider4 = aButtonList->buttons[4];

		slider0->value = translucencyBottomBar;
		slider1->value = translucencyBarIcons;
		slider2->value = translucencyAppBackgrounds;
		slider3->value = translucencyPageControls;
		slider4->value = translucencyBarIconSymbols;

		addValueToSlider(slider0, translucencyBottomBar);
		addValueToSlider(slider1, translucencyBarIcons);
		addValueToSlider(slider2, translucencyAppBackgrounds);
		addValueToSlider(slider3, translucencyPageControls);
		addValueToSlider(slider4, translucencyBarIconSymbols);
	}

	if (!touchesAreBlocked && (hidKeysDown()&KEY_TOUCH || hidKeysHeld()&KEY_TOUCH || hidKeysUp()&KEY_TOUCH)) {
		touchPosition touch;
		hidTouchRead(&touch);
		touchX = touch.px;
		touchY = touch.py;

		if (!(hidKeysUp()&KEY_TOUCH)) {
			previousTouchX = touchX;
			previousTouchY = touchY;
		}

		int previous0 = slider0->value;
		int previous1 = slider1->value;
		int previous2 = slider2->value;
		int previous3;

		int new0 = updateSliderValue(touchX, touchY, slider0);
		int new1 = updateSliderValue(touchX, touchY, slider1);
		int new2 = updateSliderValue(touchX, touchY, slider2);
		int new3;
		int new4;

		if (screen == GFX_TOP) {
			if (previous0 != new0) {
				statusbarNeedsUpdate = true;
			}

			if (previous1 != new1) {
				staticWaterDrawn = false;
			}

			translucencyTopBar = new0;
			translucencyWater = new1;
			translucencyAppShadow = new2;
		} else {
			previous3 = slider3->value;

			new3 = updateSliderValue(touchX, touchY, slider3);
			new4 = updateSliderValue(touchX, touchY, slider4);

			if (previous0 != new0) {
				toolbarNeedsUpdate = true;
			}

			if (previous2 != new2 || previous3 != new3) {
				alphaImagesDrawn = false;
			}

			translucencyBottomBar = new0;
			translucencyBarIcons = new1;
			translucencyAppBackgrounds = new2;
			translucencyPageControls = new3;
			translucencyBarIconSymbols = new4;
		}
	}

	btnDrawButtonList(aButtonList);
}

bool panelTranslucencySlidersMade = false;
buttonList panelTranslucencySlidersTop;
buttonList panelTranslucencySlidersBottom;

void drawPanelTranslucencyAdjust(gfxScreen_t screen) {
	if (!panelTranslucencySlidersMade) {
		panelTranslucencySlidersMade = true;

		addSlider(170, "Translucency", &panelTranslucencySlidersTop);
		addSlider(130, "Red", &panelTranslucencySlidersTop);
		addSlider(90, "Green", &panelTranslucencySlidersTop);
		addSlider(50, "Blue", &panelTranslucencySlidersTop);
		addSlider(10, "Left edge offset", &panelTranslucencySlidersTop);

		addSlider(160, "Translucency", &panelTranslucencySlidersBottom);
		addSlider(120, "Red", &panelTranslucencySlidersBottom);
		addSlider(80, "Green", &panelTranslucencySlidersBottom);
		addSlider(40, "Blue", &panelTranslucencySlidersBottom);
	}

	buttonList * panelTranslucencySliders;

	if (screen == GFX_TOP) {
		panelTranslucencySliders = &panelTranslucencySlidersTop;
	} else {
		panelTranslucencySliders = &panelTranslucencySlidersBottom;
	}

	MAGFXDrawPanel(GFX_TOP, false);
	MAGFXDrawPanel(GFX_BOTTOM, false);

	button * slider1 = panelTranslucencySliders->buttons[0];
	button * slider2 = panelTranslucencySliders->buttons[1];
	button * slider3 = panelTranslucencySliders->buttons[2];
	button * slider4 = panelTranslucencySliders->buttons[3];
	button * slider5;

	if (screen == GFX_TOP) {
		slider5 = panelTranslucencySliders->buttons[4];

		slider1->value = panelAlphaTop;
		slider2->value = panelRTop;
		slider3->value = panelGTop;
		slider4->value = panelBTop;
		slider5->value = panelLeftOffsetTop;

		addValueToSlider(slider1, panelAlphaTop);
		addValueToSlider(slider2, panelRTop);
		addValueToSlider(slider3, panelGTop);
		addValueToSlider(slider4, panelBTop);
		addValueToSlider(slider5, panelLeftOffsetTop);
	} else {
		slider1->value = panelAlphaBottom;
		slider2->value = panelRBottom;
		slider3->value = panelGBottom;
		slider4->value = panelBBottom;

		addValueToSlider(slider1, panelAlphaBottom);
		addValueToSlider(slider2, panelRBottom);
		addValueToSlider(slider3, panelGBottom);
		addValueToSlider(slider4, panelBBottom);
	}

	if (!touchesAreBlocked && (hidKeysDown()&KEY_TOUCH || hidKeysHeld()&KEY_TOUCH || hidKeysUp()&KEY_TOUCH)) {
		touchPosition touch;
		hidTouchRead(&touch);
		touchX = touch.px;
		touchY = touch.py;

		if (!(hidKeysUp()&KEY_TOUCH)) {
			previousTouchX = touchX;
			previousTouchY = touchY;
		}

		int previousTranslucency = slider1->value;

		int previousR = slider2->value;
		int previousG = slider3->value;
		int previousB = slider4->value;

		int newTranslucency = updateSliderValue(touchX, touchY, slider1);

		int newR = updateSliderValue(touchX, touchY, slider2);
		int newG = updateSliderValue(touchX, touchY, slider3);
		int newB = updateSliderValue(touchX, touchY, slider4);

		if (screen == GFX_TOP) {
			int newLeftOffset = updateSliderValue(touchX, touchY, slider5);

			panelAlphaTop = newTranslucency;
			panelLeftOffsetTop = newLeftOffset;

			panelRTop = newR;
			panelGTop = newG;
			panelBTop = newB;
		} else {
			panelAlphaBottom = newTranslucency;

			panelRBottom = newR;
			panelGBottom = newG;
			panelBBottom = newB;
		}

		if (newTranslucency != previousTranslucency || newR != previousR || newG != previousG || newB != previousB) {
			panelsDrawn = false;
			pageControlPanelsDrawn = false;
		}
	}

	btnDrawButtonList(panelTranslucencySliders);
}
