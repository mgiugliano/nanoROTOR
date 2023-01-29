//
// nanoROTOR: utility_functions.hs
//
//
// Jan 22nd, 2023 - Michele (iv3ifz) & Bruno (iv3kyq)
//

#include <EEPROM.h>            // EEPROM library, for non-volatile storage
#include <Wire.h>              // I2C library
#include <LiquidCrystal_I2C.h> // LCD display library

// GLOBAL VARIABLES
// ----------------

// User interface
int GUI = 0;                       // 0 = AUTOMATIC MODE, 1 = MANUAL MODE, 2 = CALIBRATION
int STATUS = 0;                    // 0 = IDLE, 1 = READY, 2 = ROTATING
unsigned long lastInteraction = 0; // last time [ms] of user interaction

// For the knob (button/encoder)
int lastButtonState = 0;    // previous state of the button
int longpressed = 0;        // longpress in action (boolean)
unsigned long startPressed; // the moment the button was pressed  [ms]

// For the actual motor control
unsigned long rotorAngle;       // Init value for the "status" of the rotor (i.e. open-loop)
unsigned long desiredAngle;     // Init value for the "desired" (i.e.target) status.
unsigned long rotorAngle_old;   // Old value of the rotor angle (for LCD refresh)
unsigned long desiredAngle_old; // Old value of the desired angle (for LCD refresh)
unsigned long val;              // temp variable for writing on EEPROM
int encoder_last_position = 0;  // Last position of the knob encoder
int encoder_status_old = 0;     // Past value of the knob status
int clockwise = 1;              // 1: clockwise, -1: c-clockwise, 0: zeroing, x: quit MANUAL MODE
int clockwise_old;              // 1 = clockwise, 0 = counter-clockwise (MANUAL MODE)

// Initialization of the LCD display
// set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);
// Custom symbols, done via https://maxpromer.github.io/LCD-Character-Creator/
uint8_t arrowUP[8] = {0x04, 0x0E, 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04};
uint8_t arrowDN[8] = {0x04, 0x04, 0x04, 0x04, 0x04, 0x1F, 0x0E, 0x04};
uint8_t deg[8] = {0x08, 0x14, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00};

// FUNCTIONS PROTOTYPES
void boot_message();          // prints the boot/welcome message
void lcd_refresh(int, int);   // refresh the LCD display (automatic mode)
void lcd_refresh_manual(int); // refresh the LCD display (manual mode)

void auto_event_handler();   // event handler for the automatic mode
void manual_event_handler(); // event handler for the manual mode

void checkButtonPress();
void updateState();
void callBackShortPress(unsigned long holdTime); // callback function for short press
void manual_event_handler();
int checkAbort();

//------------------------------------------------------------------------------
void boot_message() // prints the boot/welcome message
{
  lcd.clear();         // clear the display
  lcd.setCursor(0, 0); // set the cursor to the first line

  lcd.print("nanoROTOR v. ");    // print the name of the software and its version
  lcd.print(VERSION);            //
  lcd.setCursor(0, 1);           // set the cursor to the second line
  lcd.print("IV2KYQ  & IV3IFZ"); // print the name of the Authors

  delay(2000);         // wait 2 seconds, before proceeding
  lcd.clear();         // clear the display
  lcd.setCursor(0, 0); // set the cursor to the first line

  return; // return to the main program
} // end boot_message()

