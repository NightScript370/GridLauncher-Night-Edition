#include "button.h"
#include <string.h>
#include "gfx.h"
#include "MAGFX.h"
#include "colours.h"

#include "lefttoolbarbuttonalphamask_bin.h"
#include "righttoolbarbuttonalphamask_bin.h"
#include "bottomrighttoolbarbuttonalphamask_bin.h"
#include "bottomlefttoolbarbuttonalphamask_bin.h"

#include "arrowleftalphamask_bin.h"
#include "arrowrightalphamask_bin.h"

#include "toolbarbuttonshadowleft_bin.h"
#include "toolbarbuttonshadowright_bin.h"
#include "toolbarbuttonshadowbottomright_bin.h"
#include "toolbarbuttonshadowbottomleft_bin.h"

#include "questionmark_bin.h"
#include "spanner_bin.h"
#include "backarrow_bin.h"
#include "folder_bin.h"
#include "home_bin.h"

#include "MAFontRobotoRegular.h"

#include "buttoncornertopleft_bin.h"
#include "buttoncornertopright_bin.h"
#include "buttoncornerbottomleft_bin.h"
#include "buttoncornerbottomright_bin.h"

#include "tick_bin.h"

#include "menu.h"

#include "themegfx.h"

#define sliderIndicatorWidth 4

void btnSetButtonType(button * aButton, int type) {
	aButton->buttonType = type;

	aButton->selected = false;
	aButton->highlighted = false;
	aButton->enabled = true;
	aButton->visible = true;

	strcpy(aButton->shortText1, "");
	strcpy(aButton->shortText2, "");
	strcpy(aButton->longText, "");

	if (type == btnButtonTypeToolbarLeft || type == btnButtonTypeToolbarRight || type == btnButtonTypeToolbarBottomRight || type == btnButtonTypeToolbarBottomLeft) {
		strcpy(aButton->shortText1, "");
		strcpy(aButton->shortText2, "");
		strcpy(aButton->longText, "");
		aButton->w = btnToolbarWidth;
		aButton->h = btnToolbarHeight;
		aButton->screen = GFX_BOTTOM;
		aButton->side = GFX_LEFT;

		if (type == btnButtonTypeToolbarLeft) {
			aButton->y = 0;
			btnTopJustify(aButton);
		} else if (type == btnButtonTypeToolbarRight) {
			btnTopJustify(aButton);
			btnRightJustify(aButton);
		} else if (type == btnButtonTypeToolbarBottomRight) {
			aButton->x = 0;
			btnRightJustify(aButton);
		} else if (type == btnButtonTypeToolbarBottomLeft) {
			aButton->x = 0;
			aButton->y = 0;
		}
	} else if (type == btnButtonTypePageArrowLeft || type == btnButtonTypePageArrowRight) {
		aButton->w = 29;
		aButton->h = 21;
		aButton->screen = GFX_BOTTOM;
		aButton->side = GFX_LEFT;

		aButton->x = (240/2) - (aButton->w/2);

		int arrowEdgeOffset = 5;

		if (type == btnButtonTypePageArrowLeft) {
			aButton->y = arrowEdgeOffset;
		} else if (type == btnButtonTypePageArrowRight) {
			aButton->y = 320-aButton->h-arrowEdgeOffset;
		}

		aButton->highlighted = true;
	} else if (type == btnButtonTypeRect) {
		aButton->visible = true;
		aButton->w = btnRectWidth;
		aButton->h = btnRectHeight;
		aButton->screen = GFX_BOTTOM;
		aButton->side = GFX_LEFT;
	} else if (type == btnButtonTypeSlider) {
		aButton->w = 30;
		aButton->h = 257;
		aButton->screen = GFX_BOTTOM;
		aButton->side = GFX_LEFT;
		aButton->value = 0;
	}
}

void btnDrawIcon(button * aButton, u8 * icon) {
	u8 r, g, b;

	if (aButton->highlighted) {
		r = 255;
		g = 255;
		b = 255;
	} else {
		r = 50;
		g = 50;
		b = 50;
	}

	u8 iconDest[aButton->w * aButton->h * 4];
	MAGFXImageWithRGBAndAlphaMask(r, g, b, icon, iconDest, aButton->w, aButton->h);
	gfxDrawSpriteAlphaBlendFade(aButton->screen, aButton->side, (u8*)iconDest, aButton->w, aButton->h, aButton->x, aButton->y, translucencyBarIconSymbols);
}

