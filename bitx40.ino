/**
    Raduino sketch for BITX40 transceiver Pavel's blend (Pavel CO7WT, pavelmc@gmail.com)

    Based on the Raduino_v1.15 for BITX40 - Allard Munters PE1NWL (pe1nwl@gooddx.net)

    This source file is under General Public License version 3.
*/

// EEPROM for settings
#include <EEPROM.h>

/**
    The main chip which generates upto three oscillators of various frequencies in the
    Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet
    from www.silabs.com although, strictly speaking it is not a requirment to understand this code.

    We are using now the simple library built by Pavel, CO7WT, you can find it as always on Github
    https://github.com/pavelmc/si5351mcu/ this lib is simple, small and has all the features we need
    by using it we cut about 25% of the code without losing any feature.

    Tip: by using the Pavel's lib you don't has to include nor initialize the Wire library, it does it for you.
*/

#include <si5351mcu.h> // https://github.com/pavelmc/si5351mcu/
Si5351mcu si5351;

/**
    The Raduino board is the size of a standard 16x2 LCD panel. It has three connectors:

    First, is an 8 pin connector that provides +5v, GND and six analog input pins that can also be
    configured to be used as digital input or output pins. These are referred to as A0,A1,A2,
    A3,A6 and A7 pins. The A4 and A5 pins are missing from this connector as they are used to
    talk to the Si5351 over I2C protocol.

    A0     A1   A2   A3    GND    +5V   A6   A7
    BLACK BROWN RED ORANGE YELLOW GREEN BLUE VIOLET

    Second is a 16 pin LCD connector. This connector is meant specifically for the standard 16x2
    LCD display in 4 bit mode. The 4 bit mode requires 4 data lines and two control lines to work:
    Lines used are : RESET, ENABLE, D4, D5, D6, D7
    We include the library and declare the configuration of the LCD panel too
*/

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

/**
    The Arduino, unlike C/C++ on a regular computer with gigabytes of RAM, has very little memory.
    We have to be very careful with variables that are declared inside the functions as they are
    created in a memory region called the stack. The stack has just a few bytes of space on the Arduino
    if you declare large strings inside functions, they can easily exceed the capacity of the stack
    and mess up your programs.
    We circumvent this by declaring a few global buffers as  kitchen counters where we can
    slice and dice our strings. These strings are mostly used to control the display or handle
    the input and output from the USB port. We must keep a count of the bytes used while reading
    the serial port as we can easily run out of buffer space. This is done in the serial_in_count variable.
*/

char c[17], b[10], printBuff1[17], printBuff2[17];

/**
    We need to carefully pick assignment of pin for various purposes.
    There are two sets of completely programmable pins on the Raduino.
    First, on the top of the board, in line with the LCD connector is an 8-pin connector
    that is largely meant for analog inputs and front-panel control. It has a regulated 5v output,
    ground and six pins. Each of these six pins can be individually programmed
    either as an analog input, a digital input or a digital output.
    The pins are assigned as follows:
          A0,   A1,  A2,  A3,   GND,   +5V,  A6,  A7
       pin 8     7    6    5     4       3    2    1 (connector P1)
        BLACK BROWN RED ORANGE YELLW  GREEN  BLUE VIOLET
        (while holding the board up so that back of the board faces you)

    Though, this can be assigned anyway, for this application of the Arduino, we will make the following
    assignment:

    A0 (digital input) for sensing the PTT. Connect to the output of U3 (LM7805) of the BITX40.
      This way the A0 input will see 0V (LOW) when PTT is not pressed, +5V (HIGH) when PTT is pressed.
    A1 (digital input) is to connect to a straight key. Open (HIGH) during key up, switch to ground (LOW) during key down.
    A2 (digital input) can be used for calibration by grounding this line (not required when you have the Function Button at A3).
    A3 (digital input) is connected to a push button that can momentarily ground this line. This Function Button will be used to switch between different modes, etc.
    A4 (already in use for talking to the SI5351)
    A5 (already in use for talking to the SI5351)
    A6 (analog input) is not currently used
    A7 (analog input) is connected to a center pin of good quality 100K or 10K linear potentiometer with the two other ends connected to ground and +5v lines available on the connector.
    This implements the tuning mechanism.
*/

#define PTT_SENSE (A0)
#define KEY (A1)
#define ANALOG_TUNING (A7)

/*
 * I confess, have killed some of the arduino pins, so I have to re-wire my arduino using
 * some free wires, if you are in my situation just uncomment and adapt the following code
 *
 */

#define KILLED_PINS true

#ifdef KILLED_PINS
    #define CAL_BUTTON (4)
    #define FBUTTON (2)
#else
    #define CAL_BUTTON (A2)
    #define FBUTTON (A3)
#endif


bool PTTsense_installed; //whether or not the PTT sense line is installed (detected automatically during startup)

/**
    The second set of 16 pins on the bottom connector P3 have the three clock outputs and the digital lines to control the rig.
    This assignment is as follows :
      Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16  (connector P3)
         +12V +12V CLK2  GND  GND CLK1  GND  GND  CLK0  GND  D2   D3   D4   D5   D6   D7
    These too are flexible with what you may do with them, for the Raduino, we use them to :

    output D5 - CW_TONE : Side tone output
    output D6 - CW_CARRIER line : turns on the carrier for CW
    output D7 - TX_RX line : Switches between Transmit and Receive in CW mode
*/

#define CW_TONE (5)
#define CW_CARRIER (6)
#define TX_RX (7)

/**
    The raduino has a number of timing parameters, all specified in milliseconds
    CW_TIMEOUT : how many milliseconds between consecutive keyup and keydowns before switching back to receive?
*/

int CW_TIMEOUT; // in milliseconds, this is the parameter that determines how long the tx will hold between cw key downs

/**
    The Raduino supports two VFOs : A and B and receiver incremental tuning (RIT).
    we define a variables to hold the frequency of the two VFOs, RIT, SPLIT
    the rit offset as well as status of the RIT

    To use this facility, wire up push button on A3 line of the control connector (Function Button)
*/

unsigned long vfoA, vfoB; // the frequencies the VFOs
bool ritOn = false; // whether or not the RIT is on
int RIToffset = 0;  // offset (Hz)
int RIT = 0; // actual RIT offset that is applied during RX when RIT is on
int RIT_old;
bool splitOn; // whether or not SPLIT is on
bool vfoActive; // which VFO (false=A or true=B) is active
byte mode_A, mode_B; // the mode of each VFO

bool firstrun = true;

/**
    We need to apply some frequency offset to calibrate the dial frequency. Calibration is done in LSB mode.
*/
int cal; // frequency offset in Hz

/**
    In USB mode we need to apply some additional frequency offset, so that zerobeat in USB is same as in LSB
*/
int USB_OFFSET; // USB offset in Hz

/**
    We can set the VFO drive to a certain level (2,4,6,8 mA)
*/
byte LSBdrive;
byte USBdrive;

// scan parameters

int scan_start_freq; // lower scan limit (kHz)
int scan_stop_freq; // upper scan limit (kHz)
int scan_step_freq; // step size (Hz)
int scan_step_delay; // step delay (ms)


/**
    Raduino has 4 modes of operation:
*/

#define LSB (0)
#define USB (1)
#define CWL (2)
#define CWU (3)