//------------------------------------------------
void lcd_refresh(int status, int force)
{
  // refresh the GUI, depending on the STATUS
  // possibly "forcing it" (even when no change occurred)
  char buffer[12]; // buffer for the sprintf function, LCD information

  if (force)     // The user has forced the refresh
    lcd.clear(); // clear the display

  // FIRST LINE OF THE LCD:
  // UPDATE, ONLY IF NEEDED - UNLESS FORCED
  if ((rotorAngle != rotorAngle_old) || (force))
  {
    lcd.setCursor(0, 0);                          // set the cursor to the first line
    sprintf(buffer, "state:  %03ld", rotorAngle); // prepare the string with the rotor angle
    lcd.print(buffer);                            // print  the current rotor angle
    lcd.write(0);                                 // Adds the "deg" character after the number
    rotorAngle_old = rotorAngle;                  // update the old rotor angle
  }
  // SECOND LINE OF THE LCD:
  // UPDATE, ONLY IF NEEDED (STATUS == 1 only) - UNLESS FORCED
  if ((status == 1) && ((desiredAngle != desiredAngle_old) || (force)))
  {
    lcd.setCursor(0, 1);                            // set the cursor to the second line
    sprintf(buffer, "target: %03ld", desiredAngle); // prepare the string with the desired angle
    lcd.print(buffer);                              // print  the current desired angle
    lcd.write(0);                                   // Adds the "deg" character after the number
    desiredAngle_old = desiredAngle;                // update the old desired angle

    lastInteraction = millis(); // update the last interaction time
  }

  // when the motor is moving, the second line of the LCD is updated with a message
  if (status == 2)
  {
    lcd.setCursor(0, 1);         // set the cursor to the second line
    lcd.print("[rotating....]"); // print  the current desired angle
  }

} // force_lcd_refresh()

void lcd_refresh_manual(int force)
{
  char buffer[12]; // buffer for the sprintf function, LCD information

  if (force)     // The user has forced the refresh
    lcd.clear(); // clear the display

  if ((rotorAngle != rotorAngle_old) || (clockwise != clockwise_old) || (force))
  {
    lcd.setCursor(0, 0);                          // set the cursor to the first line
    sprintf(buffer, "state:  %03ld", rotorAngle); // prepare the string with the rotor angle
    lcd.print(buffer);                            // print  the current rotor angle
    lcd.write(0);                                 // Adds the "deg" character after the number
    rotorAngle_old = rotorAngle;                  // update the old rotor angle

    if (clockwise == 1)
    {
      lcd.setCursor(15, 0);
      lcd.write(1); // This adds the "arrow up"
    }
    else if (clockwise == -1)
    {
      lcd.setCursor(15, 0);
      lcd.write(2); // This adds the "arrow down"
    }
    else if (clockwise == 0)
    {
      lcd.setCursor(15, 0);
      lcd.print("0"); // This adds the "zeroing symbol"
    }
    else
    {
      lcd.setCursor(15, 0);
      lcd.print("x");
    }
    // set the cursor to the second line
    lcd.setCursor(0, 1);        // set the cursor to the second line
    lcd.print("[manual mode]"); // print the current mode
  }
} // end lcd_refresh_manual()

//------------------------------------------------