void drawButtonRect(gfxScreen_t screen, gfx3dSide_t side, int x, int y, int w, int h, int r, int g, int b) {
	u8 corner[9*9*4];

	MAGFXImageWithRGBAndAlphaMask(r, g, b, (u8*)buttoncornertopleft_bin, corner, 9, 9);
	gfxDrawSpriteAlphaBlend(screen, side, corner, 9, 9, x+(w-9), y);

	MAGFXImageWithRGBAndAlphaMask(r, g, b, (u8*)buttoncornertopright_bin, corner, 9, 9);
	gfxDrawSpriteAlphaBlend(screen, side, corner, 9, 9, x+(w-9), y+h-9);

	MAGFXImageWithRGBAndAlphaMask(r, g, b, (u8*)buttoncornerbottomleft_bin, corner, 9, 9);
	gfxDrawSpriteAlphaBlend(screen, side, corner, 9, 9, x, y);

	MAGFXImageWithRGBAndAlphaMask(r, g, b, (u8*)buttoncornerbottomright_bin, corner, 9, 9);
	gfxDrawSpriteAlphaBlend(screen, side, corner, 9, 9, x, y+h-9);

	u8 rgb[3] = {r, g, b};

	gfxDrawRectangle(screen, side, rgb, x, y+9, w, h-18);
	gfxDrawRectangle(screen, side, rgb, x+9, y, w-18, h);
}

void drawButtonRectForButton(button * aButton, int r, int g, int b) {
	drawButtonRect(aButton->screen, aButton->side, aButton->x, aButton->y, aButton->w, aButton->h, r, g, b);
}

#define sliderButtonHeight 30
#define sliderButtonWidth slider->w
#define sliderButtonX slider->x
#define sliderMinusButtonY slider->y - sliderButtonHeight
#define sliderPlusButtonY slider->y + slider->h

void btnDrawSlider(button * slider) {
	if (!slider->visible) {
		return;
	}

	rgbColour * aRGB = inactiveColour();

	drawButtonRectForButton(slider, aRGB->r, aRGB->g, aRGB->b);

	u8 rgb[3] = {0, 0, 0};

	gfxDrawRectangle(slider->screen, slider->side, rgb, slider->x, (slider->y+slider->value)-(sliderIndicatorWidth/2), slider->w, sliderIndicatorWidth);

	rgb[0] = 255;
	rgb[1] = 255;
	rgb[2] = 255;

	gfxDrawRectangle(slider->screen, slider->side, rgb, slider->x+1, ((slider->y+slider->value)-(sliderIndicatorWidth/2))+1, slider->w-2, sliderIndicatorWidth-2);

	rgbColour * dark = darkTextColour();
	MAFont *font = &MAFontRobotoRegular10;
	int lineHeight = font->lineHeight;

	if (strlen(slider->shortText1) > 0) {
		MADrawTextWrap(slider->screen, slider->side, slider->x + (slider->w/2) - (lineHeight/2), slider->y + 20, slider->shortText1, font, dark->r, dark->g, dark->b, 0, 0);
	}

	if (strlen(slider->shortText2) > 0) {
		int width = MATextWidthInPixels(slider->shortText2, font);

		MADrawTextWrap(slider->screen, slider->side, slider->x + (slider->w/2) - (lineHeight/2), (slider->y + slider->h) - width - 20, slider->shortText2, font, dark->r, dark->g, dark->b, 0, 0);
	}

	drawButtonRect(slider->screen, slider->side, sliderButtonX, sliderMinusButtonY, sliderButtonWidth, sliderButtonHeight, dark->r, dark->g, dark->b);

	drawButtonRect(slider->screen, slider->side, sliderButtonX, sliderPlusButtonY, sliderButtonWidth, sliderButtonHeight, dark->r, dark->g, dark->b);

	rgbColour *light = lightTextColour();

	MADrawText(slider->screen, slider->side, sliderButtonX-2, sliderMinusButtonY+10, "-", &MAFontRobotoRegular16, light->r, light->g, light->b);
	MADrawText(slider->screen, slider->side, sliderButtonX-2, sliderPlusButtonY+10, "+", &MAFontRobotoRegular16, light->r, light->g, light->b);
}

