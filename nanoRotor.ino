//
// nanoROTOR: a minimal ArduinoNano-based replacement for the
// control unit of a antenna rotor (AR-302).
//
// Jan 22nd and 29th, 2023 - Michele (iv3ifz) & Bruno (iv3kyq)
//

#include "config.h"            // Configuration file
#include "utility_functions.h" // Utility functions

void setup()
{
  Serial.begin(9600); // Serial port is configured at 9600 bauds

  pinMode(RELAY1, OUTPUT); // RELAY1 pin is configured as OUTPUT
  pinMode(RELAY2, OUTPUT); // RELAY2 pin is configured as OUTPUT

  digitalWrite(RELAY1, HIGH); // NO pin is NOT connected to the common pin
  digitalWrite(RELAY2, HIGH); // NO pin is NOT connected to the common pin

  pinMode(CLK_PIN, INPUT_PULLUP); // CLK pin is configured as INPUT
  pinMode(DT_PIN, INPUT_PULLUP);  // DT pin is configured as INPUT
  pinMode(SW_PIN, INPUT_PULLUP);  // SW pin is configured as INPUT

  // set up the LCD's custom characters and initialize the LCD
  lcd.createChar(0, deg);     // create a new character (for the LCD)
  lcd.createChar(1, arrowUP); // create a new character (for the LCD)
  lcd.createChar(2, arrowDN); // create a new character (for the LCD)
  lcd.home();                 // go home
  lcd.init();                 // initialize the lcd
  lcd.backlight();            // Check also "no" back light

  GUI = 0;    // start with the "mode" GUI set to 'auto'
  STATUS = 0; // start with the "mode" status set to 'idle'

  rotorAngle_old = -1;   // force LCD update, initialising to an impossible value
  desiredAngle_old = -1; // force LCD update, initialising to an impossible value
  clockwise_old = -5;    // force LCD update, initialising to an impossible value

  // Init value for the "status" of the rotor (i.e. open-loop)

  // This is done the very first time!
  // rotorAngle = 314;
  // EEPROM.put(0, rotorAngle);

  EEPROM.get(0, rotorAngle);                  // Read the value from the EEPROM
  if ((rotorAngle > 360) || (rotorAngle < 0)) // If the value is corrupted...
  {
    lcd.clear();               // clear the display
    lcd.setCursor(0, 0);       // set the cursor to the first line
    lcd.print("EEPROM ERROR"); // print an error message
    while (1)
      ; // wait forever
  }

  // Init value for the "desired" (i.e.target) status.
  desiredAngle = rotorAngle; // Initialisation

  // Launch the booting message... before starting the main loop
  boot_message(); // Display the booting message

  lastInteraction = millis(); // Reset the last interaction timer
  Serial.println(rotorAngle);
} // end of setup()
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void loop() // Main loop
{
  switch (GUI) // GUI = 0 --> auto mode
  {            // GUI = 1 --> manual mode
  case 0:
    auto_event_handler(); // auto mode
    break;

  case 1:
    manual_event_handler(); // manual mode
    break;
  }

  EEPROM.get(0, val);          // Read the value from the EEPROM
  if (rotorAngle != val)       // If the value differs from EEPROM
    EEPROM.put(0, rotorAngle); // Store the new value in the EEPROM
} // end of loop()
  //------------------------------------------------------------------------------