//
// HARDWARE CONFIGURATION (for the Arduino Nano): do not change this section
//

#define VERSION "0.2"
#define STEP 2 // step of the knob, in degrees

// CALIBRATION
unsigned long Tcal = 81000; // in millisec - time for one full rotation (calibration)
// We can now infer the (open-loop) actual/expected/theoretical speed of the rotor [deg/msec]
float degPerMilli = (360. / (float)Tcal); // [deg/msec] = [deg] / [msec];

// 2-Relays Module
#define RELAY1 5 // CLOCKWISE MOTOR         (normally open contact: NO)
#define RELAY2 6 // COUNTER CLOCKWISE MOTOR (normally open contact: NO)

// KY-040 Rotary Encoder Module
#define CLK_PIN 2
#define DT_PIN 3
#define SW_PIN 4

// IC2 adapted for LCD 1602
// SDA (serial data) --->    A4
// SCL (serial clock) --->   A5

/* WIRING INSTRUCTIONS:

# Encoder (5 wires):

CLK -----> green  --> D2       Arduino Nano
DT  -----> yellow --> D3       Arduino Nano
SW  -----> orange --> D4       Arduino Nano
+   -----> red    --> (+5V)    external power "bus"
GND -----> brown  --> (ground) external power "bus"



# LCD / i2C (4 wires):

GND ----> black  -->  (ground) external power "bus"
VCC ----> white  -->  (+5V)    external power "bus"
SDA ----> grey   -->  A4       Arduino Nano
SCL ----> purple -->  A5       Arduino Nano



# RELAYS (4 wires):

GND ----> yellow -->  (ground) external power "bus"
IN1 ----> blue   -->  D5       Arduino Nano
IN2 ----> black  -->  D6       Arduino Nano
VCC ----> green  -->  (+5V)    external power "bus"


# ARDUINO (10 wires + USB):
D2 -----> CLK -----> green
D3 -----> DT  -----> yellow
D4 -----> SW  -----> orange
D5 -----> IN1 -----> blue
D6 -----> IN2 -----> black
A4 -----> SDA -----> grey
A5 -----> SCL -----> purple
GND ----> gray  -->  (ground) external power "bus"

*VCC ----> white -->  (+5V)    external power "bus"
*USB connector to a PC (+5V) for programming

(*): disconnect VCC when USB is connected, and vice versa!!!!

*/
