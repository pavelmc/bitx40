# bitx40
BITX40 sketch for Raduino

This sketch is intended as universal, standard Raduino software that should always work, even on a unmodified out-of-the-box BITX40 + raduino board. Without any hardware modifications the sketch provides the standard basic LSB functionality.
The sketch provides additional features such as USB, CW, RIT/SPLIT etc., but these will only become functional when the related (minimal) hardware mods are made. See the operating and modification instructions at https://github.com/amunters/bitx40/blob/master/operating%20instructions for full details.

Important:
This sketch is confirmed working OK with the si5351 library v2.01.
Older library versions v1.xx definitely don't work (compilation errors).
Newer versions (v2.02, v2.03, v2.04) compile OK but give strong pops/clicks in the speaker during tuning.
This issue is still under investigation. See https://github.com/etherkit/Si5351Arduino/issues/44
# Until this is solved, please use v2.01 of the si5351 library.
The si5351 library v2.01 can be downloaded from https://github.com/etherkit/Si5351Arduino/releases/tag/v2.0.1


Revision record

v1.14.1
- Corrected small bug in v1.14 that caused slight ticking noise when the radio was left idle.

v1.14
- added VFO A/B monitoring mode (press Function Button 5 times)
- use RX-offset instead of TX-offset in CW mode - the display now shows the correct TX frequency in CW
- changed the way to switch from CW to SSB mode: press PTT to return to SSB mode (tks Willy W1LY)
- restored the functionality for old way calibration method
- simplified the method for sidetone setting: hold key down to hear sidetone
- improved the display during "fast scan" at tuning pot limits (tks Paul KC8WBK)

v1.13
- added frequency scanning capability
- added functionality so that the user can set the CW timout value via the SETTINGS menu
- added decimal point to the VFO for better readability, like so: A 7.123.4 LSB
- simplified calibration routine and cleaned up the code to preserve memory space 

v1.12
- improved responsiveness of Function Button for better user experience
- corrected Tuning Range and SideTone setting procedures

v1.11
- added menu beeps (needs CW sidetone to be wired up)
- corrected a minor bug that "TX" is always shown when PTT-SENSE line has not been installed

v1.10
- added CW functionality (for straight morse key). This function can also be used just for tuning up.
  This requires the CW-CARRIER line connected to Raduino output D6 (connector P3, pin 15).
  (see https://github.com/amunters/bitx40/blob/master/CW-CARRIER%20wiring.png for wiring instructions).
  The morse key itself shall be connected to Raduino pin A1 (brown wire).
  Both sidebands (CWU or CWL) are available for CW operation.
- Semi break-in for CW
  This requires the TX-RX line from Raduino output D7 (connector P3, pin 16) to override the existing PTT
  switch using a NPN transistor. 
  (see https://github.com/amunters/bitx40/blob/master/TX-RX%20line%20wiring.png for wiring instructions)  
- CW side-tone
  This requires some wiring from Raduino output D5 (connector P3, pin 14) to the speaker.
  (see https://github.com/amunters/bitx40/blob/master/sidetone%20wiring.png for wiring instructions).
  The desired sidetone pitch can be set using the Function Button in the SETTINGS menu.
- Frequency tuning is disabled during TX (to prevent flutter or "FM-ing" during TX).
  This requires the PTT SENSE line connected to pin A0 (black wire).
  (see https://github.com/amunters/bitx40/blob/master/PTT%20SENSE%20wiring.png for wiring instructions).

v1.09
- added RIT (SPLIT) functionality. This requires a PTT sense line connected to pin A0 (black wire)
  (see https://github.com/amunters/bitx40/blob/master/PTT%20SENSE%20wiring.png for wiring instructions)
- simplified tuning range setting in SETTINGS menu
- less frequent EEPROM writes so that EEPROM will last longer

v1.08
- mode (LSB or USB) of each VFO A and B is now also memorized
- the BITX status (VFO frequencies, modes) is now stored in EEPROM every 10 seconds and retrieved during start up
- a warning message "uncalibrated" is displayed when calibration data has been erased

v1.07
- Added functionality via the Function Button:
  Use a pusbutton to momentarily ground pin A3 (orange wire). Do NOT install an external pull-up restistor!
- dual VFO capability (RIT is not yet working though)
- LSB/USB mode
- Settings menu for calibration, tuning range, VFO drive level
- All settings are stored in EEPROM and read during startup

v1.06
- no functional changes in this version, only improved the updateDisplay routine (Jack Purdum, W8TEE)
  (replaced fsprint commmands by str commands for code space reduction)

v1.05
- in setup(): increase the VFO drive level to 4mA to kill the birdie at 7199 kHz (Allard, PE1NWL)
  (4mA seems the optimum value in most cases, but you may try different drive strengths for best results -
  accepted values are 2,4,6,8 mA)

v1.04
- Sketch now allows the (optional) use of a 10-turn potentiometer for complete band coverage (Allard, PE1NWL)
- Standard settings are still for a 1-turn pot.
- But if you want to use a 10-turn pot instead, change the values for 'TUNING_RANGE' and 'baseTune'
  in lines 189 and 190 to your liking
  
v1.03
- improved tuning "flutter fix" (Jerry, KE7ER)

v1.02
- fixed the calibration routine (Allard, PE1NWL)
- fetch the calibration correction factor from EEPROM at startup (Allard, PE1NWL)
- added some changes to comply with si5351 library v2. (Allard, PE1NWL)

v1.01
- original BITX40 sketch (Ashhar Farhan)