/**
    Raduino needs to keep track of current state of the transceiver. These are a few variables that do it
*/
byte mode = LSB; // mode of the currently active VFO
char *model[] = {" LSB", " USB", " CWL", " CWU"};
bool inTx = false; // whether or not we are in transmit mode
bool keyDown = false; // whether we have a key up or key down
unsigned long TimeOut = 0;

/** Tuning Mechanism of the Raduino
    We use a linear pot that has two ends connected to +5 and the ground. the middle wiper
    is connected to ANALOG_TUNNING pin. Depending upon the position of the wiper, the
    reading can be anywhere from 0 to 1023.
    If we want to use a multi-turn potentiometer with a tuning range of 500 kHz and a step
    size of 50 Hz we need 10,000 steps which is about 10x more than the steps that the ADC
    provides. Arduino's ADC has 10 bits which results in 1024 steps only.
    We can artificially expand the number of steps by a factor 10 by oversampling 100 times.
    As a result we get 10240 steps.
    The tuning control works in steps of 50Hz each for every increment between 10 and 10230.
    Hence the turning the pot fully from one end to the other will cover 50 x 10220 = 511 KHz.
    But if we use the standard 1-turn pot, then a tuning range of 500 kHz would be too much.
    (tuning would become very touchy). In the SETTINGS menu we can limit the tuning range
    depending on the potentiometer used and the band section of interest. Tuning beyond the
    limits is still possible by the 'scan-up' and 'scan-down' mode at the end of the pot.
    At the two ends, that is, the tuning starts stepping up or down in 10 KHz steps.
    To stop the scanning the pot is moved back from the edge.F
*/

int TUNING_RANGE; // tuning range (in kHz) of the tuning pot
unsigned long baseTune = 7100000UL; // frequency (Hz) when tuning pot is at minimum position

#define bfo_freq (11998000UL)

int old_knob = 0;

int CW_OFFSET; // the amount of offset (Hz) during RX, equal to sidetone frequency
int RXshift = 0; // the actual frequency shift that is applied during RX depending on the operation mode


#define LOWEST_FREQ  (6995000L) // absolute minimum frequency (Hz)
#define HIGHEST_FREQ (7500000L) //  absolute maximum frequency (Hz)

unsigned long frequency; // the 'dial' frequency as shown on the display

/**
    The raduino has multiple RUN-modes:
*/
#define RUN_NORMAL (0)  // normal operation
#define RUN_CALIBRATE (1) // calibrate VFO frequency in LSB mode
#define RUN_DRIVELEVEL (2) // set VFO drive level
#define RUN_TUNERANGE (3) // set the range of the tuning pot
#define RUN_CWOFFSET (4) // set the CW offset (=sidetone pitch)
#define RUN_SCAN (5) // frequency scanning mode
#define RUN_SCAN_PARAMS (6) // set scan parameters
#define RUN_MONITOR (7) // frequency scanning mode

byte RUNmode = RUN_NORMAL;

/**
    S-meter/TX-Power defines and vars

    This needs a group of mods described in the files
    * smeter_mod.png
    * tx-meter.png
    * tx_power_mod.md
    * agc_mod.png

    You can find them on this project folder
*/
#define MINTERVAL   10      // msecs, how often we sample the signal
#define SMETER      (A6)    // Analog input
#define AGC         3       // PWM pin for AGC
#define SSAMPLE     3       // how many samples to average
unsigned long stimeout = millis() + MINTERVAL;  // smeter timeout
word smeterpool = 0;    // this will hold the sum of samples
word smeter = 0;        // this will hold the value of the smeter for the bar
byte smcount = 0;       // how many samples lies on smeterpool


/**
    Display Routines
    These two display routines print a line of characters to the upper or lower lines of the 16x2 display
*/

void printLine1(char *c) {
    // only refresh the display when there was a change
    if (strcmp(c, printBuff1)) {
        // start of line 1
        lcd.setCursor(0, 0);
        // write text
        lcd.print(c);
        // update copy
        strcpy(printBuff1, c);
        // fill with spaces
        fillSpaces(c);
    }
}

void printLine2(char *c) {
    // only refresh the display when there was a change
    if (strcmp(c, printBuff2)) {
        // start of line 2
        lcd.setCursor(0, 1);
        // write text
        lcd.print(c);
        // update copy
        strcpy(printBuff2, c);
        // fill with spaces
        fillSpaces(c);
    }
}


// helper function to fill the remaining "space" with spaces...
void fillSpaces(char *c) {
    // complete the lcd line with spaces up to 15
    for (byte i = strlen(c); i < 16; i++) lcd.print(' ');
}

/**
    Building upon the previous  two functions,
    update Display paints the first line as per current state of the radio
*/

void updateDisplay() {
    // tks Jack Purdum W8TEE
    // replaced fsprint commmands by str commands for code size reduction

    memset(c, 0, sizeof(c));
    memset(b, 0, sizeof(b));

    ltoa(frequency, b, DEC);

    if (!vfoActive) // VFO A is active
        strcpy(c, "A ");
    else
        strcpy(c, "B ");

    // put the frecuency in the format "7.110.2"
    c[2] = b[0];
    strcat(c, ".");
    strncat(c, &b[1], 3);
    strcat(c, ".");
    strncat(c, &b[4], 1);

    // simpler & efficient
    strcat(c, model[mode]);

    if (inTx)
        strcat(c, " TX");
    else if (splitOn)
        strcat(c, " SP");

    printLine1(c);
}

// function to generate a bleep sound (FB menu)
void bleep(int pitch, int duration, byte repeat) {
    for (byte i = 0; i < repeat; i++) {
        tone(CW_TONE, pitch);
        delay(duration);
        noTone(CW_TONE);
        delay(duration);
    }
}

bool calbutton = false;

/**
    To use calibration sets the accurate readout of the tuned frequency
    To calibrate, follow these steps:
    1. Tune in a LSB signal that is at a known frequency.
    2. Now, set the display to show the correct frequency,
      the signal will no longer be tuned up properly
    3. Use the "LSB calibrate" option in the "Settings" menu (or Press the CAL_BUTTON line to the ground (pin A2 - red wire))
    4. tune in the signal until it sounds proper.
    5. Press the FButton (or Release CAL_BUTTON)
    In step 4, when we say 'sounds proper' then, for a CW signal/carrier it means zero-beat
    and for LSB it is the most natural sounding setting.

    Calibration is an offset value that is added to the VFO frequency.
    We store it in the EEPROM and read it in setup() when the Radiuno is powered up.

    Then select the "USB calibrate" option in the "Settings" menu and repeat the same steps for USB mode.
*/

