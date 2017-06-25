#pragma once
#include <3ds.h>

#ifndef BUTTON_H
#define BUTTON_H

#define btnButtonTypeToolbarLeft 10
#define btnButtonTypeToolbarRight 20
#define btnButtonTypeToolbarBottomRight 30
#define btnButtonTypePageArrowLeft 40
#define btnButtonTypePageArrowRight 50
#define btnButtonTypeRect 60
#define btnButtonTypeToolbarBottomLeft 70
#define btnButtonTypeSlider 80

#define btnButtonIconBackArrow 10
#define btnButtonIconSpanner 20
#define btnButtonIconQuestionMark 30
#define btnButtonIconFolder 40
#define btnButtonIconHome 50

#define btnRectWidth 40
#define btnRectHeight 130

#define btnToolbarWidth 36
#define btnToolbarHeight 36

#endif

typedef struct {
	int x, y, w, h;
	int buttonType, buttonIcon;
	bool highlighted, visible, selected, enabled;
	gfxScreen_t screen;
	gfx3dSide_t side;
	char longText[32], shortText1[32], shortText2[32];
	int value;
	void (*callback)();
	void *callbackObject1, *callbackObject2;
	bool (*selectedCallback)();
	int tag;
} button;

#define buttonListMaxButtons 8

typedef struct {
	button buttons[buttonListMaxButtons][sizeof(button)];
	int numButtons;
} buttonList;

void btnDrawButton(button * aButton);
bool btnTouchWithin(int touchX, int touchY, button * aButton);
void btnCheckHighlight(int touchX, int touchY, button * aButton);
void btnTopJustify(button * aButton);
void btnRightJustify(button * aButton);
void btnCentreVertically(button * aButton);
void btnSetButtonType(button * aButton, int type);
int updateSliderValue(int touchX, int touchY, button * slider);
void btnAddButtonToButtonList(button * aButton, buttonList *aButtonList);
void btnListCheckHighlight(buttonList * aButtonList, int touchX, int touchY);
void btnListCheckRunCallback(buttonList * aButtonList, int touchX, int touchY);
void btnRunCallback(button * aButton);
void btnCheckRunCallback(button * aButton, int touchX, int touchY);
void btnListUnHighlight(buttonList * aButtonList);
void btnDrawButtonList(buttonList * aButtonList);
button * btnButtonInListWithTag(buttonList * aButtonList, int tag);
