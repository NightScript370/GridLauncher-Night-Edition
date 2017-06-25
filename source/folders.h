#pragma once
#include <3ds.h>
#include "menu.h"

extern menu_s foldersMenu;
extern bool show3DSFolder;

char * currentFolder();
char * currentFolderName();

void buildFoldersList();
void setFolder(char * folderName);