int shift, current_setting;
void calibrate() {
    if (RUNmode != RUN_CALIBRATE) {

        if (mode == USB)
            current_setting = USB_OFFSET;
        else
            current_setting = cal;

        shift = current_setting - (analogRead(ANALOG_TUNING) * 10) + 5000;
    }

    // The tuning knob gives readings from 0 to 1000
    // Each step is taken as 10 Hz and the mid setting of the knob is taken as zero

    if (mode == USB) {
        USB_OFFSET = constrain((analogRead(ANALOG_TUNING) * 10) - 5000 + shift, -5000, 5000);

        if (analogRead(ANALOG_TUNING) < 5 && USB_OFFSET > -5000)
            shift = shift - 10;
        else if (analogRead(ANALOG_TUNING) > 1020 && USB_OFFSET < 5000)
                shift = shift + 10;
    } else {
        cal = constrain((analogRead(ANALOG_TUNING) * 10) - 5000 + shift, -5000, 5000);

        if (analogRead(ANALOG_TUNING) < 5 && cal > -5000)
            shift = shift - 10;
        else if (analogRead(ANALOG_TUNING) > 1020 && cal < 5000)
            shift = shift + 10;
    }

    // if Fbutton is pressed again (or when the CAL button is released), we save the setting
    if (!digitalRead(FBUTTON) || (calbutton && digitalRead(CAL_BUTTON))) {
        RUNmode = RUN_NORMAL;
        calbutton = false;

        if (mode == USB)
            printLine2((char *)"USB Calibrated!");
        else
            printLine2((char *)"LSB Calibrated!");

        // update the eeprom
        saveEEPROM();

        delay(700);
        bleep(600, 50, 2);
        printLine2((char *)"--- SETTINGS ---");
        shiftBase(); //align the current knob position with the current frequency
    } else {
        // while offset adjustment is in progress, keep tweaking the
        // frequency as read out by the knob, display the change in the second line
        RUNmode = RUN_CALIBRATE;

        if (mode == USB) {
            si5351.setFreq(2, bfo_freq + frequency + cal / 5 * 19 - USB_OFFSET);
            itoa(USB_OFFSET, b, DEC);
        } else {
            si5351.setFreq(2, bfo_freq - frequency + cal);
            itoa(cal, b, DEC);
        }

        strcpy(c, "offset ");
        strcat(c, b);
        strcat(c, " Hz");
        printLine2(c);
    }
}


/**
    The setFrequency is a little tricky routine, it works differently for USB and LSB
    The BITX BFO is permanently set to lower sideband, (that is, the crystal frequency
    is on the higher side slope of the crystal filter).

    LSB: The VFO frequency is subtracted from the BFO. Suppose the BFO is set to exactly 12 MHz
    and the VFO is at 5 MHz. The output will be at 12.000 - 5.000  = 7.000 MHz
    USB: The BFO is subtracted from the VFO. Makes the LSB signal of the BITX come out as USB!!
    Here is how it will work:
    Consider that you want to transmit on 14.000 MHz and you have the BFO at 12.000 MHz. We set
    the VFO to 26.000 MHz. Hence, 26.000 - 12.000 = 14.000 MHz. Now, consider you are whistling a tone
    of 1 KHz. As the BITX BFO is set to produce LSB, the output from the crystal filter will be 11.999 MHz.
    With the VFO still at 26.000, the 14 Mhz output will now be 26.000 - 11.999 = 14.001, hence, as the
    frequencies of your voice go down at the IF, the RF frequencies will go up!

    Thus, setting the VFO on either side of the BFO will flip between the USB and LSB signals.

    In addition we add some offset to USB mode so that the dial frequency is correct in both LSB and USB mode.
    The amount of offset can be set in the SETTING menu as part of the calibration procedure.

    Furthermore we add/substract the sidetone frequency only when we receive CW, to assure zero beat
    between the transmitting and receiving station (RXshift)
    The desired sidetone frequency can be set in the SETTINGS menu.
*/

void setFrequency(unsigned long f) {
    if (mode & 1) // if we are in UPPER side band mode
        si5351.setFreq(2, bfo_freq + f + cal * 19 / 5 - USB_OFFSET - RXshift - RIT);
    else // if we are in LOWER side band mode
        si5351.setFreq(2, bfo_freq - f + cal - RXshift - RIT);

    updateDisplay();
}

/**
    The checkTX toggles the T/R line. If you would like to make use of RIT, etc,
    you must connect pin A0 (black wire) via a 10K resistor to the output of U3
    This is a voltage regulator LM7805 which goes on during TX. We use the +5V output
    as a PTT sense line (to tell the Raduino that we are in TX).
*/

void checkTX() {
    // We don't check for ptt when transmitting cw
    // as long as the TimeOut is non-zero, we will continue to hold the
    // radio in transmit mode
    if (TimeOut > 0) return;

    if (digitalRead(PTT_SENSE) && !inTx) {
        // go in transmit mode
        inTx = true;
        RXshift = RIT = RIT_old = 0;

        mode = mode & B11111101; // leave CW mode, return to SSB mode

        if (!vfoActive)
            mode_A = mode;      // if VFO A is active
        else
            mode_B = mode;      // if VFO B is active

        // update eeprom data
        saveEEPROM();

        setFrequency(frequency);
        shiftBase();
        updateDisplay();

        // when SPLIT is on, swap the VFOs
        if (splitOn) swapVFOs();
    }

    if (!digitalRead(PTT_SENSE) && inTx) {
        //go in receive mode
        inTx = false;
        updateDisplay();

        // when SPLIT was on, swap the VFOs back to original state
        if (splitOn) swapVFOs();
    }
}

/*  CW is generated by unbalancing the mixer when the key is down.
    During key down, the output CW_CARRIER is HIGH (+5V).
    This output is connected via a 10K resistor to the mixer input. The mixer will
    become unbalanced when CW_CARRIER is HIGH, so a carrier will be transmitted.
    During key up, the output CW_CARRIER is LOW (0V). The mixer will remain balanced
    and the carrrier will be suppressed.

    The radio will go into CW mode automatically as soon as the key goes down, and
    return to normal LSB/USB mode when the key has been up for some time.

    There are three variables that track the CW mode
    inTX    : true when the radio is in transmit mode
    keyDown : true when the CW is keyed down, you maybe in transmit mode (inTX true)
              and yet between dots and dashes and hence keyDown could be true or false
    TimeOut : Figures out how long to wait between dots and dashes before putting
              the radio back in receive mode

    When we transmit CW, we need to apply some offset (800Hz) to the TX frequency, in
    order to keep zero-beat between the transmitting and receiving station. The shift
    depends on whether upper or lower sideband CW is used:
    In CW-U (USB) mode we must shift the TX frequency 800Hz up
    In CW-L (LSB) mode we must shift the TX frequency 800Hz down

    The default offset (CW_OFFSET) is 800Hz, the default timeout (CW_TIMEOUT) is 350ms.
    The user can change these in the SETTINGS menu.
*/

