/* RGB Remote Control
   by: Jim Lindblom
   SparkFun Electronics
   date: October 1, 2013

   This sketch uses Ken Shirriff's *awesome* IRremote library:
       https://github.com/shirriff/Arduino-IRremote

   RGB Remote Control uses a combination of SparkFun's 
   IR Remote (https://www.sparkfun.com/products/11759) and an
   IR receiver diode (https://www.sparkfun.com/products/10266) to
   control an RGB LED.

   The IR Remote's power button turns the LED on or off. The A, B, 
   and C buttons select a channel (red, green, or blue). The up
   and down arrows increment or decrement the LED brightness on that channel.
   The left and right arrows turn a channel to min or max, and
   circle set it to the middle.

   Hardware setup:
     * The output of an IR Receiver Diode (38 kHz demodulating
       version) should be connected to the Arduino's pin 11.
       * The IR Receiver diode should also be powered off the
         Arduino's 5V and GND rails.
 */

#include <IRremote.h> // Include the IRremote library
#include <IRremoteInt.h>

/* Connect the output of the IR receiver diode. */
int RECV_PIN = 10;
/* Initialize the irrecv part of the IRremote  library */
IRrecv irrecv(RECV_PIN);
decode_results results; // This will store our IR received codes

void setup()
{
  Serial.begin(9600); // Use serial to debug.
  Serial.println("#start");
  irrecv.enableIRIn(); // Start the receiver
}

// loop() constantly checks for any received IR codes.
void loop()
{
  if (irrecv.decode(&results)) 
  {
    for (unsigned int i = 0; i < results.rawlen; i++) {
      Serial.print(results.rawbuf[i]);
      Serial.print(':');
    }
    Serial.println();
    irrecv.resume();
  }
}


