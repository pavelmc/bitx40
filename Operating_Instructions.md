# User instructions for Bitx Pavel's blend #
(from Raduino_v1.17.1 code with minor tweaks.)

## WARNING! ##
After the version update all calibration data, drive level settings, etc will be erased.
Before updating note down your cal values etc. After the update use the Function Button to set them back again.

## 10-TURN TUNING POT ##
The default tuning range with the standard supplied 1-turn tuning pot is only 50 kHz. If you install a 10-turn pot instead you can extend the tuning range for full 40m band coverage.

![Using 10 turns pot](./images/Vishay%20100K%2C%2010-turn%20pot%20wire%20up.jpg?raw=true)

You can use the Function Button to change that, go to the SETTINGS menu and set the desired tuning range, or directly on code before compiling and upload, check the code around line 1768 and adapt the value to your needs.

### Don't have a Function Button yet? ###
If you don't install a pushbutton then the basic functions will still work, of course you will miss the dual VFO, RIT, SPLIT, USB and CW capability then, see below for instructions.

## PIN LAYOUT ##
Here you can check the pin layout for reference for all the mods in this file.

![Raduino Pinout for referece](./images/raduino_pin_layout.png?raw=true)

## PTT SENSE WIRING ##
Connect pin A0 (connector P1, black wire) via a 10K resistor to the output of U3 (LM7805 regulator) on the BITX40 board.

![PTT sense mod](./images/PTT%20SENSE%20wiring.png?raw=true)

When the PTT is not pressed (RX mode), the regulator will be off, so pin A0 will see 0V (LOW). When the PTT is pressed (TX mode), the regulator will be on, so pin A0 will see +5V (HIGH).

The PTT SENSE is required for the CW, RIT, SPLIT functionality, and for disabling frequency updating during TX
(to prevent "FM-ing"). If you don't install the PTT sense, LSB and USB operation will still work normally.

## CONNECTING A MORSE KEY or 'TUNE' BUTTON ##
A morse key (or electronic keyer) can be connected to Raduino pin A1 (connector P1, brown wire). When the key is up (open) pin A1 will be HIGH. When the key is down (closed, shorted to ground) pin A1 will be LOW, and a carrier will be transmitted.

You could also wire up a simple push button instead of connecting a morse key. The generated CW carrier can be used for tuning up your antenna. In that case please note that you will be transmitting a carrier at full duty cycle, therefore don't keep the tune button pressed for too long to prevent overheating the final!

## CW-CARRIER WIRING ##
This is required for CW operation. Connect a wire from Raduino ouput D6 (connector P3, pin 15), via a 10K series resistor, to the input of the mixer.

![CW carrier mod](./images/CW-CARRIER%20wiring.png?raw=true)

When the key is down ouput D6 will be HIGH. This injects some DC current into the mixer so that it becomes unbalanced. As a result a CW carrier will be generated.

**Note:** If the carrier is not generated at full output power, you may need to reduce the 10K series resistor to a lower value for more drive. However try to keep it as high as possible to keep a clean CW signal. Never use a resistor less than 1K!

The CW-CARRIER is only required for CW functionality. If you don't install this line everything else will still work normally.

## CW SIDE TONE WIRING ##
A side tone is available at Raduino output D5 (connector P3, pin 14). This signal can be fed to the speaker/headphones in parallel to the output from the existing audio amplifier.

![CW side tone mod](./images/sidetone%20wiring.png?raw=true)

The desired side tone pitch can be set using the Function Button in the SETTINGS menu. The CW-side tone is only used for CW operation. If you don't install this line everything else will still work normally.

## TX-RX WIRING ##
This is required for CW operation.

![TX-RX wiring mod](./images/TX-RX%20line%20wiring.png?raw=true)

When the key is down output D7 (connector P3, pin 15) will be HIGH. It will only go LOW again when the key has been up for at least 350 ms (this timeout value can be changed via the SETTINGS menu).

This signal is used to drive an NPN transistor which is connected in parallel to the existing PTT switch, so that it will bypass the PTT switch during CW operation. As as result the relays will be activated as long as D7 is HIGH.

***Suggestion:** If you have a combined microphone/PTT connector, the PTT bypass transistor can be soldered directly on the back of it. The PTT bypass transistor may be used in the future for VOX functionality as well)*

## CW SPOT Button ##
Connect a momentary pushbutton between pin D4 (connector P3) and ground. Arduino's internal pull-up resistors are used, therefore do NOT install an external pull-up resistor!

When operating CW it is important that both stations transmit their carriers on the same frequency. When the SPOT button is pressed while the radio is in RX mode, the RIT will be turned off and the sidetone will be generated (but no carrier will be transmitted).