void checkCW() {

    if (!keyDown && !digitalRead(KEY)) {
        keyDown = true;

        //switch to transmit mode if we are not already in it
        if (!inTx) {
            // activate the PTT switch - go in transmit mode
            digitalWrite(TX_RX, 1);

            //give the relays a few ms to settle the T/R relays
            delay(5);
            inTx = true;

            // when SPLIT is on, swap the VFOs first
            if (splitOn) swapVFOs();

            mode = mode | 2; // go into to CW mode

            if (!vfoActive)
                mode_A = mode;  // if VFO A is active
            else
                mode_B = mode;  // if VFO B is active

            // update the eeprom data
            saveEEPROM();

            // update a lot of things
            RXshift = 0;
            setFrequency(frequency);
            shiftBase();
        }
    }

    //keep resetting the timer as long as the key is down
    if (keyDown) TimeOut = millis() + CW_TIMEOUT;

    //if the key goes up again after it's been down
    if (keyDown && digitalRead(KEY)) {
        keyDown = false;
        TimeOut = millis() + CW_TIMEOUT;
    }

    //if we are in cw-mode and have a keyup for a "longish" time (CW_TIMEOUT value in ms)
    if (TimeOut > 0 && inTx && TimeOut < millis()) {

        inTx = false;
        // reset the CW timeout counter
        TimeOut = 0;
        RXshift = CW_OFFSET;
        setFrequency(frequency);
        shiftBase();

        // then swap the VFOs back when SPLIT was on
        if (splitOn) swapVFOs();

        // release the PTT switch - move the radio back to receive
        digitalWrite(TX_RX, 0);
        //give the relays a few ms to settle the T/R relays
        delay(10);
    }


    if (keyDown) {
        digitalWrite(CW_CARRIER, 1); // generate carrier
        tone(CW_TONE, CW_OFFSET); // generate sidetone
    } else {
        digitalWrite(CW_CARRIER, 0); // stop generating the carrier
        noTone(CW_TONE); // stop generating the sidetone
    }
}

byte param;

/**
    The Function Button is used for several functions
    NORMAL menu (normal operation):
    1 short press: swap VFO A/B
    2 short presses: toggle RIT on/off
    3 short presses: toggle SPLIT on/off
    4 short presses: toggle LSB/USB
    5 short presses: start freq scan mode
    5 short presses: start A/B monitor mode
    long press (>1 Sec): VFO A=B
    VERY long press (>3 sec): go to SETTINGS menu

    SETTINGS menu:
    1 short press: LSB calibration
    2 short presses: USB calibration
    3 short presses: Set VFO drive level in LSB mode
    4 short presses: Set VFO drive level in USB mode
    5 short presses: Set tuning range
    6 short presses: Set the 2 CW parameters (sidetone pitch, CW timeout)
    7 short presses: Set the 4 scan parameters (lower limit, upper limit, step size, step delay)
    long press: exit SETTINGS menu - go back to NORMAL menu
*/
char clicks;
char *clickLabels[] = {"Swap VFOs", "RIT ON", "SPLIT ON/OFF", "Switch mode", "Start freq scan", "Monitor VFO A/B", "LSB calibration", "USB calibration", "VFO drive - LSB", "VFO drive - USB", "Set tuning range", "Set CW params", "Set scan params"};

void checkButton() {
    static byte action;
    static long t1, t2;
    static bool pressed = false;

    if (digitalRead(FBUTTON)) {
        t2 = millis() - t1; //time elapsed since last button press
        if (pressed)
            if (clicks < 10 && t2 > 600 && t2 < 3000) { //detect long press to reset the VFO's
            bleep(600, 50, 1);
            resetVFOs();
            delay(700);
            clicks = 0;
            }

        if (t2 > 500) { // max time between button clicks (ms)
            action = clicks;
            if (clicks >= 10)
                clicks = 10;
            else
                clicks = 0;
        }
        pressed = false;

    } else {
        delay(10);
            if (!digitalRead(FBUTTON)) {
            // button was really pressed, not just some noise
            if (ritOn) {
                toggleRIT();
                bleep(600, 50, 1);
                delay(700);
                return;
            }

            if (!pressed) {
                pressed = true;
                t1 = millis();
                bleep(1200, 50, 1);
                action = 0;
                clicks++;

                if (clicks > 17) clicks = 11;

                if (clicks > 6 && clicks < 10) clicks = 1;

                // simple an elegant way of print the cecons line labels
                printLine2(clickLabels[1 + clicks]);

            }
            // long press: reset the VFOs
            else if ((millis() - t1) > 600 && (millis() - t1) < 800 && clicks < 10)
                printLine2((char *)"Reset VFOs");

            // VERY long press: go to the SETTINGS menu
            if ((millis() - t1) > 3000 && clicks < 10) {
                bleep(1200, 150, 3);
                printLine2((char *)"--- SETTINGS ---");
                clicks = 10;
                //disable RIT if is was on
                if (ritOn) toggleRIT();
            }
            // long press: return to the NORMAL menu
            else if ((millis() - t1) > 1500 && clicks > 10) {
                bleep(1200, 150, 3);
                clicks = -1;
                pressed = false;
                printLine2((char *)" --- NORMAL ---");
                delay(700);
            }
        }
    }

    if (action != 0 && action != 10) bleep(600, 50, 1);

    switch (action) {
        // NORMAL menu

        case 1: // swap the VFOs
            swapVFOs();
            saveEEPROM();
            delay(700);
            break;

        case 2: // toggle the RIT on/off
            toggleRIT();
            delay(700);
            break;

        case 3: // toggle SPLIT on/off
            toggleSPLIT();
            delay(700);
            break;

        case 4: // toggle the mode LSB/USB
            toggleMode();
            delay(700);
            break;

        case 5: // start scan mode
            RUNmode = RUN_SCAN;
            TimeOut = millis() + scan_step_delay;
            frequency = scan_start_freq * 1000L;
            printLine2((char *)"freq scanning");
            break;

        case 6: // Monitor mode
            RUNmode = RUN_MONITOR;
            TimeOut = millis() + scan_step_delay;
            printLine2((char *)"A/B monitoring");
            break;

        // SETTINGS MENU

        case 11: // calibrate the dial frequency in LSB
            RXshift = 0;
            mode = LSB;
            setFrequency(frequency);
            SetSideBand(LSBdrive);
            calibrate();
            break;

        case 12: // calibrate the dial frequency in USB
            RXshift = 0;
            mode = USB;
            setFrequency(frequency);
            SetSideBand(USBdrive);
            calibrate();
            break;

        case 13: // set the VFO drive level in LSB
            mode = LSB;
            SetSideBand(LSBdrive);
            VFOdrive();
            break;

        case 14: // set the VFO drive level in USB
            mode = USB;
            SetSideBand(USBdrive);
            VFOdrive();
            break;

        case 15: // set the tuning pot range
            set_tune_range();
            break;

        case 16: // set CW parameters (sidetone pitch, CW timeout)
            param = 1;
            set_CWparams();
            break;

        case 17: // set the 4 scan parameters
            param = 1;
            scan_params();
            break;
    }
}

void swapVFOs() {
  if (vfoActive) { // if VFO B is active
    vfoActive = false; // switch to VFO A
    vfoB = frequency;
    frequency = vfoA;
    mode = mode_A;
  }

  else { //if VFO A is active
    vfoActive = true; // switch to VFO B
    vfoA = frequency;
    frequency = vfoB;
    mode = mode_B;
  }

  if (mode & 1) // if we are in UPPER side band mode
    SetSideBand(USBdrive);
  else // if we are in LOWER side band mode
    SetSideBand(LSBdrive);

  if (!inTx && mode > 1)
    RXshift = CW_OFFSET;
  else
    RXshift = 0;


  shiftBase(); //align the current knob position with the current frequency
}

void toggleRIT() {
  if (!PTTsense_installed) {
    printLine2((char *)"Not available!");
    return;
  }
  ritOn = !ritOn; // toggle RIT
  if (!ritOn)
    RIT = RIT_old = 0;
  shiftBase(); //align the current knob position with the current frequency
  firstrun = true;
  if (splitOn) {
    splitOn = false;
    saveEEPROM();
  }
  updateDisplay();
}


