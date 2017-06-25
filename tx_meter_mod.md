# TX-meter mod explanation #

The TX-meter mod is just a simple current probe on the TX line, the problem is that you need to adjust the toroid sampling to get the correct levels for the Arduino.

I used a ferrite-black toroid borrowed from a broken electronic light bulb, I don't know what AL or u it is, but that doesn't matter so much as log as you have a voltmeter, let's see how:

* Built the circuit described on tx_meter_mod.png, but don't connect the output wire to the circuit, instead connect it to a volt meter.
* Get into transmission mode and speak loudly or make a long hhhheeeeellllllooooooo! and check what it the highest voltage registered.
* If you voltage is in the range of 4 to 5 volts you are set, just connect it to the point marked TX on the s-meter mod and you are ready to rock.
* If your voltage is to high replace the resistor in parallel with the winding on the toroid for one with a lower value and repeat the loud speaking and voltage check, you will see how the voltage is going down; repeat until your voltage is between 4 and 5 volts.
* If your voltage is to low replace the resistor in parallel with the winding on the toroid for one with a higher value and repeat the loud speaking and voltage check, you will see how the voltage is going up; repeat until your voltage is between 4 and 5 volts.
* In the step above if you don't get enough voltage even without the resistor you need to change the toroid or add more turns to it.

## R in parallel? ##

I used a R of 270 ohms in parallel with this toroid, but experimenting I can live also with 3 turns and 47 ohms in parallel also, so you need to adjust for your particular toroid windings and resistor.

## Note ##

After measuring my toroid, I get this data: (for reference)

* Outer diameter: 10 mm
* Inner diameter: 5.5 mm
* Core tick (width): 4.75 mm
* Turns: 13
* Inductance: 264.3 uH (L-C-CE meter Kit)
* Calculated AL: 1563.91 nH/N^2 (Mini Ring Core Calculator)
* Calculated ui: 2753.6 (Mini Ring Core Calculator)
