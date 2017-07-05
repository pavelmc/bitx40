# Bitx40 sketch, Pavel's blend #

BITX40 sketch for Raduino (Pavel's folk)

## Important notes: ##
* This sketch is based on the Allard Munters raduino's sketch (https://github.com/amunters/bitx40/), and I pretend to keep it that way, but has a few mods for my particular hardware/software you may found interesting, all mods and instructions for Allard's code apply.
* I don't longer use the Jason NT7S Si5351 library, I have switched to my own Si5351 library implementation, it's way more code efficient and has the same features (at least the ones we need)
* You have to download the new Si5351 library from here: https://github.com/pavelmc/Si5351mcu/ once you get it (click on the green "Clone or download" button), just unzip it and put it on your library folder, you may want to rename the resultant folder from the default "Si5351mcu-master" to "Si5351mcu".

## Added features so far ##
* LCD RX meter in the second line (relative power level in RX, not accurate in ANY sense, need hardware mod: see smeter_mod.png file for details, _You can improve the sensitivity by using germanium or schottky diodes instead the showed 1N4148_)
* TX power meter, using the same RX meter functions (need hardware mod: see tx_meter_mod.png and tx_meter_mod.md file for adjusting)
* Software controlled AGC (need the smeter mod, plus the one shown in agc_software.png file) _You may notice a little buzzing noise when AGC kicks in, the buzz is via the MCU not the analog chain so we have to live with it_

Check the [Operating Instructions](Operating_Instructions.md) file inside this code repository for mods and tips about they.

## TODO list (with no particular order) ##
* Use the split-in-files feature of the Arduino IDE to organize the code.
* Shrink the code as much as possible (code optimizations & use of yatuli?).
* Use more than one button with just ONE analog pin instead the multi-click strategy (use of bmux?)
* Get rid of a lot of blocking delays.
* Add CAT support (after wipe/smooth the blocking delays)
* Add memories.
* User's comments and wishes.

## Changelog ##

### v1.4 ###

* Update to catch up with the features added in the Raduino v1.17.1 from Allard's code (CW SPOT and bug fixes)
* Upgraded the operations instructions.
    - More user friendly version with embedded images.
    - Add instructions for the S-meter, AGC and TX-power mods details and tricks.
* Moved all images to its own folder "images".

### v1.3 ###

* Fix for labels on second line not matching real functions (bug introduced during optimization in v1.2)
* Fix for the diode drawn backwards in the smeter_mod.png file.
* Fix for **true** calibration of the Si5351, not just a freq shift on different modes, now it applies a real calibration inside the lib (USB remains by shifting the VFO, LSB is truly calibrated)
* We don't longer support the legacy CAL_BUTTON in A2, as every body knows how to calibrate via the FBUTTON button during runtime.
* More code optimizations.
* Added the image files for the AGC mod and the TX power meter.

#### WARNING ! ####

After this version you will notice that the calibration procedure is different, after adjusting the calibration write down your calibration and keep it for future reference.

### v1.2 ###

* Several code optimizations & code shrinking.
* Added the smeter_mod.png file.

### v1.1 ###

* Included the Smeter function.
* First version of the Software controlled AGC, need to tune the hardware part for a smooth action.

### v1.0 ###

Based on raduino_v1.15

* Switch from Jason NT7S Si5351 library to my own; result: ~25% of firmware space freed.
* Included the Smeter function.
* First version of the Software controlled AGC, need to tune the hardware part for a smooth action.
