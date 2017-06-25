#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <3ds.h>

#include "titles.h"
#include "smdh.h"
#include "menu.h"
#include "filesystem.h"
#include "MAGFX.h"
#include "config.h"
#include "regionfree.h"
#include "logText.h" 

extern int debugValues[100];

menu_s titleMenu;
titleBrowser_s titleBrowser;

#define maxIgnoreTitleIDs 100
bool ignoreTitleIDsLoaded = false;
char ignoreTitleIDs[maxIgnoreTitleIDs][17];
int numIgnoreTitleIDs;

bool titlemenuIsUpdating = false;
bool titleMenuInitialLoadDone = false;
bool preloadTitles = true;
bool titleLoadPaused = false;
bool titleLoadCancelled = false;

void titlesInit()
{
	amInit();
}

void titlesExit()
{
	amExit();
}

void initTitleInfo(titleInfo_s* ti, u8 mediatype, u64 title_id)
{
	if(!ti)return;

	ti->mediatype = mediatype;
	ti->title_id = title_id;
	ti->icon = NULL;
}

void freeTitleInfo(titleInfo_s* ti)
{
	if(!ti)return;

	ti->title_id = 0x0;
	if(ti->icon)free(ti->icon);
	ti->icon = NULL;
}

Result loadTitleInfoIcon(titleInfo_s* ti)
{
	if(!ti || ti->icon)return -1;

	ti->icon = malloc(sizeof(smdh_s));
	if(!ti->icon)return -1;

	Handle fileHandle;
	u32 archivePath[] = {ti->title_id & 0xFFFFFFFF, (ti->title_id >> 32) & 0xFFFFFFFF, ti->mediatype, 0x00000000};
	static const u32 filePath[] = {0x00000000, 0x00000000, 0x00000002, 0x6E6F6369, 0x00000000};
	Result ret = FSUSER_OpenFileDirectly(&fileHandle, ARCHIVE_SAVEDATA_AND_CONTENT, (FS_Path){PATH_BINARY, 0x10, (u8*)archivePath}, (FS_Path){PATH_BINARY, 0x14, (u8*)filePath}, FS_OPEN_READ, 0);
 
	if(ret)
	{
		free(ti->icon);
		ti->icon = NULL;
		return ret;
	}

	u32 bytesRead;
	ret = FSFILE_Read(fileHandle, &bytesRead, 0x0, ti->icon, sizeof(smdh_s));

	if(ret)
	{
		free(ti->icon);
		ti->icon = NULL;
	}

	FSFILE_Close(fileHandle);

	return ret;
}

bool application_filter(u64 tid)
{
	u32 tid_high = tid >> 32;
	return (tid_high == 0x00040010 || tid_high == 0x00040000 || tid_high == 0x00040002);
}

void initTitleList(titleList_s* tl, titleFilter_callback filter, u8 mediatype)
{
	if(!tl)return;

	tl->mediatype = mediatype;

	if(tl->filter) tl->filter = filter;
	else tl->filter = &application_filter;

	tl->num = 0;
	tl->titles = NULL;

	populateTitleList(tl);
}

void freeTitleList(titleList_s* tl)
{
	if(!tl)return;

	int i;
	for(i=0; i<tl->num; i++)freeTitleInfo(&tl->titles[i]);

	tl->num = 0;
	if(tl->titles)free(tl->titles);
	tl->titles = NULL;
}

int populateTitleList(titleList_s* tl)
{
	if(!tl)return 0;
	Result ret;

	u32 old_num = tl->num;

	u32 num;
	ret = AM_GetTitleCount(tl->mediatype, &num);

	if(ret)
	{
		freeTitleList(tl);
	}else if(num){
		// temp buffer is not ideal, but i like modularity
		u64* tmp = (u64*)malloc(sizeof(u64) * num);

		if(!tmp)
		{
			tl->num = 0;
			return 1;
		}

		ret = AM_GetTitleList(NULL, tl->mediatype, num, tmp);

		if(!ret)
		{
			int i;

			// apply tid filter
			for(i=0; i<num; i++)
			{
				if(!tl->filter(tmp[i]))
				{
					num--;
					tmp[i] = tmp[num];
					i--;
				}
			}

			if(tl->num != num || tl->mediatype == 2)
			{
				freeTitleList(tl);

				tl->num = num;

				if(tl->num) tl->titles = malloc(sizeof(titleInfo_s) * tl->num);

				if(tl->titles)
				{
					for(i=0; i<tl->num; i++)
					{
						initTitleInfo(&tl->titles[i], tl->mediatype, tmp[i]);
					}
				}
			}
		}else tl->num = 0;

		free(tmp);
	}

	return old_num != tl->num;
}

