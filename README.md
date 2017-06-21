# Bitx40 Pavel's blend #

BITX40 sketch for Raduino (Pavel's folk)

## Important notes: ##
* This sketch is based on the Allard Munters raduino's sketch (https://github.com/amunters/bitx40/), and I pretend to keep it that way, but has a few mods for my particular hardware/software you may found interesting, all mods and instructions for Allard's code apply.
* I don't longer use the Jason NT7S Si5351 library, I have switched to my own Si5351 library implementation, it's way more code efficient and has the same features (at least the ones we need)
* You have to download the new Si5351 library from here: https://github.com/pavelmc/Si5351mcu/ once you get it, just unzip it and put it on your library folder, you may want to rename the resultant folder from the default "Si5351mcu-master" to "Si5351mcu".

## Added features so far ##
* Smeter in the second line (need hardware mod, see smeter_mod.png file for details)
* Software controlled AGC (need the Smeter mod, plus some hardware mod, see agc_software.png file for details)

## TODO list ##
* Use the split-in-files feature of the Arduino IDE to organize the code.
* Shrink the code as much as possible (use of yatuli).
* Use more than one button with just ONE analog pin (use of bmux)
* Add memories.
* User's comments and wishes.

## Changelog ##

### v1.0 ###

Based on raduino_v1.15

* Switch from Jason NT7S Si5351 library to my own; result: ~25% of firmware space freed.
* Included the Smeter function.
* First version of the Software controlled AGC, need to tune the hardware part for a smooth action.