void toggleSPLIT() {
  if (!PTTsense_installed) {
    printLine2((char *)"Not available!");
    return;
  }
  splitOn = !splitOn; // toggle SPLIT
  saveEEPROM();
  if (ritOn) {
    ritOn = false;
    RIT = RIT_old = 0;
    shiftBase();
  }
  updateDisplay();
}

void toggleMode() {
  if (PTTsense_installed)
    mode = (mode + 1) & 3; // rotate through LSB-USB-CWL-CWU
  else
    mode = (mode + 1) & 1; // switch between LSB and USB only (no CW)

  if (mode & 2) // if we are in CW mode
    RXshift = CW_OFFSET;
  else // if we are in SSB mode
    RXshift = 0;

  if (mode & 1) // if we are in UPPER side band mode
    SetSideBand(USBdrive);
  else // if we are in LOWER side band mode
    SetSideBand(LSBdrive);
}

void SetSideBand(byte drivelevel) {

  set_drive_level(drivelevel);
  setFrequency(frequency);
  if (!vfoActive)
    mode_A = mode;  // if VFO A is active
  else
    mode_B = mode;  // if VFO B is active

  // update the epprom
  saveEEPROM();
}


// resetting the VFO's will set both VFO's to the current frequency and mode
void resetVFOs() {
  printLine2((char *)"VFO A=B !");
  vfoA = vfoB = frequency;
  mode_A = mode_B = mode;
  updateDisplay();
  bleep(600, 50, 1);
  saveEEPROM();
}

void VFOdrive() {
  static byte drive;

  if (RUNmode != RUN_DRIVELEVEL) {

    if (mode & 1) // if UPPER side band mode
      current_setting = USBdrive / 2 - 1;
    else // if LOWER side band mode
      current_setting = LSBdrive / 2 - 1;

    shift = analogRead(ANALOG_TUNING);
  }

  //generate drive level values 2,4,6,8 from tuning pot
  drive = 2 * ((((analogRead(ANALOG_TUNING) - shift) / 50 + current_setting) & 3) + 1);

  // if Fbutton is pressed again, we save the setting

  if (!digitalRead(FBUTTON)) {
    RUNmode = RUN_NORMAL;
    printLine2((char *)"Drive level set!");

    if (mode & 1)
      USBdrive = drive;     // if UPPER side band mode
    else
      LSBdrive = drive;     // if LOWER side band mode

    // update the eeprom
    saveEEPROM();

    delay(700);
    bleep(600, 50, 2);
    printLine2((char *)"--- SETTINGS ---");
    shiftBase(); //align the current knob position with the current frequency
  }
  else {
    // while the drive level adjustment is in progress, keep tweaking the
    // drive level as read out by the knob and display it in the second line
    RUNmode = RUN_DRIVELEVEL;
    set_drive_level(drive);

    itoa(drive, b, DEC);
    strcpy(c, "drive level ");
    strcat(c, b);
    strcat(c, "mA");
    printLine2(c);
  }
}

/* this function allows the user to set the tuning range depending on the type of potentiometer
  for a standard 1-turn pot, a range of 50 KHz is recommended
  for a 10-turn pot, a tuning range of 200 kHz is recommended
*/
void set_tune_range() {

  if (RUNmode != RUN_TUNERANGE) {
    current_setting = TUNING_RANGE;
    shift = current_setting - 10 * analogRead(ANALOG_TUNING) / 20;
  }

  //generate values 10-500 from the tuning pot
  TUNING_RANGE = constrain(10 * analogRead(ANALOG_TUNING) / 20 + shift, 10, 500);
  if (analogRead(ANALOG_TUNING) < 5 && TUNING_RANGE > 10)
    shift = shift - 10;
  else if (analogRead(ANALOG_TUNING) > 1020 && TUNING_RANGE < 500)
    shift = shift + 10;

  // if Fbutton is pressed again, we save the setting
  if (!digitalRead(FBUTTON)) {
    //Write the 2 bytes of the tuning range into the eeprom memory.
    saveEEPROM();
    printLine2((char *)"Tune range set!");
    RUNmode = RUN_NORMAL;
    delay(700);
    bleep(600, 50, 2);
    printLine2((char *)"--- SETTINGS ---");
    shiftBase(); //align the current knob position with the current frequency
  }

  else {
    RUNmode = RUN_TUNERANGE;
    itoa(TUNING_RANGE, b, DEC);
    strcpy(c, "range ");
    strcat(c, b);
    strcat(c, " kHz");
    printLine2(c);
  }
}

/* this function allows the user to set the two CW parameters: CW-OFFSET (sidetone pitch) and CW-TIMEOUT.
*/

void set_CWparams() {

  if (firstrun) {
    if (param == 1) {
      CW_TIMEOUT = 10; // during CW offset adjustment, temporarily set CW_TIMEOUT to minimum
      current_setting = CW_OFFSET;
      shift = current_setting - analogRead(ANALOG_TUNING) - 200;
    }
    else {
      current_setting = CW_TIMEOUT;
      shift = current_setting - 10 * (analogRead(ANALOG_TUNING) / 10);
    }
  }

  if (param == 1) {
    //generate values 500-1000 from the tuning pot
    CW_OFFSET = constrain(analogRead(ANALOG_TUNING) + 200 + shift, 200, 1200);
    if (analogRead(ANALOG_TUNING) < 5 && CW_OFFSET > 200)
      shift = shift - 10;
    else if (analogRead(ANALOG_TUNING) > 1020 && CW_OFFSET < 1200)
      shift = shift + 10;
  }
  else {
    //generate values 0-1000 from the tuning pot
    CW_TIMEOUT = constrain(10 * ( analogRead(ANALOG_TUNING) / 10) + shift, 0, 1000);
    if (analogRead(ANALOG_TUNING) < 5 && CW_TIMEOUT > 10)
      shift = shift - 10;
    else if (analogRead(ANALOG_TUNING) > 1020 && CW_TIMEOUT < 1000)
      shift = shift + 10;
  }

  // if Fbutton is pressed again, we save the setting
  if (!digitalRead(FBUTTON)) {
    if (param == 1) {
      //Write the 2 bytes of the CW offset into the eeprom memory.
      saveEEPROM();
      bleep(600, 50, 1);
      delay(200);
    }
    else {
      //Write the 2 bytes of the CW Timout into the eeprom memory.
      saveEEPROM();
      printLine2((char *)"CW params set!");
      RUNmode = RUN_NORMAL;
      delay(700);
      bleep(600, 50, 2);
      printLine2((char *)"--- SETTINGS ---");
      shiftBase(); //align the current knob position with the current frequency
    }
    param ++;
    firstrun = true;
  }

  else {
    RUNmode = RUN_CWOFFSET;
    firstrun = false;
    if (param == 1) {
      itoa(CW_OFFSET, b, DEC);
      strcpy(c, "sidetone ");
      strcat(c, b);
      strcat(c, " Hz");
    }
    else {
      itoa(CW_TIMEOUT, b, DEC);
      strcpy(c, "timeout ");
      strcat(c, b);
      strcat(c, " ms");
    }
    printLine2(c);
  }
}

/* this function allows the user to set the 4 scan parameters: lower limit, upper limit, step size and step delay
*/

