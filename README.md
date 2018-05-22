# Air conditioner controller

Controller for an air conditioner unit

manufacturer: ACOND
models: AS-12UR4SYDDA, AS-09UR4SYDDA


## IR receiver and display module

connected to main unit via 5 wires:
- YK (orange)
- KEY (yellow)
- SDA1 (greenish)
- +5V (blue)
- GND (purple)

JST-SM 2.5mm connector, male connector view from the key side, cables going up: leftmost = gnd

- YK and KEY seems to be high (5V) all the time, except for some noise spikes
- SDA1 carries a signal, repeating slightly faster than 1Hz, presumably bidirectional, since the front unit needs to know the current temperature.single pulse is 5/6ms. i.e. 1.2kHz

=================
## ESP 8266 troubleshooting

http://www.esp8266.com/viewtopic.php?p=24592