void auto_event_handler() // this function is repeatedly called in main loop()
{
  // Declare local variables: note the use of "unsigned long" for the time variables!
  unsigned long delta, t, t0, tdelta, rotorAngle_0;
  int signDelta;
  int tmp; // temporary variable

  // Event handling, dep on the STATUS (i.e. 0 = IDLE, 1 = READY, 2 = ROTATING)
  switch (STATUS)
  {
  case 0:                   // 'idle': do nothing. Refresh LCD (iff needed)
    lcd_refresh(STATUS, 0); // refresh the (first line of the) LCD display, if needed
    // lcd.noBacklight();

    // TEST WHETHER THE KNOB HAS BEEN JUST TURNED LEFT OR RIGHT
    if ((tmp = digitalRead(CLK_PIN)) == encoder_status_old) // reads the encoder state
    {
      STATUS = 1;                 // if the knob has been turned, then the new STATUS is 'ready'
      desiredAngle = rotorAngle;  // set the desired angle to the current rotor angle
      encoder_status_old = tmp;   // update the old state of the knob
      lastInteraction = millis(); // update the last interaction time
    }
    break;

  case 1:                   // status is 'ready' (i.e. awaken up by the knob!)
    lcd_refresh(STATUS, 0); // refresh the (entire) LCD display with the current information
    // lcd.backlight();

    // CHECK FOR KNOB-TURNING EVENTS AND UPDATE THE desiredAngle accordingly
    tmp = digitalRead(CLK_PIN); // reads the encoder state
    if ((encoder_last_position == 0) && (tmp == HIGH))
    {
      if (digitalRead(DT_PIN) == LOW) // clockwise
      {
        if ((desiredAngle + STEP) > 360)
          desiredAngle = 360;
        else
          desiredAngle = desiredAngle + STEP; // update the desired angle
      }
      else
      { // counter-clockwise
        if (((float)desiredAngle - (float)STEP) < 0)
          desiredAngle = 0;
        else
          desiredAngle = desiredAngle - STEP; // update the desired angle
      }
      lastInteraction = millis(); // update the last interaction time
    }
    encoder_last_position = tmp; // update the old state of the knob

    if (((t = millis()) - lastInteraction) > 10000)
    {                         // more than 10s passed since last user "interaction"... go to IDLE
      STATUS = 0;             // go to IDLE
      lcd_refresh(STATUS, 1); // force refresh of the (entire) LCD, do that only line 1 is displayed
      lastInteraction = t;    // update the last interaction time
    }

    // CHECK WHETHER THE BUTTON HAS BEEN PRESSED
    checkButtonPress(); // check whether the button has been pressed for a short/long time
    // if (!digitalRead(SW_PIN) == HIGH) // if the button is pressed
    // {
    //   STATUS = 2; // go to 'moving' status
    // }
    break;

  case 2:                       // status is 'moving'
    digitalWrite(RELAY1, HIGH); // turn off the relays
    digitalWrite(RELAY2, HIGH); // turn off the relays
    // lcd.backlight();
    delta = (desiredAngle > rotorAngle) ? desiredAngle - rotorAngle : rotorAngle - desiredAngle;
    signDelta = (desiredAngle > rotorAngle) ? 1 : -1;

    if (delta == 0)
    {
      STATUS = 1; // go to 'ready' status
      break;
    }

    t0 = millis();
    if (signDelta >= 0) // desired angle > current angle: turn clockwise
    {
      lcd.setCursor(15, 0);
      digitalWrite(RELAY1, LOW); // only relay 1 is activated, when moving clockwise
      lcd.write(1);              // This adds the "deg" character after the number
    }
    else // desired angle < current angle: turn counter-clockwise
    {
      lcd.setCursor(15, 0);
      digitalWrite(RELAY1, LOW); // both relays are activated
      digitalWrite(RELAY2, LOW); // when moving counter-clockwise
      lcd.write(2);              // This adds the "deg" character after the number
    }
    lcd_refresh(STATUS, 0);

    tdelta = (unsigned long)((float)delta / degPerMilli);
    //  delay(tdelta);
    rotorAngle_0 = rotorAngle;
    while ((t = millis()) <= (t0 + tdelta))
    {
      rotorAngle = rotorAngle_0 + signDelta * (unsigned long)((float)(t - t0) * degPerMilli);
      // Serial.println(rotorAngle);
      lcd_refresh(1, 0);

      if (checkAbort())
      {
        digitalWrite(RELAY1, HIGH);   // turn off the relays
        digitalWrite(RELAY2, HIGH);   // turn off the relays
        lcd.clear();                  // clear the display
        lcd.setCursor(0, 0);          // set the cursor to the first line
        lcd.print("! Interrupted !"); // print the name of the software and its version
        delay(1000);
        break;
      }
    } // end while()

    digitalWrite(RELAY1, HIGH); // turn off the relays
    digitalWrite(RELAY2, HIGH); // turn off the relays

    STATUS = 0;
    lcd_refresh(STATUS, 1);

    lastInteraction = millis();
  }
} // end auto_event_handler()