While the SPOT button is held pressed, the radio will temporarily go into "FINE TUNE" mode, allowing the VFO to be set at 1Hz precision. This feature works also in SSB mode (except that no sidetone will be generated then). Tune the VFO so that the pitch of the received CW signal is equal to the pitch of the CW Spot tone. By aligning the CW Spot tone to match the pitch of an incoming station's signal, you will cause your signal and the other station's signal to be exactly on the same frequency (zero beat).

*The SPOT button is just an extra tuning aid and is not strictly required for CW - if you don't install it, CW operation is still possible*

## S-METER ##

![S-meter mod](./images/smeter_mod.png?raw=true)

You need to build the circuit described on the picture above, once you have wired it you then need to route the output wire to the A6 input of the raduino.

***Warning!:** Under no circumstance omit the 1k5 resistor in series with the output wire or you are risking your arduino in the presence of strong signals, being said that I need to say that it's value is not critical, any value between 1 and 2.2k will work.*

***Suggestion:** For a more smooth operation is advised to put a capacitor of around 10nf (not critical, 1nF to 33nF will do it) in parallel with the A6 pin to ground.*

The signals showed by the S-meter in the second line has no reference against a typical s-meter, it's just a relative strength meter and in my hardware will activate (show the first char in the second line) with signals above about S7 in my old Yaesu FT-747GX.

## SIMPLE SOFTWARE AGC ##
This mod needs the S-meter mod to work.

![AGC mod](./images/agc_software.png?raw=true)

This mod will make help you to attenuate some strong signals from nearby TX stations automagically, it's a simple AGC with a limited range due to the architecture of the bitx40, do not expect it to work like pro rigs!

We have a internal measure of the input signals thanks to the S-METER mod, then why not to use that for taking down the gain of the RX chain when its needed; that what this mod makes. It takes a mean with some latency of the input signals and take it to the outside world via a PWM output in pin 3.

Sadly the arduino platform uses the PWM at audible frequencies and to make that don't disturb our reception you needs to isolate the PWM output from the RF board.
That's why in the circuit in the image mentioned before you need to route the D3 output and the ground return from the optocoupler DIRECTLY from the arduino board, the LED diode is a common 5mm red diode, it's just there because its fun to watch (increase the resistor to the double ~680 if you plan to remove the LED), hell you can even put in in your front panel to know when the bitx40 board has the AGC applied.

## TX-METER ##
The TX meter mod needs the S-METER mod.

![TX-meter mod](./images/tx_meter_mod.png?raw=true)

The TX-meter mod is just a simple current probe on the TX line, the problem is that you need to adjust the toroid sampling to get the correct levels for the Arduino.

I used a ferrite-black toroid borrowed from a broken electronic light bulb, I don't know what AL or u it is, but that doesn't matter so much as log as you have a voltmeter, let's see how:

* Built the circuit above, but don't connect the output wire to the circuit, instead connect it to a volt meter. *(check below for ballpark values)*
* Get into transmission mode and speak loudly or make a long hhhheeeeellllllooooooo! and check what it the highest voltage registered.
* If you voltage is in the range of 4 to 5 volts you are set, just connect it to the point marked TX on the s-meter mod and you are ready to rock.
* If your voltage is _higher_ than that, replace the resistor in parallel with the winding on the toroid for one with a _lower_ value one and repeat the loud speaking and voltage check, you will see how the voltage is going down; repeat until your voltage is between 4 and 5 volts.
* If your voltage is _lower_ than that, replace the resistor in parallel with the winding on the toroid for one with a _higher_ value and repeat the loud speaking and voltage check, you will see how the voltage is going up; repeat until your voltage is between 4 and 5 volts.
* In the step above if you don't get enough voltage even without the resistor you need to change the toroid or add more turns to it.

I used a R of 270 ohms in parallel with this toroid, but experimenting I can live also with 3 turns and 47 ohms in parallel also, so you need to adjust for your particular toroid windings and resistor.

After measuring my toroid, I get this data: (for reference)

* Outer diameter: 10 mm
* Inner diameter: 5.5 mm
* Core tick (width): 4.75 mm
* Turns: 13
* Inductance: 264.3 uH (L-C-CE meter Kit)
* Calculated AL: 1563.91 nH/N^2 (Mini Ring Core Calculator)
* Calculated ui: 2753.6 (Mini Ring Core Calculator)

## FUNCTION BUTTON WIRING ##
Connect a momentary pushbutton between pin A3 (connector P1, orange wire) and ground. Arduino's internal pull-up resistors are used, therefore do NOT install an external pull-up resistor!

## FUNCTION BUTTON USAGE ##

