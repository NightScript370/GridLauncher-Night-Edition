#include "sound.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "filesystem.h"

themeSound themeSoundBGM;
themeSound themeSoundMove;
themeSound themeSoundSelect;
themeSound themeSoundBack;
themeSound themeSoundBoot;

bool audioActive = false;
bool waitForSounds = true;

#include "logText.h"

void audio_load(const char *audio, themeSound * aThemeSound){

	FILE *file = fopen(audio, "rb");
	if(file != NULL && audioActive){
		fseek(file, 0, SEEK_END);
		aThemeSound->sndsize = ftell(file);
		fseek(file, 0, SEEK_SET);
		aThemeSound->sndsize = aThemeSound->sndsize - 0x48;
		fseek(file, 0x48, SEEK_SET);
		aThemeSound->sndbuffer = linearAlloc(aThemeSound->sndsize);
		fread(aThemeSound->sndbuffer, 1, aThemeSound->sndsize, file);
		fclose(file);
		if (aThemeSound->sndbuffer != NULL) {
			aThemeSound->loaded = true;
		} else {
			aThemeSound->loaded = false;
		}
	} else {
		aThemeSound->loaded = false;
	}

	if (aThemeSound->loaded) {
		aThemeSound->duration = aThemeSound->sndsize / 88244;
	} else {
		aThemeSound->duration = 0;
	}
}

void audioPlay(themeSound * aThemeSound, bool loop) {
   if (aThemeSound->loaded && audioActive) {
	u32 flags;

	if (loop) {
		flags = SOUND_FORMAT_16BIT | SOUND_REPEAT;
	} else {
		flags = SOUND_FORMAT_16BIT;
	}

	csndPlaySound(aThemeSound->channel, flags, 44100, 1, 0, aThemeSound->sndbuffer, aThemeSound->sndbuffer, aThemeSound->sndsize);
   }
}

void audioFree(themeSound * aThemeSound) {
	if (audioActive) {
		memset(aThemeSound->sndbuffer, 0, aThemeSound->sndsize);
		GSPGPU_FlushDataCache(aThemeSound->sndbuffer, aThemeSound->sndsize);
		linearFree(aThemeSound->sndbuffer);
	}
}

void audio_stop(void){
	if (audioActive) {
		csndExecCmds(true);

		CSND_SetPlayState(0x8, 0);
		CSND_SetPlayState(0x9, 0);
		CSND_SetPlayState(0x10, 0);

		csndExecCmds(true);

		audioFree(&themeSoundBGM);
		audioFree(&themeSoundMove);
		audioFree(&themeSoundSelect);
		audioFree(&themeSoundBoot);
		audioFree(&themeSoundBack);

		csndExecCmds(true);
	}
}

char* concat(char *s1, char *s2)
{
	char *result = malloc(strlen(s1)+strlen(s2)+1);
	//in real code you would check for errors in malloc here.
	//perhaps this should be done?
	strcpy(result, s1);
	strcat(result, s2);
	return result;
}

void loadThemeSoundOrDefault(char * filename, themeSound * aThemeSound, int channel) {
	char * themePath = currentThemePath();

	char * filePath = concat(themePath, filename);
	if (fileExists(filePath, &sdmcArchive)) {
		audio_load(filePath, aThemeSound);
		aThemeSound->channel = channel;
	}
	else {
		char * defaultFilePath = concat(defaultThemePath, filename);
		if (fileExists(defaultFilePath, &sdmcArchive)) {
			audio_load(defaultFilePath, aThemeSound);
			aThemeSound->channel = channel;
		}
		free(defaultFilePath);
	}
	free(filePath);
}

void initThemeSounds() {
	if (audioActive) {
//		logTextP("Load BGM", "/bootlog.txt", true);
		loadThemeSoundOrDefault("BGM.bin", &themeSoundBGM, 8);
//		logTextP("Load move sound", "/bootlog.txt", true);
		loadThemeSoundOrDefault("movesound.bin", &themeSoundMove, 9);
//		logTextP("Load select sound", "/bootlog.txt", true);
		loadThemeSoundOrDefault("selectsound.bin", &themeSoundSelect, 10);
//		logTextP("Load back sound", "/bootlog.txt", true);
		loadThemeSoundOrDefault("backsound.bin", &themeSoundBack, 10);
	}
}

void startBGM() {
	if (audioActive) {
//		logTextP("Play music", "/bootlog.txt", true);
		audioPlay(&themeSoundBGM, true);
	}
}

void playBootSound() {
	if (audioActive) {
		loadThemeSoundOrDefault("bootsound.bin", &themeSoundBoot, 10);
		audioPlay(&themeSoundBoot, false);
	}
}

void waitForSoundToFinishPlaying(themeSound * aThemeSound) {
	if (waitForSounds && aThemeSound->loaded) {
		u8 playing = 0;
		csndIsPlaying(aThemeSound->channel, &playing);
		while (playing == 1) {
			csndIsPlaying(aThemeSound->channel, &playing);
		}
	}
}
