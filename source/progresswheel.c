#include "progresswheel.h"
#include "MAGFX.h"
#include "gfx.h"
#include "themegfx.h"

#include "progressWheelFrame1_bin.h"
#include "progressWheelFrame2_bin.h"
#include "progressWheelFrame3_bin.h"
#include "progressWheelFrame4_bin.h"
#include "progressWheelFrame5_bin.h"
#include "progressWheelFrame6_bin.h"

int currentFrame = 0;

void drawProgressWheel(gfxScreen_t screen, gfx3dSide_t side, int x, int y) {

	if (themeHasProgressWheel) {
		drawProgressWheelImage(currentFrame, screen, x, y);
	} else {
		u8 * progressWheelFrame;

		switch (currentFrame) {
			case 0:
				progressWheelFrame = (u8*)progressWheelFrame1_bin;
				break;

			case 1:
				progressWheelFrame = (u8*)progressWheelFrame2_bin;
				break;

			case 2:
				progressWheelFrame = (u8*)progressWheelFrame3_bin;
				break;

			case 3:
				progressWheelFrame = (u8*)progressWheelFrame4_bin;
				break;

			case 4:
				progressWheelFrame = (u8*)progressWheelFrame5_bin;
				break;

			case 5:
				progressWheelFrame = (u8*)progressWheelFrame6_bin;
				break;

			default:
				progressWheelFrame = (u8*)progressWheelFrame1_bin;
				break;
		}

		gfxDrawSpriteAlphaBlend(screen, side, (u8*)progressWheelFrame, 36, 36, x, y);
	}


	currentFrame++;// = aProgressWheel->frame + 1;
	if ((themeHasProgressWheel && currentFrame >= numProgressWheelImages) || (!themeHasProgressWheel && currentFrame >= 6)) {
		currentFrame = 0;
	}
}