titleInfo_s* findTitleList(titleList_s* tl, u64 tid)
{
	if(!tl)return NULL;

	// special case : gamecard mediatype with 0 tid
	if(!tid && tl->mediatype == 2 && tl->num)return &tl->titles[0];

	int i;
	for(i=0; i<tl->num; i++)
	{
		if(tl->titles[i].title_id == tid)return &tl->titles[i];
	}

	return NULL;
}

titleInfo_s* findTitleBrowser(titleBrowser_s* tb, u8 mediatype, u64 tid)
{
	if(!tb || mediatype > 2)return NULL;

	return findTitleList(&tb->lists[2-mediatype], tid);
}

void initTitleBrowser(titleBrowser_s* tb, titleFilter_callback filter)
{
	if(!tb)return;

	int i;
	for(i=0; i<3; i++)
	{
		initTitleList(&tb->lists[i], filter, (u8)2-i);
	}

	tb->total = 0;
	tb->nextCheck = 0;
	tb->selectedId = 0;
	tb->selected = NULL;
}

bool titleBrowserRefreshed = false;

void refreshTitleBrowser(titleBrowser_s* tb) {
	if (titleBrowserRefreshed) return;
	titleBrowserRefreshed = true;

	tb->total = 0;
	tb->selectedId = 0;

	int i;

	for(i=0; i<3; i++) {
		freeTitleList(&tb->lists[i]);
		populateTitleList(&tb->lists[i]);
		tb->total += tb->lists[i].num;
	}
}

void getIgnoredTitleIDs() {
	if (!ignoreTitleIDsLoaded) {
		ignoreTitleIDsLoaded = true;
		numIgnoreTitleIDs = 0;
		int maxTextSize = 20 * maxIgnoreTitleIDs;
		char ignoreText[maxTextSize];

		FILE *fp = fopen(ignoredTitlesPath, "r");

		if (fp != NULL) {
			fgets(ignoreText, maxTextSize, fp);
			char * split;
			split = strtok(ignoreText, ",");
			while (split != NULL) {
				strcpy(ignoreTitleIDs[numIgnoreTitleIDs], split);
				numIgnoreTitleIDs++;

				split = strtok(NULL, ",");
			}
		}
		fclose(fp);
	}
}

void saveIgnoredTitleIDs() {
	int n = 0;

	menuEntry_s * me = titleMenu.entries;
	while (me) {
		if (!me->showTick) {
			n++;
		}

		me = me->next;
	}

	char ignored[n*20];
	strcpy(ignored, "");

	int c=0;

	me = titleMenu.entries;
	while (me) {
		if (!me->showTick) {
			char sTitleId[17];
			sprintf(sTitleId, "%llu", me->title_id);
			strcat(ignored, sTitleId);

			if (c < n-1) {
				strcat(ignored, ",");
			}

			c++;
		}

		me = me->next;
	}

	FILE* fSave = fopen(ignoredTitlesPath, "w" );
	if (fSave != NULL) {
		fputs(ignored, fSave);
	}
	fclose(fSave);

	ignoreTitleIDsLoaded = false;
	getIgnoredTitleIDs();
}

bool titleIgnored(u64 titleID) {
	char sTitleId[17];
	sprintf(sTitleId, "%llu", titleID);

	int iIgnoreTitleID;
	for (iIgnoreTitleID=0; iIgnoreTitleID<numIgnoreTitleIDs; iIgnoreTitleID++) {
		if (strcmp(sTitleId, ignoreTitleIDs[iIgnoreTitleID]) == 0) {
			return true;
		}
	}

	return false;
}

//#include "MAFontRobotoRegular.h"

void addTitleBannerImagePathToMenuEntry(menuEntry_s *me, u64 title_id) {
	char sTitleID[32];
	sprintf(sTitleID, "%llu", title_id);
	addBannerPathToMenuEntry(me->bannerImagePath, titleBannersPath, sTitleID, &me->bannerIsFullScreen, &me->hasBanner);
}