void btnDrawMaskedBitmapForButton(button * aButton, u8 const * mask, int r, int g, int b, int translucencyLevel) {
	u8 dest[aButton->w * aButton->h * 4];
	MAGFXImageWithRGBAndAlphaMask(r, g, b, (u8*)mask, dest, aButton->w, aButton->h);
	gfxDrawSpriteAlphaBlendFade(aButton->screen, aButton->side, dest, aButton->w, aButton->h, aButton->x, aButton->y, translucencyLevel);
}

void btnDrawButton(button * aButton) {
	if (!aButton->visible) {
		return;
	}

	if (aButton->buttonType == btnButtonTypeSlider) {
		btnDrawSlider(aButton);
		return;
	}

	rgbColour *aRGB;

	if (aButton->highlighted) {
		aRGB = tintColour();

	} else {
		aRGB = inactiveColour();
	}

	u8 r = aRGB->r;
	u8 g = aRGB->g;
	u8 b = aRGB->b;


	if (aButton->buttonType == btnButtonTypeToolbarLeft) {
		if (themeImageExists(themeImageTopLeftButton)) {
			if (aButton->highlighted && themeImageExists(themeImageTopLeftButtonSelected)) {
				drawThemeImage(themeImageTopLeftButtonSelected, aButton->screen, aButton->x-(36-aButton->h), aButton->y);
			} else {
				drawThemeImage(themeImageTopLeftButton, aButton->screen, aButton->x-(36-aButton->h), aButton->y);
			}
		} else {
			gfxDrawSpriteAlphaBlendFade(aButton->screen, aButton->side, (u8*)toolbarbuttonshadowleft_bin, 41, 41, aButton->x-(41-aButton->h), aButton->y, translucencyBarIcons);
			btnDrawMaskedBitmapForButton(aButton, lefttoolbarbuttonalphamask_bin, r, g, b, translucencyBarIcons);
		}
	} else if (aButton->buttonType == btnButtonTypeToolbarRight) {
		if (themeImageExists(themeImageTopRightButton)) {
			if (aButton->highlighted && themeImageExists(themeImageTopRightButtonSelected)) {
				drawThemeImage(themeImageTopRightButtonSelected, aButton->screen, aButton->x-(36-aButton->h), aButton->y-(36-aButton->w));
			} else {
				drawThemeImage(themeImageTopRightButton, aButton->screen, aButton->x-(36-aButton->h), aButton->y-(36-aButton->w));
			}
		} else {
			gfxDrawSpriteAlphaBlendFade(aButton->screen, aButton->side, (u8*)toolbarbuttonshadowright_bin, 41, 41, aButton->x-(41-aButton->h), aButton->y-(41-aButton->w), translucencyBarIcons);
			btnDrawMaskedBitmapForButton(aButton, righttoolbarbuttonalphamask_bin, r, g, b, translucencyBarIcons);
		}
	} else if (aButton->buttonType == btnButtonTypeToolbarBottomRight) {
		if (themeImageExists(themeImageBottomRightButton)) {
			if (aButton->highlighted && themeImageExists(themeImageBottomRightButtonSelected)) {
				drawThemeImage(themeImageBottomRightButtonSelected, aButton->screen, aButton->x, aButton->y-(36-aButton->w));
			} else {
				drawThemeImage(themeImageBottomRightButton, aButton->screen, aButton->x, aButton->y-(36-aButton->w));
			}
		} else {
			gfxDrawSpriteAlphaBlendFade(aButton->screen, aButton->side, (u8*)toolbarbuttonshadowbottomright_bin, 41, 41, aButton->x, aButton->y-(41-aButton->w), translucencyBarIcons);
			btnDrawMaskedBitmapForButton(aButton, bottomrighttoolbarbuttonalphamask_bin, r, g, b, translucencyBarIcons);
		}
	} else if (aButton->buttonType == btnButtonTypeToolbarBottomLeft) {
		if (themeImageExists(themeImageBottomLeftButton)) {
			if (aButton->highlighted && themeImageExists(themeImageBottomLeftButtonSelected)) {
				drawThemeImage(themeImageBottomLeftButtonSelected, aButton->screen, aButton->x, aButton->y);
			} else {
				drawThemeImage(themeImageBottomLeftButton, aButton->screen, aButton->x, aButton->y);
			}
		} else {
			gfxDrawSpriteAlphaBlendFade(aButton->screen, aButton->side, (u8*)toolbarbuttonshadowbottomleft_bin, 41, 41, aButton->x, aButton->y, translucencyBarIcons);
			btnDrawMaskedBitmapForButton(aButton, bottomlefttoolbarbuttonalphamask_bin, r, g, b, translucencyBarIcons);
		}
	} else if (aButton->buttonType == btnButtonTypePageArrowLeft) {
		btnDrawMaskedBitmapForButton(aButton, arrowleftalphamask_bin, r, g, b, translucencyPageControls);
	} else if (aButton->buttonType == btnButtonTypePageArrowRight) {
		btnDrawMaskedBitmapForButton(aButton, arrowrightalphamask_bin, r, g, b, translucencyPageControls);
	} else if (aButton->buttonType == btnButtonTypeRect) {
		drawButtonRectForButton(aButton, r, g, b);
	} else {
		return;
	}

	rgbColour * dark = darkTextColour();

	if (aButton->selected) {
		gfxDrawSpriteAlphaBlend(aButton->screen, aButton->side, (u8*)tick_bin, 48, 48, aButton->x, aButton->y);
	}

	if (aButton->buttonIcon == btnButtonIconBackArrow) {
		if (themeImageExists(themeImageBackSymbol)) {
			drawThemeImage(themeImageBackSymbol, aButton->screen, aButton->x, aButton->y);
		} else {
			btnDrawIcon(aButton, (u8*)backarrow_bin);
		}
	} else if (aButton->buttonIcon == btnButtonIconSpanner) {
		if (themeImageExists(themeImageSettingsSymbol)) {
			drawThemeImage(themeImageSettingsSymbol, aButton->screen, aButton->x, aButton->y);
		} else {
			btnDrawIcon(aButton, (u8*)spanner_bin);
		}
	} else if (aButton->buttonIcon == btnButtonIconQuestionMark) {
		if (themeImageExists(themeImageHelpSymbol)) {
			drawThemeImage(themeImageHelpSymbol, aButton->screen, aButton->x, aButton->y);
		} else {
			btnDrawIcon(aButton, (u8*)questionmark_bin);
		}
	} else if (aButton->buttonIcon == btnButtonIconFolder) {
		if (themeImageExists(themeImageFoldersSymbol)) {
			drawThemeImage(themeImageFoldersSymbol, aButton->screen, aButton->x, aButton->y);
		} else {
			btnDrawIcon(aButton, (u8*)folder_bin);
		}
	} else if (aButton->buttonIcon == btnButtonIconHome) {
		if (themeImageExists(themeImageHomeSymbol)) {
			drawThemeImage(themeImageHomeSymbol, aButton->screen, aButton->x, aButton->y);
		} else {
			btnDrawIcon(aButton, (u8*)home_bin);
		}
	}

	MAFont *font = &MAFontRobotoRegular10;

	int lineHeight = font->lineHeight;

	int textOffset = 10;

	if (strlen(aButton->shortText1) > 0) {
		MADrawTextWrap(aButton->screen, aButton->side, aButton->x + (aButton->w/2), aButton->y+textOffset, aButton->shortText1, font, dark->r, dark->g, dark->b, 0, 0);
	}

	if (strlen(aButton->shortText2) > 0) {
		MADrawTextWrap(aButton->screen, aButton->side, aButton->x + (aButton->w/2) - lineHeight, aButton->y + textOffset, aButton->shortText2, font, dark->r, dark->g, dark->b, 0, 0);
	}

	if (strlen(aButton->longText) > 0) {
		MADrawTextWrap(aButton->screen, aButton->side, aButton->x + (aButton->w/2) - (lineHeight/2), aButton->y + textOffset, aButton->longText, font, dark->r, dark->g, dark->b, 0, 0);
	}

}

