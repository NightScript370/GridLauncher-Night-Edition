#include <string.h>
#include <math.h>
#include <malloc.h>
#include "water.h"
#include "colours.h"
#include "MAGFX.h"

#include "stillwater_bin.h"
#include "stillwaterborder_bin.h"

#define BG_WATER_CONTROLPOINTS (100)
#define BG_WATER_NEIGHBORHOODS (3)
#define BG_WATER_DAMPFACTOR (0.7f)
#define BG_WATER_SPRINGFACTOR (0.85f)
#define BG_WATER_WIDTH (500)
#define BG_WATER_OFFSET (-25)

bool waterEnabled = true;
bool hideWaves = false;
bool waterAnimated = true;

static waterEffect_s waterEffect;
static int backgroundCnt;

void initWater() {
	initWaterEffect(&waterEffect, BG_WATER_CONTROLPOINTS, BG_WATER_NEIGHBORHOODS, BG_WATER_DAMPFACTOR, BG_WATER_SPRINGFACTOR, BG_WATER_WIDTH, BG_WATER_OFFSET);
	backgroundCnt = 0;
}

#define waterTopLevel 50
#define waterLevelDiff 5
#define waterLowerLevel waterTopLevel - waterLevelDiff

int topLevel = waterTopLevel;
int lowerLevel = waterLowerLevel;

int staticWaterX = 0;

u8 tintedWater[70*400*4];
u8 tintedWaterBorder[70*400*4];
bool staticWaterDrawn = false;

void drawWater() {
	if (waterEnabled) {
		rgbColour * waterTop = waterTopColour();
		rgbColour * waterBottom = waterBottomColour();

		if (!waterAnimated) {
			if (hideWaves) {
				if (staticWaterX > -70) {
					staticWaterX -= 2;
				}
			} else {
				if (staticWaterX < 0) {
					staticWaterX += 2;
				}
			}


			if (!staticWaterDrawn) {
				MAGFXImageWithRGBAndAlphaMask(waterBottom->r, waterBottom->g, waterBottom->b, (u8*)stillwater_bin, tintedWater, 70, 400);
				MAGFXImageWithRGBAndAlphaMask(waterTop->r, waterTop->g, waterTop->b, (u8*)stillwaterborder_bin, tintedWaterBorder, 70, 400);
					staticWaterDrawn = true;
			}


			gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, tintedWater, 70, 400, staticWaterX, 0, translucencyWater);
			gfxDrawSpriteAlphaBlendFade(GFX_TOP, GFX_LEFT, tintedWaterBorder, 70, 400, staticWaterX, 0, translucencyWater);
			return;
		}

		if (hideWaves) {
			if (lowerLevel > 0) {
				topLevel -= 1;
				lowerLevel -= 1;
			}
		} else {
			if (lowerLevel < waterLowerLevel) {
				topLevel += 1;
				lowerLevel += 1;
			}
		}


		u8 * waterBorderColor = (u8[]){waterTop->r, waterTop->g, waterTop->b};
		u8 * waterColor = (u8[]){waterBottom->r, waterBottom->g, waterBottom->b};

		gfxDrawWave(GFX_TOP, GFX_LEFT, waterBorderColor, waterColor, topLevel, 20, 5, (gfxWaveCallback)&evaluateWater, &waterEffect);
		gfxDrawWave(GFX_TOP, GFX_LEFT, waterColor, waterBorderColor, lowerLevel, 20, 0, (gfxWaveCallback)&evaluateWater, &waterEffect);
	}
}

void updateWater() {
	if (!waterAnimated) {
		return;
	}

	exciteWater(&waterEffect, sin(backgroundCnt*0.1f)*2.0f, 0, true);
	updateWaterEffect(&waterEffect);
	backgroundCnt++;
}

void initWaterEffect(waterEffect_s* we, u16 n, u16 s, float d,  float sf, u16 w, s16 offset) {
	if(!we)return;

	we->numControlPoints=n;
	we->neighborhoodSize=s;
	we->dampFactor=d;
	we->springFactor=sf;
	we->width=w;
	we->offset=offset;
	we->controlPoints=calloc(n, sizeof(float));
	we->controlPointSpeeds=calloc(n, sizeof(float));
}

//dst shouldn't have been initialized
void copyWaterEffect(waterEffect_s* dst, waterEffect_s* src) {
	if(!dst || !src)return;

	initWaterEffect(dst, src->numControlPoints, src->neighborhoodSize, src->dampFactor, src->springFactor, src->width, src->offset);
	memcpy(dst->controlPoints, src->controlPoints, sizeof(float)*src->numControlPoints);
	memcpy(dst->controlPointSpeeds, src->controlPointSpeeds, sizeof(float)*src->numControlPoints);
}

void killWaterEffect(waterEffect_s* we) {
	if(!we)return;

	free(we->controlPoints);
	free(we->controlPointSpeeds);
}

float getNeighborAverage(waterEffect_s* we, int k) {
	if(!we || k<0 || k>=we->numControlPoints)return 0.0f;

	float sum=0.0f;
	float factors=0.0f;

	int i;
	for(i=k-we->neighborhoodSize; i<k+we->neighborhoodSize; i++) {
		if(i==k)continue;
		const int d=i-k;
		const float f=fabs(1.0f/d); // TODO : better function (gauss ?)
		float v=0.0f;
		if(i>=0 && i<we->numControlPoints)v=we->controlPoints[i];
		sum+=f*v;
		factors+=f;
	}

	return sum/factors;
}

float evaluateWater(waterEffect_s* we, u16 x) {
	if(!we || x>=we->width)return 0.0f;

	const float vx=((float)((x-we->offset)*we->numControlPoints))/we->width;
	const int k=(int)vx;
	const float f=vx-(float)k;

	return we->controlPoints[k]*(1.0f-f)+we->controlPoints[k+1]*f;
}

void exciteWater(waterEffect_s* we, float v, u16 k, bool absolute) {
	if(!we || k>=we->numControlPoints)return;

	if(absolute) {
		we->controlPoints[k]=v;
		we->controlPointSpeeds[k]=0.0f;
	} else we->controlPoints[k]+=v;
}

void updateWaterEffect(waterEffect_s* we) {
	if(!we)return;

	waterEffect_s tmpwe;
	copyWaterEffect(&tmpwe, we);

	int k;
	for(k=0; k<we->numControlPoints; k++) {
		float rest=getNeighborAverage(&tmpwe, k);
		we->controlPointSpeeds[k]*=we->dampFactor;
		we->controlPointSpeeds[k]+=(rest-we->controlPoints[k])*we->springFactor;
		we->controlPoints[k]+=we->controlPointSpeeds[k];
	}

	killWaterEffect(&tmpwe);
}
