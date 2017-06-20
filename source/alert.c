#include "alert.h"
#include "MAFontRobotoRegular.h"
#include "colours.h"
#include "button.h"
#include <string.h>
#include "MAGFX.h"
#include "stdlib.h"
#include "touchblock.h"

int alertSelectedButton = 0;

int drawAlert(char * title, char * body, u8 * image, int numButtons,  char buttonTitles[3][32]) {
	//int ret = alertButtonNone;

	if (hidKeysDown()&KEY_A) {
		return alertSelectedButton;
	}

	if (hidKeysDown()&KEY_B) {
		return alertButtonKeyB;
	}

	if (hidKeysDown()&KEY_DOWN) {
		alertSelectedButton++;
		if (alertSelectedButton >= numButtons) {
			alertSelectedButton = 0;
		}
	}

	if (hidKeysDown()&KEY_UP) {
		alertSelectedButton--;
		if (alertSelectedButton < 0) {
			alertSelectedButton = numButtons-1;
		}
	}

	MAGFXDrawPanel(GFX_TOP, true);
	MAGFXDrawPanel(GFX_BOTTOM, false);

	MAFont font = MAFontRobotoRegular16;
	int leftOffset = 30;
	int topOffset = 240 - 50 - font.lineHeight;

	rgbColour * textCol = titleTextColour();
	MADrawText(GFX_TOP, GFX_LEFT, topOffset, leftOffset, title, &font, textCol->r, textCol->g, textCol->b);

	font = MAFontRobotoRegular10;
	topOffset -= font.lineHeight * 2;

	int totalWidthForText = 400;

	rgbColour * dark = darkTextColour();

	MADrawTextWrap(GFX_TOP, GFX_LEFT, topOffset, leftOffset, body, &font, dark->r, dark->g, dark->b, totalWidthForText-(2*leftOffset), 0);


	/*
		WEIRD BUG ALERT!

		It was discovered that some alerts did not display their body text.
		In the process of trying to work out what was happening, I created
		a string containing the number of lines drawn by MADrawTextWrap and
		drew this on the screen so I could see what was actually happening.
		When I did this, the body text started appearing as well as the text
		showing how many lines it had drawn. I then found that it was the
		process of creating a string and putting something in it which was
		allowing the text to be drawn. I actually have no idea why this fixes
		the bug with drawing the body text. It feels like a memory management
		thing, but I can't figure it out.
	*/

	char s[64];
	strcpy(s, "5");

	int screenHeight = 240;
	int screenWidth = 320;

	int buttonGap = 10;

	int totalHeight = (numButtons*btnRectWidth) + ((numButtons-1)*buttonGap);
	int diff = screenHeight-totalHeight;

	int x = screenHeight - (diff/2) - btnRectWidth;
	int y = (screenWidth/2) - (btnRectHeight/2);

	touchPosition touch;
	hidTouchRead(&touch);
	int touchX = touch.px;
	int touchY = touch.py;

	int i;

	for (i=0; i<numButtons; i++) {
		button * aButton = malloc(sizeof(button));
		btnSetButtonType(aButton, btnButtonTypeRect);
		strcpy(aButton->longText, buttonTitles[i]);
		strcpy(aButton->shortText1, "");
		strcpy(aButton->shortText2, "");
		aButton->x = x;
		aButton->y = y;
		aButton->highlighted = (i == alertSelectedButton);
		aButton->selected = false;
		x -= (btnRectWidth+buttonGap);

		btnDrawButton(aButton);

		if (!touchesAreBlocked && hidKeysDown()&KEY_TOUCH && btnTouchWithin(touchX, touchY, aButton)) {
			if (i == alertSelectedButton) {
				return alertSelectedButton;
			}
			else {
				alertSelectedButton = i;
			}
		}

		free(aButton);


	}

	return alertButtonNone;
}