void populateTitleMenu(menu_s* aTitleMenu, titleBrowser_s *tb, bool filter, bool forceHideRegionFree, bool setFilterTicks) {
	getIgnoredTitleIDs();

	if (!aTitleMenu) {
		return;
	}

	aTitleMenu->entries=NULL;
	aTitleMenu->numEntries=0;
	aTitleMenu->selectedEntry=0;
	aTitleMenu->scrollLocation=0;
	aTitleMenu->scrollVelocity=0;
	aTitleMenu->scrollBarSize=0;
	aTitleMenu->scrollBarPos=0;
	aTitleMenu->scrollTarget=0;
	aTitleMenu->atEquilibrium=false;

	/*
	If there is no game cart in right now but there needs to be one shown if a cart is inserted,
	add a hidden dummy entry to the menu. This entry will be updated with the cart details
	and shown if a cart is inserted.
	*/

	if (!regionFreeGamecardIn && !forceHideRegionFree) {
		static menuEntry_s rfEntry;
		rfEntry.hidden = true;
		rfEntry.isTitleEntry = false;
		rfEntry.isRegionFreeEntry = true;
		rfEntry.isShortcut = false;
		addMenuEntryCopy(aTitleMenu, &rfEntry);
	}

	int i;

	for(i=0; i<3; i++) {
		const titleList_s* tl = &tb->lists[i];
		titleInfo_s *titles = tl->titles;

		int titleNum;
		int count = tl->num;

		for (titleNum = 0; titleNum < count; titleNum++) {
//			MADrawText(GFX_TOP, GFX_LEFT, 0, 0, "Loading", &MAFontRobotoRegular10, 255, 255, 255);

			while (titleLoadPaused) {
//				MADrawText(GFX_TOP, GFX_LEFT, 15, 0, "Paused", &MAFontRobotoRegular10, 255, 255, 255);
				svcSleepThread(1000000000ULL);

				if (titleLoadCancelled) {
					return;
				}
			}

			if (titleLoadCancelled) {
				return;
			}

			/*
			If a game card is not inserted, we do not need to add the first title.
			This is because a dummy entry for the cart was already added above.
			*/

			if (i==0 && titleNum == 0 && (!regionFreeGamecardIn || forceHideRegionFree)) {
				continue;
			}

			titleInfo_s aTitle = titles[titleNum];

			if (filter && titleIgnored(aTitle.title_id)) {
				continue;
			}

			if (!aTitle.icon) {
				loadTitleInfoIcon(&aTitle);
			}

			if (!aTitle.icon) {
				continue;
			}

			static menuEntry_s me;

			me.hidden = false;
			me.isTitleEntry = false;
			me.isRegionFreeEntry = false;
			me.isShortcut = false;
			me.bannerImagePath[0] = '\0';

			addTitleBannerImagePathToMenuEntry(&me, aTitle.title_id);
//			addTitleBannerImagePathToMenuEntry(&me, 1125899907535104);


			/*
			If adding the title for the inserted cart, set its isRegionFreeEntry flag.
			We need to do this here because it won't have been done by adding the dummy
			entry above.
			*/

			if (i==0 && titleNum == 0 && regionFreeGamecardIn) {
				me.isRegionFreeEntry = true;
			}

			extractSmdhData(aTitle.icon, me.name, me.description, me.author, me.iconData);

			me.title_id = aTitle.title_id;
			me.mediatype = aTitle.mediatype;

			if (me.title_id == 1125968626461184) {
				strcpy(me.name, "System Transfer");
				strcpy(me.description, "System Transfer");
			}

			if (setFilterTicks) {
				if (titleIgnored(aTitle.title_id)) {
					me.showTick = NULL;
				}
				else {
					me.showTick = &trueBool;
				}
			}
			else {
				me.showTick = NULL;
			}

			addMenuEntryCopy(aTitleMenu, &me);

//				titleMenu->numEntries = titleMenu->numEntries + 1;
			updateMenuIconPositions(aTitleMenu);

			if (titleLoadCancelled) {
				return;
			}
		}
	}

	updateMenuIconPositions(aTitleMenu);
}

