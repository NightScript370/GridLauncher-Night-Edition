#include "logText.h"
#include <stdio.h>
#include <string.h>

void logTextP(char *text, char const * path, bool append) {
	char * mode;

	if (append)
		mode = "a";
	else
		mode = "w";

	FILE* fSave = fopen( path, mode );
	if (fSave != NULL) {
		fprintf(fSave, "%s\n", text);
	}
	fclose(fSave);
}

void logText(char *text) {
	char const * path = "sdmc:/log.txt";
	logTextP(text, path, true);
}