void scan_params() {

  if (firstrun) {
    switch (param) {

      case 1: // set the lower scan limit

        current_setting = scan_start_freq;
        shift = current_setting - analogRead(ANALOG_TUNING) / 2 - 7000;
        break;

      case 2: // set the upper scan limit

        current_setting = scan_stop_freq;
        shift = current_setting - map(analogRead(ANALOG_TUNING), 0, 1024, scan_start_freq, 7500);
        break;

      case 3: // set the scan step size

        current_setting = scan_step_freq;
        shift = current_setting - 50 * (analogRead(ANALOG_TUNING) / 5);
        break;

      case 4: // set the scan step delay

        current_setting = scan_step_delay;
        shift = current_setting - 50 * (analogRead(ANALOG_TUNING) / 25);
        break;
    }
  }

  switch (param) {

    case 1: // set the lower scan limit

      //generate values 7000-7500 from the tuning pot
      scan_start_freq = constrain(analogRead(ANALOG_TUNING) / 2 + 7000 + shift, 7000, 7500);
      if (analogRead(ANALOG_TUNING) < 5 && scan_start_freq > 7000)
        shift = shift - 1;
      else if (analogRead(ANALOG_TUNING) > 1020 && scan_start_freq < 7500)
        shift = shift + 1;
      break;

    case 2: // set the upper scan limit

      //generate values 7000-7500 from the tuning pot
      scan_stop_freq = constrain(map(analogRead(ANALOG_TUNING), 0, 1024, scan_start_freq, 7500) + shift, scan_start_freq, 7500);
      if (analogRead(ANALOG_TUNING) < 5 && scan_stop_freq > scan_start_freq)
        shift = shift - 1;
      else if (analogRead(ANALOG_TUNING) > 1020 && scan_stop_freq < 7500)
        shift = shift + 1;
      break;

    case 3: // set the scan step size

      //generate values 50-10000 from the tuning pot
      scan_step_freq = constrain(50 * (analogRead(ANALOG_TUNING) / 5) + shift, 50, 10000);
      if (analogRead(ANALOG_TUNING) < 5 && scan_step_freq > 50)
        shift = shift - 50;
      else if (analogRead(ANALOG_TUNING) > 1020 && scan_step_freq < 10000)
        shift = shift + 50;
      break;

    case 4: // set the scan step delay

      //generate values 0-2500 from the tuning pot
      scan_step_delay = constrain(50 * (analogRead(ANALOG_TUNING) / 25) + shift, 0, 2000);
      if (analogRead(ANALOG_TUNING) < 5 && scan_step_delay > 0)
        shift = shift - 50;
      else if (analogRead(ANALOG_TUNING) > 1020 && scan_step_delay < 2000)
        shift = shift + 50;
      break;
  }

  // if Fbutton is pressed, we save the setting

  if (!digitalRead(FBUTTON)) {
    switch (param) {

      case 1: // save the lower scan limit

        //Write the 2 bytes of the start freq into the eeprom memory.
        saveEEPROM();
        bleep(600, 50, 1);
        break;

      case 2: // save the upper scan limit

        //Write the 2 bytes of the stop freq into the eeprom memory.
        saveEEPROM();
        bleep(600, 50, 1);
        break;

      case 3: // save the scan step size

        //Write the 2 bytes of the step size into the eeprom memory.
        saveEEPROM();
        bleep(600, 50, 1);
        break;

      case 4: // save the scan step delay

        //Write the 2 bytes of the step delay into the eeprom memory.
        saveEEPROM();
        printLine2((char *)"Scan params set!");
        RUNmode = RUN_NORMAL;
        delay(700);
        bleep(600, 50, 2);
        printLine2((char *)"--- SETTINGS ---");
        shiftBase(); //align the current knob position with the current frequency
        break;
    }
    param ++;
    firstrun = true;
  }

  else {
    RUNmode = RUN_SCAN_PARAMS;
    firstrun = false;
    switch (param) {

      case 1: // display the lower scan limit

        itoa(scan_start_freq, b, DEC);
        strcpy(c, "lower ");
        strcat(c, b);
        strcat(c, " kHz");
        break;

      case 2: // display the upper scan limit

        itoa(scan_stop_freq, b, DEC);
        strcpy(c, "upper ");
        strcat(c, b);
        strcat(c, " kHz");
        break;

      case 3: // display the scan step size

        itoa(scan_step_freq, b, DEC);
        strcpy(c, "step ");
        strcat(c, b);
        strcat(c, " Hz");
        break;

      case 4: // display the scan step delay

        itoa(scan_step_delay, b, DEC);
        strcpy(c, "delay ");
        strcat(c, b);
        strcat(c, " ms");
        break;
    }
    printLine2(c);
  }
}


// function to read the position of the tuning knob at high precision (Allard, PE1NWL)
int knob_position() {
  long knob = 0;
  // the knob value normally ranges from 0 through 1023 (10 bit ADC)
  // in order to increase the precision by a factor 10, we need 10^2 = 100x oversampling

  for (byte i = 0; i < 100; i++) {
    knob = knob + analogRead(ANALOG_TUNING) - 10; // take 100 readings from the ADC
  }
  knob = (knob + 5L) / 10L; // take the average of the 100 readings and multiply the result by 10
  //now the knob value ranges from -100 through 10130 (10x more precision)
  return knob;
}

/* Many BITX40's suffer from a strong birdie at 7199 kHz (LSB).
  This birdie may be eliminated by using a different VFO drive level in LSB mode.
  In USB mode, a high drive level may be needed to compensate for the attenuation of
  higher VFO frequencies.
  The drive level for each mode can be set in the SETTINGS menu
*/

void set_drive_level(byte level) {
  //~ switch (level) {
    //~ case 2:
      //~ si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_2MA);
      //~ break;
    //~ case 4:
      //~ si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_4MA);
      //~ break;
    //~ case 6:
      //~ si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_6MA);
      //~ break;
    //~ case 8:
      //~ si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
      //~ break;
  //~ }

    // dynamically calculate the power needed
    // 0 ~ 2mA, 1 ~ 4mA... 3 ~ 8mA
    si5351.setPower(2, level/2 - 1);
}


void doRIT() {

  int knob = knob_position(); // get the current tuning knob position

  if (firstrun) {
    current_setting = RIToffset;
    shift = current_setting - ((knob - 5000) / 10 * 5);
    firstrun = false;
  }

  //generate values -5000 ~ +5000 from the tuning pot
  RIToffset = (knob - 5000) / 10 * 5 + shift;
  if (knob < 0 && RIToffset > -2500)
    shift = shift - 50;
  else if (knob > 10000 && RIToffset < 2500)
    shift = shift + 50;

  RIT = RIToffset;

  if (RIT != RIT_old)
    setFrequency(frequency);

  itoa(RIToffset, b, DEC);
  strcpy(c, "RIT ");
  strcat(c, b);
  strcat(c, " Hz");
  printLine2(c);
  delay(100);
  RIT_old = RIT;
  old_knob = knob;
}


/**
   Function to align the current knob position with the current frequency
   If we switch between VFO's A and B, the frequency will change but the tuning knob
   is still in the same position. We need to apply some offset so that the new frequency
   corresponds with the current knob position.
   This function reads the current knob position, then it shifts the baseTune value up or down
   so that the new frequency matches again with the current knob position.
*/

