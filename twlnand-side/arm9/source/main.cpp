/*-----------------------------------------------------------------
 Copyright (C) 2005 - 2013
	Michael "Chishm" Chisholm
	Dave "WinterMute" Murphy
	Claudio "sverx"

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

------------------------------------------------------------------*/
#include <nds.h>
#include <nds/fifocommon.h>
#include <stdio.h>
#include <fat.h>
#include <sys/stat.h>
#include <limits.h>
#include <nds/disc_io.h>

#include <string.h>
#include <unistd.h>

#include "nds_loader_arm9.h"

#include "inifile.h"

using namespace std;

bool logEnabled = false;

//---------------------------------------------------------------------------------
void stop (void) {
//---------------------------------------------------------------------------------
	while (1) {
		swiWaitForVBlank();
	}
}

std::string ReplaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
}

std::string filename;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	if (fatInitDefault()) {
		CIniFile bootstrapini( "sd:/_nds/nds-bootstrap.ini" );

		scanKeys();
		if (keysHeld() & KEY_A) {
			filename = bootstrapini.GetString("NDS-BOOTSTRAP", "NDS_PATH","");
			filename = ReplaceAll(filename, "fat:/", "sd:/");
			runNdsFile (filename.c_str(), 0, 0);
		}

		runNdsFile ("sd:/_nds/bootstrap-hb.nds", 0, 0);

		// Subscreen as a console
		videoSetModeSub(MODE_0_2D);
		vramSetBankH(VRAM_H_SUB_BG);
		consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);	

		iprintf ("sd:/_nds/bootstrap-hb.nds\n");		
		iprintf ("not found.\n");		
		iprintf ("\n");		
		iprintf ("Press A to run without bootstrap.\n");	
		
		while (1) {
			scanKeys();
			if (keysHeld() & KEY_A) break;
		}
		
		filename = bootstrapini.GetString("NDS-BOOTSTRAP", "NDS_PATH","");
		filename = ReplaceAll(filename, "fat:/", "sd:/");
		runNdsFile (filename.c_str(), 0, 0);
		
		// Clear the screen
		iprintf ("\x1b[2J");
		
		iprintf ("Selected homebrew not found.\n");		
		iprintf ("\n");		
		iprintf ("Press B to return to\n");		
		iprintf ("HOME Menu.\n");		
	} else {
		
		// Subscreen as a console
		videoSetModeSub(MODE_0_2D);
		vramSetBankH(VRAM_H_SUB_BG);
		consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256, 15, 0, false, true);	

		iprintf ("fatinitDefault failed!\n");		
		iprintf ("\n");		
		iprintf ("Press B to return to\n");		
		iprintf ("HOME Menu.\n");		
	}
	
	while (1) {
		scanKeys();
		if (keysHeld() & KEY_B) fifoSendValue32(FIFO_USER_01, 1);	// Tell ARM7 to reboot into 3DS HOME Menu (power-off/sleep mode screen skipped)
	}

	return 0;
}
