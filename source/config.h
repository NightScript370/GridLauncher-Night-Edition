#ifndef CONFIG_H
#define CONFIG_H

#include <3ds.h>
#include "menu.h"
#include "button.h"

#ifdef __cplusplus
extern "C" {
#endif

	extern menu_s settingsMenu;
	extern menu_s themesMenu;
	extern menu_s waterMenu;
	extern menu_s themeSettingsMenu;
	extern menu_s gridSettingsMenu;

	extern bool settingsMenuNeedsInit;

	extern bool showRegionFree;
	extern bool sortAlpha;
	extern bool showAppBackgrounds;
	extern bool wrapScrolling;

	extern bool randomTheme;

	extern buttonList themeButtons;

#define configTypeMain 10
#define configTypeTheme 20

#define logoTypeDefault 1
#define logoTypeCompact 2
#define logoTypeClassic 3
#define logoTypeNone 4

extern int logoType;

	void setConfigString(char* key, char* value, int configType);
	void setConfigBool(char* key, bool value, int configType);
	void setConfigInt(char* key, int value, int configType);

	bool getConfigBoolForKey(char* key, bool defaultValue, int configType);
	int getConfigIntForKey(char* key, int defaultValue, int configType);
	char * getConfigStringForKey(char* key, char* defaultValue, int configType);

	void saveConfig();
	void saveConfigWithType(int configType);

	void initConfigMenu();
	void handleSettingsMenuSelection(menu_s *m);
	void addSettingsMenuEntry(char * name, char * description, u8 * icon, bool * showTick, menu_s *m,  void (*callback)(), void *callbackObject1, void *callbackObject2);

	void buildThemesList();
	void setTheme(char * themeName);
	void randomiseTheme();

#ifdef __cplusplus
}
#endif

#endif