//titleInfo_s* getTitleWithID(titleBrowser_s* tb, u64 tid) {
//	int i;
//	for(i=0; i<3; i++) {
//		const titleList_s* tl = &tb->lists[i];
//		titleInfo_s *titles = tl->titles;
//
//		int titleNum;
//		int count = tl->num;// sizeof(titles) / sizeof(titleInfo_s);
//
//		for (titleNum = 0; titleNum < count; titleNum++) {
//			titleInfo_s * aTitle = &titles[titleNum];
//			if (aTitle->title_id == tid) {
//				return aTitle;
//			}
//		}
//
//	}
//
//	return NULL;
//}

Handle titleLoadThreadRequest; 
Thread titleLoadThread;

u32 *titleLoadThreadStack;
#define STACKSIZE (4 * 1024)

#include <malloc.h>

titleBrowser_s * reloadTitleBrowser;
menu_s * reloadTitleMenu;
bool filterTitlesWhenLoading;
bool forceHideRegionFreeWhenLoading;
bool titleThreadNeedsRelease = false;
bool populateFilterTicksWhenLoading;

void resumeTitleLoading() {
	titleLoadPaused = false;
}

void pauseTitleLoading() {
	titleLoadPaused = true;
}

void cancelTitleLoading() {
	titleLoadCancelled = true;
	titleLoadPaused = false;
	titleMenuInitialLoadDone = false;
	titlemenuIsUpdating = false;
//	svcSignalEvent(titleLoadThreadRequest);
}

void titleLoadFunction() {
	refreshTitleBrowser(reloadTitleBrowser);
	clearMenuEntries(reloadTitleMenu);
	populateTitleMenu(reloadTitleMenu, reloadTitleBrowser, filterTitlesWhenLoading, forceHideRegionFreeWhenLoading, populateFilterTicksWhenLoading);

	if (titleLoadCancelled) {
//		logText("Returned from cancelled populate function");
	}
	else {
//		logText("Title load completed");
		titlemenuIsUpdating = false;
		titleMenuInitialLoadDone = true;
	}
}

void titleLoadThreadFunction(void *arg) {
	while (1) {
		svcWaitSynchronization(titleLoadThreadRequest, U64_MAX);
		svcClearEvent(titleLoadThreadRequest);

		titleLoadFunction();

//		logText("Returned from titleLoadFunction");

		if (titleLoadCancelled) {
			titleLoadCancelled = false;
//			logText("Cancelled");
		}

		svcExitThread();
	}
}

void releaseTitleThread() {
	svcCloseHandle(titleLoadThreadRequest);
	threadFree(titleLoadThread); 
	titleThreadNeedsRelease = false;
}

void updateTitleMenu(titleBrowser_s * aTitleBrowser, menu_s * aTitleMenu, char * titleText, bool filter, bool forceHideRegionFree, bool setFilterTicks) {
	if (titlemenuIsUpdating) {
		return;
	}

	titlemenuIsUpdating = true;
	titleMenuInitialLoadDone = false;
	titleLoadCancelled = false;

	if (titleThreadNeedsRelease) {
		releaseTitleThread();
	}

	reloadTitleMenu = aTitleMenu;
	reloadTitleBrowser = aTitleBrowser;
	filterTitlesWhenLoading = filter;
	forceHideRegionFreeWhenLoading = forceHideRegionFree;
	populateFilterTicksWhenLoading = setFilterTicks;

	if (aTitleMenu->numEntries > 0) {
		gotoFirstIcon(aTitleMenu);
	}

	if (preloadTitles) {
		titleThreadNeedsRelease = true;

		svcCreateEvent(&titleLoadThreadRequest,0);
		titleLoadThread = threadCreate(titleLoadThreadFunction, 0, STACKSIZE/4, 0x3f, 0, true); 
		if (titleLoadThread != NULL) { 
			svcSignalEvent(titleLoadThreadRequest);
		}
		else {
			titleLoadFunction();
		}
	}
	else {
		drawDisk("Loading titles");
		gfxFlip();
		titleLoadFunction();
	}
}

void toggleTitleFilter(menuEntry_s *me, menu_s * m) {
	if (me->showTick == NULL) {
		me->showTick = &trueBool;
	}
	else {
		me->showTick = NULL;
	}
}

void createTitleInfoFromTitleID(u64 title_id, u8 mediaType, titleInfo_s *info) {
	if (!info)
		return;

	info->title_id = title_id;
	info->mediatype = mediaType;
}