void manual_event_handler()
{
  unsigned long t0;
  unsigned long rotorAngle_0;
  int tmp; // temporary variable

  lcd_refresh_manual(0);

  // CHECK FOR KNOB-TURNING EVENTS AND UPDATE THE desiredAngle accordingly
  tmp = digitalRead(CLK_PIN); // reads the encoder state
  if ((encoder_last_position == 0) && (tmp == HIGH))
  {
    switch (clockwise)
    {
    case -2:
      if (digitalRead(DT_PIN) == LOW) // clockwise
        clockwise = -1;
      else
        clockwise = 1;
      break;

    case -1:
      if (digitalRead(DT_PIN) == LOW) // clockwise
        clockwise = 0;
      else
        clockwise = -2;
      break;

    case 0:
      if (digitalRead(DT_PIN) == LOW) // clockwise
        clockwise = 1;
      else
        clockwise = -1;
      break;

    case 1:
      if (digitalRead(DT_PIN) == LOW) // clockwise
        clockwise = -2;
      else
        clockwise = 0;
      break;
    }
  }
  encoder_last_position = tmp; // update the old state of the knob

  rotorAngle_0 = rotorAngle;
  t0 = millis();
  if (clockwise == 1)
    while (!digitalRead(SW_PIN))
    {
      rotorAngle = rotorAngle_0 + clockwise * (unsigned long)((float)(millis() - t0) * degPerMilli);
      lcd_refresh_manual(0);
      digitalWrite(RELAY1, LOW); // both relays are activated
    }
  else if (clockwise == -1)
    while (!digitalRead(SW_PIN))
    {
      rotorAngle = rotorAngle_0 + clockwise * (unsigned long)((float)(millis() - t0) * degPerMilli);
      lcd_refresh_manual(0);
      digitalWrite(RELAY1, LOW); // both relays are activated
      digitalWrite(RELAY2, LOW); // when moving counter-clockwise
    }
  else if (clockwise == 0)
    while (!digitalRead(SW_PIN))
    {
      rotorAngle = 0;
      lcd_refresh_manual(1);
    }
  else if (clockwise == -2)
    while (!digitalRead(SW_PIN))
    {
      GUI = 0;
      STATUS = 0;
      lcd_refresh(STATUS, 1);
    }
  digitalWrite(RELAY1, HIGH); // both relays are activated
  digitalWrite(RELAY2, HIGH); // when moving counter-clockwise
} // end manual_event_handler()
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void checkButtonPress()
{
  int buttonState;                    // current state of the button
  buttonState = !digitalRead(SW_PIN); // i.e. buttonState == HIGH, if the button is pressed
  if (buttonState != lastButtonState) // The button's state has changed!
  {
    updateState();                 // This runs only once.
    lastButtonState = buttonState; // save state for next loop
  }

} // end checkButtonPress()
//--------------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void updateState()                        // This runs only once.
{                                         // The button's state just changed (pressed/released)
  unsigned long holdTime = 0;             // how long the button was hold       [ms]
  int buttonState = !digitalRead(SW_PIN); // i.e. buttonState == HIGH, if the button is pressed

  if (buttonState == HIGH)   // current state of the button
    startPressed = millis(); // I note down the current time, in msec
  else                       // released
  {
    holdTime = millis() - startPressed; // I compute for how long it has been pressed
    callBackShortPress(holdTime);
  }
  lastInteraction = millis(); //  update the last interaction time
} // end updateState()
//--------------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void callBackShortPress(unsigned long holdTime) // callback function
{
  if (holdTime < 30.) // 30 ms is the *minimum* time for a debounced switch
  {
    Serial.println("--- SWITCH BOUNCING DETECTED --- : IGNORE");
    STATUS = 0; // go back to 'idle' status
  }
  else if (holdTime < 500.) // 500 ms is the *max* time for a "short press"
  {
    Serial.println("Short-press! (<0.5s)");
    STATUS = 2; // go from the 'idle' status to the 'moving' status
  }
  else if (holdTime <= 2000.) // 2 s is the *minimum* time for a "long press"
  {                           // Long press implies that the user wants to set the position to 0 deg
    Serial.println("Longer-press! (<2s)");
    desiredAngle = 0;       // set the position to 0 deg
    lcd_refresh(STATUS, 1); // forced update the LCD
    delay(1000);            // wait for 1 s before going to the 'moving' status
    STATUS = 2;             // go to 'moving' status
  }
  else if (holdTime > 5000.) // 5 s is the *minimum* time for a "very long press"
  {                          // implies that the user wants to switch to the 'manual' mode
    Serial.println("Very Long-press! (>5s)");
    GUI = 1;    // GUI set to 'manual'
    STATUS = 0; // go to 'idle' status
  }
} // end callBackShortPress()
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
int checkAbort() // check if the user wants to abort the current moving operation
{
  int buttonState = !digitalRead(SW_PIN); // current state of the button

  if (buttonState != lastButtonState) // The button's state has changed!
  {
    return 1;
    lastButtonState = buttonState;
  }
  else
    return 0;
} // end checkAbort()
//------------------------------------------------------------------------------