Several functions are available with just one pushbutton, in normal operation mode:

* 1 short press - toggle VFO A/B.
* 2 short presses - RIT on (PTT sense is required for this function) (press FB again to switch RIT off)
* 3 short presses - toggle SPLIT on/off (PTT sense is required for this function)
* 4 short presses - switch mode (rotate through LSB-USB-CWL-CWU)
* 5 short presses - start frequency SCAN mode.
* 6 short presses - start VFO A/B monitoring mode.
* long press (> 1 second) is VFO A=B.

When you press the Fbutton VERY long (>3 seconds) you will enter the SETTINGS menu; In the SETTINGS menu:

**1 short press** - VFO frequency calibration in LSB mode

  - use another transceiver to generate a carrier at a known frequency (for example 7100.0 kHz)
    (or ask a friend to transmit a carrier at a known frequency)
  - before going into the calibration mode, first set the VFO to 7100.0 kHz in LSB mode.
    (the received signal may not yet be zero beat at this point)
  - go into the LSB calibration mode (1 short press)
  - using the tuning pot, adjust the LSB offset for exactly zero beat.
  - press the Function Button again to save the setting.

**2 short presses** - VFO frequency calibration in USB mode

  - USB calibration depends on LSB calibration, so make sure that LSB calibration has been done first!
  - use another transceiver to generate a carrier at a known frequency (for example 7100.0 kHz)
    (or ask a friend to transmit a carrier at a known frequency)
  - before going into the calibration mode, first set the VFO to 7100.0 kHz in USB mode.
    (the received signal may not yet be zero beat at this point)
  - go into the USB calibration mode (2 short presses)
  - using the tuning pot, adjust the USB offset for exactly zero beat.
  - press the Function Button again to save the setting.

**3 short presses** - set VFO drive level in LSB mode

  - tune to 7199 kHz, on most BITX40 transceivers a strong birdie is heard in LSB mode.
  - give 3 short presses to the FB to enter the VFO drive level adjustment.
  - the default drive level in LSB mode is 4mA.
  - using the tuning pot, try different drive levels (2,4,6,8 mA) to minimize the strength of the birdie.
  - press the FB again to save the setting.

**4 short presses** - set VFO drive level in USB mode

  - tune to a weak signal.
  - give 4 short presses to the FB to enter the VFO drive level adjustment.
  - the default drive level in USB mode is 8mA.
  - using the tuning pot, try different drive levels (2,4,6,8 mA) for maximum signal to noise ratio.
  - press the FB again to save the setting.

    Extra Note: If the max. drive level of 8mA is still insufficient for USB mode, removal of C91 and C92 may help. These caps attenuate the VFO signal at higher frequencies. They're actually only needed for the analog VFO and can safely be removed if you use the Raduino DDS instead of the analog VFO.

**5 short presses** - set tuning range

  - the default tuning range is 50 kHz (7100-7150 kHz), this is OK for a standard 1-turn pot.
  - if you install a multi-turn pot instead you can extend the tuning range.
  - using the tuning pot, set the desired tuning range.
  - recommended value: 50 kHz for a 1-turn pot, 200 kHz for a 10-turn pot.
  - press the FB again to save the setting.

**6 short presses** - set CW paramaters (sidetone pitch, CW timeout)

 - the sidetone is generated.
 - using the tuning pot, set the desired sidetone pitch.
 - press the FB.
 - using the tuning pot, set the desired timeout value (ms)
 - press the FB again to save the settings.

**7 short presses** - set frequency SCAN parameters

 - using the tuning pot, set the desired lower frequency scan limit.
 - press the FB.
 - using the tuning pot, set the desired upper frequency scan limit.
 - press the FB.
 - using the tuning pot, set the desired scan step size.
 - press the FB.
 - using the tuning pot, set the desired scan step delay (also used for A/B monitoring mode)
 - press the FB again to save the settings.

**long press (>1 second)** - return to the NORMAL mode.

_**All user settings are stored in EEPROM and retrieved during startup.**_

When you keep the Fbutton pressed during power on all user settings will be erased and set back to "factory" values:

* VFO calibration (LSB): 0
* VFO offset (USB): 1500 Hz
* VFO drive level (LSB): 4mA
* VFO drive level (USB): 8mA
* Tuning pot range: 50 kHz
* Mode LSB for both VFO A and B
* CW side tone: 800 Hz
* CW timeout: 350 ms
* Lower scan limit: 7100 kHz
* Upper scan limit: 7150 kHz
* Scan step: 1 kHz
* Scan step delay: 500 ms

A warning message "uncalibrated" will be displayed until you recalibrate the VFO again.