bool btnTouchWithinRect(int touchX, int touchY, int x, int y, int w, int h) {
	if (touchX > y && touchX < (y+h) && (240-touchY) > x && (240-touchY) < (x+w)) {
		return true;
	}

	return false;
}

bool btnTouchWithin(int touchX, int touchY, button * aButton) {
	return btnTouchWithinRect(touchX, touchY, aButton->x, aButton->y, aButton->w, aButton->h);
}

void btnCheckHighlight(int touchX, int touchY, button * aButton) {
	aButton->highlighted = btnTouchWithin(touchX, touchY, aButton);
}

void btnTopJustify(button * aButton) {
	aButton->x = 240-aButton->w;
}

void btnRightJustify(button * aButton) {
	aButton->y = 320-aButton->h;
}

int updateSliderValue(int touchX, int touchY, button * slider) {
	if (btnTouchWithin(touchX, touchY, slider)) {
		slider->value = touchX - slider->y - 1;
	}

	if (hidKeysDown()&KEY_TOUCH) {
		if (btnTouchWithinRect(touchX, touchY, sliderButtonX, sliderMinusButtonY, sliderButtonWidth, sliderButtonHeight)) {

			if (slider->value > 0) {
				slider->value = slider->value - 1;
			}
		} else if (btnTouchWithinRect(touchX, touchY, sliderButtonX, sliderPlusButtonY, sliderButtonWidth, sliderButtonHeight)) {

			if (slider->value < slider->h - 2) {
				slider->value = slider->value + 1;
			}
		}
	}

	return slider->value;
}

