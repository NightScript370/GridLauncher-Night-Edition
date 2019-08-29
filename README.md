# GridLauncher

[![Build Status](https://travis-ci.org/NightYoshi370/GridLauncher-Night-Edition.svg?branch=master)](https://travis-ci.org/NightYoshi370/GridLauncher-Night-Edition)

## Development Status
This is mostly dead. Updates will barely be made

#### Presentation

This fork of smealum's homebrew launcher adds many features including a grid layout, customisable colour schemes, wallpapers and much much more.
It is intended to be a drop-in replacement for the original launcher.

Big thanks to smealum for the original hax and launcher, suloku for the encouragement and help fixing bugs, and the GBATemp community for all the support and encouragement.

#### Usage

To use the grid launcher as your homebrew menu, simply rename the 3dsx executable to boot.3dsx and place it at the root of your SD card.

Select the "?" icon in the top right corner of the launcher to view help pages. Press START in hbmenu to reboot your console into home menu. Use the D-PAD, CIRCLE-PAD or the touchscreen to select an application, and press A or touch it again to start it.

#### Todo
- Ability to boot into .nds
- Support Rosalina
- Fix compilation on latest DevKitARM

#### Credits

- Smealum: Making the original HBLauncher
- GEMISIS, fincs, mtheall: hbmenu 1.0 backend
- Fluto, arkhandar: hbmenu 1.0 graphics
- dotjasp: regionfree icon
- mashers: Forking hbmenu 1.0 to make GridLauncher              
- NightYoshi370: Forking Gridlauncher to add a couple of features              
- Robz8: Making a TWL-side app for this.

#### How to compile

Firstly, you'll need to install [DevKitARM](https://devkitpro.org/) with the 3DS Portlibs, zlib and lippng            
Then, you'll need to clone the repo with `git clone https://github.com/maorninja/GridLauncher.git`             
Finally, run "make" in a terminal instance and if it works well, it should compile.

#### Please see the [README.md from the original launcher](https://github.com/smealum/3ds_hb_menu) for more technical details

More developement will be on [Mario Making Mods](http://mariomods.net/thread/120-gridlauncher-2-0#post1047) but there will still be some updates on [GBATemp](https://gbatemp.net/threads/gridlauncher-2-0.474881/)