void shiftBase() {
  setFrequency(frequency);
  long knob = knob_position(); // get the current tuning knob position
  baseTune = frequency - (knob * TUNING_RANGE / 10L);
}

/**
   The Tuning mechansim of the Raduino works in a very innovative way. It uses a tuning potentiometer.
   The tuning potentiometer that a voltage between 0 and 5 volts at ANALOG_TUNING pin of the control connector.
   This is read as a value between 0 and 1000. By 100x oversampling ths range is expanded by a factor 10.
   Hence, the tuning pot gives you 10,000 steps from one end to the other end of its rotation. Each step is 50 Hz,
   thus giving maximum 500 Khz of tuning range. The tuning range is scaled down depending on the TUNING_RANGE value.
   The standard tuning range (for the standard 1-turn pot) is 50 Khz. But it is also possible to use a 10-turn pot
   to tune accross the entire 40m band. The desired TUNING_RANGE can be set via the Function Button in the SETTINGS menu.
   When the potentiometer is moved to either end of the range, the frequency starts automatically moving
   up or down in 10 Khz increments, so it is still possible to tune beyond the range set by TUNING_RANGE.
*/

void doTuning() {

  int knob = knob_position(); // get the current tuning knob position

  // tuning is disabled during TX (only when PTT sense line is installed)
  if (inTx && (abs(knob - old_knob) > 6)) {
    printLine2((char *)"dial is locked");
    shiftBase();
    firstrun = true;
    return;
  }
  else if (inTx)
    return;

  // the knob is fully on the low end, move down by 10 Khz and wait for 300 msec
  if (knob < -80 && frequency > LOWEST_FREQ) {
    baseTune = baseTune - 10000L;
    frequency = baseTune + (long(knob) * TUNING_RANGE / 10L);
    setFrequency(frequency);
    if (clicks < 10) {
      printLine2((char *)"<<<<<<<"); // tks Paul KC8WBK
    }
    delay(300);
  }

  // the knob is full on the high end, move up by 10 Khz and wait for 300 msec
  else if (knob > 10120L && frequency < HIGHEST_FREQ) {
    baseTune = baseTune + 10000L;
    frequency = baseTune + (long(knob) * TUNING_RANGE / 10L);
    setFrequency(frequency);
    if (clicks < 10) {
      printLine2((char *)"         >>>>>>>"); // tks Paul KC8WBK
    }
    delay(300);
  }

  // the tuning knob is at neither extremities, tune the signals as usual ("flutter fix" tks Jerry KE7ER)
  else {
    if (knob != old_knob) {
      static byte dir_knob;
      if ( (knob > old_knob) && ((dir_knob == 1) || ((knob - old_knob) > 5)) ||
           (knob < old_knob) && ((dir_knob == 0) || ((old_knob - knob) > 5)) ) {
        if (knob > old_knob) {
          dir_knob = 1;
          frequency = baseTune + (long(knob + 5) * TUNING_RANGE / 10L);
        }
        else {
          dir_knob = 0;
          frequency = baseTune + (long(knob) * TUNING_RANGE / 10L);
        }
        old_knob = knob;
        setFrequency(frequency);
      }
    }
  }

  if (!vfoActive) // VFO A is active
    vfoA = frequency;
  else
    vfoB = frequency;

  delay(50);
}

byte raduino_version; //version identifier
byte firmware_version; // version from the firmware

void factory_settings() {
    printLine1((char *)"loading standard");
    printLine2((char *)"settings...");

    // set all parameters to defaults and then force the update i the eeprom
    firmware_version = raduino_version;    //version identifier
    cal = 0;              //cal offset value (0 Hz)
    USB_OFFSET = 1500;    //USB offset (1500 Hz)
    LSBdrive = 4;         //VFO drive level in LSB/CWL mode (4 mA)
    USBdrive = 8;         //VFO drive level in USB/CWU mode (8 mA)
    TUNING_RANGE = 50;    //tuning range (50 kHz)
    CW_OFFSET = 800;      //CW offset / sidetone pitch (800 Hz)
    vfoA = 7125000UL;     // VFO A frequency (7125 kHz)
    vfoB = 7125000UL;     // VFO B frequency (7125 kHz)
    mode_A = 0;           // mode VFO A (LSB)
    mode_B = 0;           // mode VFO B (LSB)
    vfoActive = false;    // vfoActive (VFO A)
    splitOn = false;      // SPLIT off
    scan_start_freq = 7110;   // scan_start_freq (7100 kHz)
    scan_stop_freq = 7150;    // scan_stop_freq (7150 kHz)
    scan_step_freq = 1000;    // scan_step_freq (1000 Hz)
    scan_step_delay = 500;    // scan_step_delay (500 ms)
    CW_TIMEOUT = 350;         // CW timout (350 ms)

    // reset eeprom data
    saveEEPROM();
    delay(1000);
}

// save the VFO frequencies when they haven't changed more than 500Hz in the past 30 seconds
// (EEPROM.put writes the data only if it is different from the previous content of the eeprom location)

void save_frequency() {
  static long t3;
  static unsigned long old_vfoA, old_vfoB;
  if ((abs(vfoA - old_vfoA) < 500UL) && (abs(vfoB - old_vfoB) < 500UL)) {
    if (millis() - t3 > 30000) {
      saveEEPROM();
      t3 = millis();
    }
  }
  else
    t3 = millis();
  if (abs(vfoA - old_vfoA) > 500UL)
    old_vfoA = vfoA;
  if (abs(vfoB - old_vfoB) > 500UL)
    old_vfoB = vfoB;
}

void scan() {

  int knob = knob_position();

  if (abs(knob - old_knob) > 8 || (digitalRead(PTT_SENSE) && PTTsense_installed) || !digitalRead(FBUTTON) || !digitalRead(KEY)) {
    //stop scanning
    TimeOut = 0; // reset the timeout counter
    RUNmode = RUN_NORMAL;
    shiftBase();
    delay(400);
  }

  else if (TimeOut < millis()) {
    if (RUNmode == RUN_SCAN) {
      frequency = frequency + scan_step_freq; // change frequency
      // test for upper limit of scan
      if (frequency > scan_stop_freq * 1000L)
        frequency = scan_start_freq * 1000L;
      setFrequency(frequency);
    }
    else // monitor mode
      swapVFOs();

    TimeOut = millis() + scan_step_delay;
  }
}

/**
     S-meter/TX-meter functions
*/
void smeter_check() {
    // just every MINTERVAL
    if (stimeout < millis()) {
        // now, measure and count
        smeterpool += analogRead(SMETER);
        smcount += 1;

        // check if we need to update the lcd
        if (smcount == SSAMPLE) {
            // yes, we have to

            // add the last reading to the pool to get persistence and decay
            smeter = (smeterpool + smeter) / (SSAMPLE + 1);

            // AGC
            analogWrite(AGC, smeter / 4);   // scaled down to 255 from 1023

            // cleaning the buffer
            memset(c, 0, sizeof(c));

            // print to buffer
            byte t = byte(smeter / 68);     // scale down from 0-1023 to 0-15
            while (t) {
                strcat(c, "#");
                t--;
            }

            // ping buffer in the second line
            printLine2(c);

            // reset counters for the next cycle
            smcount = 0;
            smeterpool = 0;
        }

        // prepare for the next cycle
        stimeout = millis() + MINTERVAL;
    }
}