void btnAddButtonToButtonList(button * aButton, buttonList *aButtonList) {
	if (aButtonList->numButtons == buttonListMaxButtons) {
		return;
	}

	memcpy(aButtonList->buttons[aButtonList->numButtons], aButton, sizeof(button));
	aButtonList->numButtons = aButtonList->numButtons + 1;
}

void btnListCheckHighlight(buttonList * aButtonList, int touchX, int touchY) {
	int i;
	for (i=0; i<aButtonList->numButtons; i++) {
		button * aButton = aButtonList->buttons[i];
		if (aButton->enabled) {
			btnCheckHighlight(touchX, touchY, aButton);
		}
	}
}

void btnRunCallback(button * aButton) {
	if (aButton->callbackObject1) {
		if (aButton->callbackObject2) {
			(aButton->callback)(aButton->callbackObject1, aButton->callbackObject2);
		} else {
			(aButton->callback)(aButton->callbackObject1);
		}
	} else {
		(aButton->callback)();
	}
}

void btnCheckRunCallback(button * aButton, int touchX, int touchY) {
	if (aButton->enabled) {
		if (btnTouchWithin(touchX, touchY, aButton)) {
			btnRunCallback(aButton);
		}
	}
}

void btnListCheckRunCallback(buttonList * aButtonList, int touchX, int touchY) {
	int i;
	for (i=0; i<aButtonList->numButtons; i++) {
		button * aButton = aButtonList->buttons[i];
		btnCheckRunCallback(aButton, touchX, touchY);
	}
}

void btnListUnHighlight(buttonList * aButtonList) {
	int i;
	for (i=0; i<aButtonList->numButtons; i++) {
		button * aButton = aButtonList->buttons[i];
		aButton->highlighted = false;
	}
}

void btnDrawButtonList(buttonList * aButtonList) {
	int i;
	for (i=0; i<aButtonList->numButtons; i++) {
		button * aButton = aButtonList->buttons[i];
		btnDrawButton(aButton);
	}
}

button * btnButtonInListWithTag(buttonList * aButtonList, int tag) {
	int i;
	for (i=0; i<aButtonList->numButtons; i++) {
		button * aButton = aButtonList->buttons[i];
		if (aButton->tag == tag) {
			return aButton;
		}
	}

	return NULL;
}