/**
    EEPROM related procedures, to simplify the EEPROM management

 **/

// structured data: Main Configuration Parameters
struct mConf {
    byte version;
    int cal;
    int usboffset;
    byte ldrive;
    byte udrive;
    int trange;
    int cwoffset;
    unsigned long vfoa;
    unsigned long vfob;
    byte modea;
    byte modeb;
    bool vfoactive;
    bool split;
    int scstartf;
    int scstopf;
    int scstepf;
    int scstepd;
    int cwtimeout;
};

// declaring the main configuration variable for mem storage
struct mConf conf;

// initialize the EEPROM mem, also used to store the values in the setup mode
// this procedure has a protection for the EEPROM life using update semantics
// it actually only write a cell if it has changed
void saveEEPROM() {
    // get the parameters from the environment
    conf.version    = raduino_version;
    conf.cal        = cal;
    conf.usboffset  = USB_OFFSET;
    conf.ldrive     = LSBdrive;
    conf.udrive     = USBdrive;
    conf.trange     = TUNING_RANGE;
    conf.cwoffset   = CW_OFFSET;
    conf.vfoa       = vfoA;
    conf.vfob       = vfoB;
    conf.modea      = mode_A;
    conf.modeb      = mode_B;
    conf.vfoactive  = vfoActive;
    conf.split      = splitOn;
    conf.scstartf   = scan_start_freq;
    conf.scstopf    = scan_stop_freq;
    conf.scstepf    = scan_step_freq;
    conf.scstepd    = scan_step_delay;
    conf.cwtimeout  = CW_TIMEOUT;

    // write it
    EEPROM.put(0, conf);
}


// load the eprom contents
void loadEEPROMConfig() {
    // read it
    EEPROM.get(0, conf);

    // put the parameters into the environment
    firmware_version    = conf.version;
    cal                 = conf.cal;
    LSBdrive            = conf.ldrive;
    USBdrive            = conf.udrive;
    TUNING_RANGE        = conf.trange;
    CW_OFFSET           = conf.cwoffset;
    vfoA                = conf.vfoa;
    vfoB                = conf.vfob;
    mode_A              = conf.modea;
    mode_B              = conf.modeb;
    vfoActive           = conf.vfoactive;
    splitOn             = conf.split;
    scan_start_freq     = conf.scstartf;
    scan_stop_freq      = conf.scstopf;
    scan_step_freq      = conf.scstepf;
    scan_step_delay     = conf.scstepd;
    CW_TIMEOUT          = conf.cwtimeout;
}


/**
   setup is called on boot up
   It setups up the modes for various pins as inputs or outputs
   initiliaizes the Si5351 and sets various variables to initial state

   Just in case the LCD display doesn't work well, the debug log is dumped on the serial monitor
   Choose Serial Monitor from Arduino IDE's Tools menu to see the Serial.print messages
*/
void setup() {
  raduino_version = 16;
  strcpy (c, "Raduino v1.17");

  // pin OUT for AGC
  pinMode(AGC, OUTPUT);

  lcd.begin(16, 2);
  printBuff1[0] = 0;
  printBuff2[0] = 0;

  // Start serial and initialize the Si5351
  Serial.begin(9600);
  analogReference(DEFAULT);
  //Serial.println("*Raduino booting up");

  //configure the morse key input to use the internal pull-up
  pinMode(KEY, INPUT_PULLUP);
  //configure the function button to use the internal pull-up
  pinMode(FBUTTON, INPUT_PULLUP);
  //configure the PTT SENSE to use the internal pull-up
  pinMode(PTT_SENSE, INPUT_PULLUP);
  //configure the CAL button to use the internal pull-up
  pinMode(CAL_BUTTON, INPUT_PULLUP);

  pinMode(TX_RX, OUTPUT);
  digitalWrite(TX_RX, 0);
  pinMode(CW_CARRIER, OUTPUT);
  digitalWrite(CW_CARRIER, 0);
  pinMode(CW_TONE, OUTPUT);
  digitalWrite(CW_TONE, 0);

  // when Fbutton or CALbutton is pressed during power up,
  // or after a version update,
  // then all settings will be restored to the standard "factory" values
  loadEEPROMConfig();
  if (!digitalRead(CAL_BUTTON) || !digitalRead(FBUTTON) || (firmware_version != raduino_version)) {
    factory_settings();
  }

  // check if PTT sense line is installed
  if (!digitalRead(PTT_SENSE))
    PTTsense_installed = true; //yes it's installed
  else
    PTTsense_installed = false; //no it's not installed

  printLine1(c);
  delay(1000);

  //display warning message when calibration data was erased
  if ((cal == 0) && (USB_OFFSET == 1500))
    printLine2((char *)"uncalibrated!");

  //initialize the SI5351
  // lib default is 27.000 Mhz, we use 25.000  Mhz
  // remember that this library start with all outputs off.
  si5351.init(25000000);

  // set & apply my calculated correction factor
  si5351.correction(cal);

  // pre-load some sweet spot freqs
  si5351.setFreq(2, 5000000);

  // reset the PLLs and enable the desired output
  si5351.reset();
  si5351.enable(2);

  if (!vfoActive) { // VFO A is active
    frequency = vfoA;
    mode = mode_A;
  }
  else {
    frequency = vfoB;
    mode = mode_B;
  }

  if (mode & 1) // if UPPER side band
    SetSideBand(USBdrive);
  else // if LOWER side band
    SetSideBand(LSBdrive);

  if (mode > 1) // if in CW mode
    RXshift = CW_OFFSET;

  shiftBase(); //align the current knob position with the current frequency

  //If no FButton is installed, and you still want to use custom tuning range settings,
  //uncomment the following line and adapt the value as desired:

  //TUNING_RANGE = 50;    // tuning range (in kHz) of the tuning pot

  //recommended tuning range for a 1-turn pot: 50kHz, for a 10-turn pot: 200kHz

  bleep(CW_OFFSET, 60, 3);
  bleep(CW_OFFSET, 180, 1);
}

void loop() {
  switch (RUNmode) {
    case 0: // for backward compatibility: execute calibration when CAL button is pressed
      if (!digitalRead(CAL_BUTTON)) {
        RUNmode = RUN_CALIBRATE;
        calbutton = true;
        factory_settings();
        printLine1((char *)"Calibrating: Set");
        printLine2((char *)"to zerobeat");
        delay(2000);
      }
      else {
        // smeter in rx mode
        if (clicks == 0 && !ritOn && !inTx) smeter_check();

        // power meter in tx mode
        if (inTx) smeter_check();

        if (PTTsense_installed) {
          checkCW();
          checkTX();
        }
        save_frequency();
        checkButton();
        if (ritOn && !inTx)
          doRIT();
        else
          doTuning();

      }
      return;
    case 1: //calibration
      calibrate();
      break;
    case 2: //set VFO drive level
      VFOdrive();
      break;
    case 3: // set tuning range
      set_tune_range();
      break;
    case 4: // set CW parameters
      set_CWparams();
      checkCW();
      break;
    case 5: // scan mode
      scan();
      break;
    case 6: // set scan paramaters
      scan_params();
      break;
    case 7: // A/B monitor mode
      scan();
      break;
  }
  //delay(100);
}
