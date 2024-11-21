#include "Arduino.h" // default

#include "IR_config.h" // Define macros for input and output pin etc.
#include "IR_codes.h"

#include "ColorPallette.h"

// include IR Remote for parsing IR Signals:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\IRremote\IRremote.h"

// include output macros and sequence build for LED rendering:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\SequenceBuild\SequenceBuild_1.0.4.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\LedMacro\LedMacro_1.1.0b.h"

// include input macro and VarPar for help with IR Inputs:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\InputMacro\InputMacro_1.0.1.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\VarPar\VarPar_1.0.2.h"

// include EEPROM linked list for storing new macro sequences:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\FixedList_EEPROM\FixedList_EEPROM_1.0.0.h"

//// Pin Definitions ////
#define WhiteLed_Pin 6
#define RedLed_Pin 9
#define GreenLed_Pin 5
#define BlueLed_Pin 10
/* Warning: Do NOT use PWM pin 11. Compare Match "OC2A" interferes with IRremote Library */
/* Note: IR Pin is pinned to 'D2' by default for use of hardware interrupts */

//////////////////////////////////
//// Output Macro Definitions ////
SequenceBuild LedBuild[4]; // primary, secondary, tertiary and quaternary sequence build for flashing LED status

// give enough macros in the macro-pool for all led values:
LedMacro _Macro[20];
LedMacroManager Macro(_Macro, (sizeof(_Macro) / sizeof(_Macro[0])));

///////////////////////////
//// LED output Values ////
uint8_t MasterOnOff_val = 255; // master on/off brightness value for all LED's. Initializes 'on'

uint8_t Brightness_val = 255; // init full brightness. This values will affect all macros (primary, secondary and tertiary)

/* !! Just because a value is declared does not nessecarly mean it is used !!
      Unused values will be kept to prevent unecessary confusing access values from being used */

// primary Led Values:
uint8_t RedLed_val[4] = {0, 0, 0, 0};
uint8_t GreenLed_val[4] = {0, 0, 0, 0};
uint8_t BlueLed_val[4] = {0, 0, 0, 0};
uint8_t WhiteLed_val[4] = {0, 0, 0, 0};
uint8_t GeneralLed_val[4] = {0, 0, 0, 0}; // simply filler values for delays, ect

/*
  Mix Down Value:
    this is for secondary and tertiary macros.
    Mixes secondary and tertiary down with corresponding primary values
      0 = primary shown 100%
      255 = secondary/tertiary/quaternary shown 100%

    !! [0] is unused. It is left for better readability !!

    [1] = secondary value MixDown
    [2] = tertiary value MixDown
    ect...
*/
uint8_t RedLed_MixDown_val[4] = {-1, 0, 0, 0};
uint8_t GreenLed_MixDown_val[4] = {-1, 0, 0, 0};
uint8_t BlueLed_MixDown_val[4] = {-1, 0, 0, 0};
uint8_t WhiteLed_MixDown_val[4] = {-1, 0, 0, 0};

/*
  SpeedAdjust_val
    This will be used to adjust macro speed
    127 will be considered mid point / normal speed
*/
uint8_t SpeedAdjust_val = 127;

/*
  led value mask:
   Binary Mask: 00000000
    *note: position '0' is never used. It is left empty for better readability
    B00000000 = x_val[0] = primary only
    B00000010 = x_val[1] = secondary enable
    B00000100 = x_val[2] = tertiary enable
    B00000110 = x_val[2] = tertiary enable, secondary standby
    ect...

    Position '1'(secondary) is used for: Macro Store/Edit
    Position '2'(tertiary) is used for: Color Edit Mode
    Position '3'(quaternary) is used for: Brightness Pulse

    Use 'bitWrite' and 'bitRead' for writing and reading mask bits
*/
uint8_t RedLed_valMask = B0;
uint8_t GreenLed_valMask = B0;
uint8_t BlueLed_valMask = B0;
uint8_t WhiteLed_valMask = B0;

//////////////////////////
//// Work Mode Values ////
/*
  'MasterOnOff_state'
    Master power state for led's
*/
bool MasterOnOff_state = true;

#define WorkMode_Static 0
#define WorkMode_MacroRun 1
#define WorkMode_MacroStore 2

uint8_t WorkMode = WorkMode_Static;

#define MacroMode_Flash 0
#define MacroMode_Strobe 1
#define MacroMode_Fade 2
#define MacroMode_Smooth 3

uint8_t MacroMode;

/*
  this can be activated during static color mode or macro store
*/
bool ColorEditState = false;

/*
  'WorkMode_StaticEditColor':
    0: red-edit
    1: green-edit
    2: blue-edit
    3: white-edit

  'WorkMode_StaticEditColor' is only used when 'ColorEditState' == true
*/
uint8_t ColorEditMode;

/*
  MacroSpeedAdjustState:
    this can be toggled during MacroRun
*/
bool MacroSpeedAdjustState = false;

/*
  DimmerEnable
    for purposes of Led Status
*/
bool DimmerEnable = true;

/*
  WhiteColorCycle
    used to cycle between multiple white modes
    handled by 'StatusLed_handle'
      0 = ColorCode_white
      1 = ColorCode_warmWhite
      2 = ColorCode_coolWhite
*/
uint8_t WhiteColorCycle = 0;
/*
  WhiteCycleEnable
    conflicts with multiple functions tied to a single button require this
*/
bool WhiteCycleEnable = true;

///////////////////
//// IR Values ////
/*
  IR_PressedButton:
    0: no button pressed(Timed Out)
    > 0: Button Actively Pressed

  ToDo: Store only necessary values relative to the buttons on the remote?
        This will be does locally in logic_handle()
*/
uint32_t IR_PressedButton = 0;
/*
  IR_LastPressedButton:
    Similar to 'IR_PressedButton'
    retains pressed button value even after button is released
    Initializes with '0'
*/
uint32_t IR_LastPressedButton = 0;

/*
  IR_NewCommand:
    0: no command
    1: new command(first time)
    2: hold command
   updates each time 'IR_handle()' is called
*/

uint8_t IR_NewCommand = 0;

//////////////////////////
//// EEPROM Sequences ////

/*
  Fixed List EEPROM's
    Flash: 20 slots available, starting at add(0)
    Strobe: 20 slots available, starting at add(100)
    Fade: 20 slots available, starting at add(200)
    Smooth: 20 slots available, starting at add(300)
    */
FixedList_EEPROM EEPROM_Macro_Flash(0, 83, 4);
FixedList_EEPROM EEPROM_Macro_Strobe(100, 183, 4);
FixedList_EEPROM EEPROM_Macro_Fade(200, 283, 4);
FixedList_EEPROM EEPROM_Macro_Smooth(300, 383, 4);

void setup()
{
  // debugging:
  // Serial.begin(115200);
  // Serial.println(F("Init"));

  // init IR reciever:
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK /*(pin 13 will blink)*/);

  Macro.fps(60);

  //// Init Led Pins ////
  digitalWrite(RedLed_Pin, LOW);
  digitalWrite(GreenLed_Pin, LOW);
  digitalWrite(BlueLed_Pin, LOW);
  digitalWrite(WhiteLed_Pin, LOW);

  pinMode(RedLed_Pin, OUTPUT);
  pinMode(GreenLed_Pin, OUTPUT);
  pinMode(BlueLed_Pin, OUTPUT);
  pinMode(WhiteLed_Pin, OUTPUT);

  delay(200);

  // Serial.println(F("Init Default Color"));
  runColor(ColorCode_default, 2000, 0);
}

void loop()
{
  // run all handles:
  Led_handle();
  IR_handle();
  // Logic_handle();  // running this is handled by ir
  SpeedAdjust_handle();
}

void Led_handle()
{
  Macro.run();
  for (uint8_t x = 0; x < (sizeof(LedBuild) / sizeof(LedBuild[0])); x++)
    LedBuild[x].run();

  uint8_t brightness_val_temp;
  brightness_val_temp = map(Brightness_val, 0, 255, 0, MasterOnOff_val); // calculate total brightness value

  // run all color values:
  uint8_t color_val_temp;

  //////// Red Led ////////
  if (bitRead(RedLed_valMask, 3)) // check quaternary macro enable
  {
    color_val_temp = map(RedLed_MixDown_val[3], 0, 255, RedLed_val[0], RedLed_val[3]);
  }
  else if (bitRead(RedLed_valMask, 2)) // check tertiary macro enable
  {
    color_val_temp = map(RedLed_MixDown_val[2], 0, 255, RedLed_val[0], RedLed_val[2]);
  }
  else if (bitRead(RedLed_valMask, 1)) // check secondary macro enable
  {
    color_val_temp = map(RedLed_MixDown_val[1], 0, 255, RedLed_val[0], RedLed_val[1]);
  }
  else // primary macro only
  {
    color_val_temp = RedLed_val[0];
  }
  color_val_temp = map(color_val_temp, 0, 255, 0, brightness_val_temp); // adjust brightness
  // color_val_temp = map(color_val_temp, 0, 255, 255, 0);                 // invert value. Development Purposes only!
  analogWrite(RedLed_Pin, color_val_temp);

  //////// Green Led ////////
  if (bitRead(GreenLed_valMask, 3)) // check quaternary macro enable
  {
    color_val_temp = map(GreenLed_MixDown_val[3], 0, 255, GreenLed_val[0], GreenLed_val[3]);
  }
  else if (bitRead(GreenLed_valMask, 2)) // check tertiary macro enable
  {
    color_val_temp = map(GreenLed_MixDown_val[2], 0, 255, GreenLed_val[0], GreenLed_val[2]);
  }
  else if (bitRead(GreenLed_valMask, 1)) // check secondary macro enable
  {
    color_val_temp = map(GreenLed_MixDown_val[1], 0, 255, GreenLed_val[0], GreenLed_val[1]);
  }
  else // primary macro only
  {
    color_val_temp = GreenLed_val[0];
  }
  color_val_temp = map(color_val_temp, 0, 255, 0, brightness_val_temp); // adjust brightness
  // color_val_temp = map(color_val_temp, 0, 255, 255, 0);                 // invert value. Development Purposes only!
  analogWrite(GreenLed_Pin, color_val_temp);

  //////// Blue Led ////////
  if (bitRead(BlueLed_valMask, 3)) // check quaternary macro enable
  {
    color_val_temp = map(BlueLed_MixDown_val[3], 0, 255, BlueLed_val[0], BlueLed_val[3]);
  }
  else if (bitRead(BlueLed_valMask, 2)) // check tertiary macro enable
  {
    color_val_temp = map(BlueLed_MixDown_val[2], 0, 255, BlueLed_val[0], BlueLed_val[2]);
  }
  else if (bitRead(BlueLed_valMask, 1)) // check secondary macro enable
  {
    color_val_temp = map(BlueLed_MixDown_val[1], 0, 255, BlueLed_val[0], BlueLed_val[1]);
  }
  else // primary macro only
  {
    color_val_temp = BlueLed_val[0];
  }
  color_val_temp = map(color_val_temp, 0, 255, 0, brightness_val_temp); // adjust brightness
  // color_val_temp = map(color_val_temp, 0, 255, 255, 0);                 // invert value. Development Purposes only!
  analogWrite(BlueLed_Pin, color_val_temp);

  //////// White Led ////////
  if (bitRead(WhiteLed_valMask, 3)) // check quaternary macro enable
  {
    color_val_temp = map(WhiteLed_MixDown_val[3], 0, 255, WhiteLed_val[0], WhiteLed_val[3]);
  }
  else if (bitRead(WhiteLed_valMask, 2)) // check tertiary macro enable
  {
    color_val_temp = map(WhiteLed_MixDown_val[2], 0, 255, WhiteLed_val[0], WhiteLed_val[2]);
  }
  else if (bitRead(WhiteLed_valMask, 1)) // check secondary macro enable
  {
    color_val_temp = map(WhiteLed_MixDown_val[1], 0, 255, WhiteLed_val[0], WhiteLed_val[1]);
  }
  else // primary macro only
  {
    color_val_temp = WhiteLed_val[0];
  }
  color_val_temp = map(color_val_temp, 0, 255, 0, brightness_val_temp); // adjust brightness
  // color_val_temp = map(color_val_temp, 0, 255, 255, 0);                 // invert value. Development Purposes only!
  analogWrite(WhiteLed_Pin, color_val_temp);
}

void IR_handle()
{
  static uint32_t __logicTimer__ = millis();
  const uint16_t ir_buttonHoldTimeout = 120; // ms between '0' commands for button to timeout
  static uint32_t ir_buttonTimer = 0;
  bool logicRan = false; // can't run logic handle twice in a row

  // reset Ir command flag:
  IR_NewCommand = 0;

  // check if held button has timedout:
  if (IR_PressedButton != 0 && millis() - ir_buttonTimer > ir_buttonHoldTimeout)
    IR_PressedButton = 0; // set to "no button actively pressed"

  // check for new IR commands:
  if (IrReceiver.decode())
  {
    switch (IrReceiver.decodedIRData.decodedRawData)
    {
    case 0:
    case IrCode_on:
    case IrCode_off:
    case IrCode_up:
    case IrCode_down:

    case IrCode_cont_white:
    case IrCode_cont_red:
    case IrCode_cont_green:
    case IrCode_cont_blue:

    case IrCode_red:
    case IrCode_lightRed:
    case IrCode_orange:
    case IrCode_yellow:

    case IrCode_green:
    case IrCode_cyan:
    case IrCode_lightBlue1:
    case IrCode_lightBlue2:

    case IrCode_blue:
    case IrCode_darkPurple:
    case IrCode_purple:
    case IrCode_lightPurple:

    case IrCode_flash:
    case IrCode_strobe:
    case IrCode_fade:
    case IrCode_smooth:

      ir_buttonTimer = millis(); // ir command is still unknown but timer can be reset already

      if (IrReceiver.decodedIRData.decodedRawData) // check for new IR command
      {
        IR_PressedButton = IrReceiver.decodedIRData.decodedRawData; // update pressed button
        IR_LastPressedButton = IR_PressedButton;
      }

      // determine whether new command or hold command:
      if (!IrReceiver.decodedIRData.decodedRawData && IR_PressedButton) // hold command
        IR_NewCommand = 2;                                              // raise hold flag
      else                                                              // new command
        IR_NewCommand = 1;                                              // raise new command flag

      if (!logicRan)
      {
        Logic_handle();
        logicRan = true;
        __logicTimer__ = millis();
      }
      break;
    }

    IrReceiver.resume();
  }

  if (!logicRan)
  {
    if (millis() - __logicTimer__ >= 10) // << 100hz default
    {
      Logic_handle();
      logicRan = true;
      __logicTimer__ = millis();
    }
  }
}

void volatile Logic_handle()
{
  ///////////////////////////////
  ////// Macro Preparation //////
  static InputMacro ir_commandMacro(false); // this is only used for the timers, state change will be checked manually
  static uint32_t ir_releasedButton = 0;    // This will be the ir Code of the button when released. Use this because 'IR_LastPressedButton' will change prematurely when new ir command is received

  /*
    ir_commandMacrroState_on:
      true = state change -> On
      false = no state change On

    ir_commandMacrroState_off:
      true = state change -> Off
      false = no state change Off

    Note: Both can be true at once if command was changed too fast.
  */
  bool ir_commandMacroState_on = false;
  bool ir_commandMacroState_off = false;

  /*
    Use 'ir_releasedButton' for Off state changes
    Use 'IR_PressedButton' for On state changes
    Use 'ir_releasedButton' or 'IR_LastPressedButton' for 'off' intervals
    Use 'ir_commandMacro' for current button press state 'bool'
  */

  //// State Changes ////
  if (ir_commandMacro && (IR_PressedButton == 0 || IR_NewCommand == 1)) // check 'off' state change
  {
    ir_commandMacro(false);          // set macro to 'off' state
    ir_commandMacroState_off = true; // update command state
  }
  if (!ir_commandMacro && IR_NewCommand == 1) // check 'on' state change
  {
    ir_commandMacro(true);
    ir_commandMacroState_on = true;
  }

  /////////////////////////////////////////
  ////// Run all Macros and Commands //////+

  ////// Master On/Off Commands //////
  {
    if (ir_commandMacroState_on)
    {
      switch (IR_PressedButton)
      {
      case IrCode_on:
        MasterOnOff_state = true;
        // Serial.println(F("Master: ON"));

        Macro.quadEase(MasterOnOff_val, 255, 1000);

        break;
      case IrCode_off:
        MasterOnOff_state = false;
        // Serial.println(F("Master: OFF"));

        Macro.quadEase(MasterOnOff_val, 0, 500);
        masterOff_Sequence();
        break;
      }
    }
  }

  ////// Brightness Dimmer Commands //////
  {
    const uint8_t brightness_minValue = 20;  // set minimume brightness level
    const uint8_t brightness_increment = 25; // brightness_increment amount, must be greater than 1
    static bool brightnessDimmer_enable = true;
    static uint32_t brightnessDimmer_disableTimer = 0;
    static uint32_t brightnessDimmer_disableAmount = 0;
    // check to reset dimmer enable:
    if (!brightnessDimmer_enable && (ir_commandMacroState_off || (brightnessDimmer_disableAmount > 0 && millis() - brightnessDimmer_disableTimer >= brightnessDimmer_disableAmount)))
    {
      brightnessDimmer_disableAmount = 0;
      brightnessDimmer_enable = true;
    }

    if (IR_NewCommand) // press and hold commands
    {
      ////// Dimmer Controls //////
      if (!ColorEditState && !MacroSpeedAdjustState)
      {
        switch (IR_PressedButton)
        {
        case IrCode_up:
          // Serial.println(F("Brightness Dim: UP"));

          if (brightnessDimmer_enable)
          {
            if (MasterOnOff_state)
            {
              // only run if brightness is less than max value
              if (Brightness_val < 255)
              {
                uint8_t target = Brightness_val;

                if (target > 255 - brightness_increment)
                  target = 255;
                else
                  target += brightness_increment;

                Macro.quadEase(Brightness_val, target, 90); // run very short(90ms) because dim controls are coming in constantly
              }
              else
              {
                brightnessDimmer_enable = false;
                LedBuild[3].setPrioritySequence(DimmerFlash, 0, true);
              }
            }
            else
            {
              Macro.stop(Brightness_val); // just make sure
              // ToDo: review:
              // Brightness_val = brightness_minValue;  // comment out to retain original brightness

              MasterOnOff_state = true;
              // Serial.println(F("\tMaster: ON"));

              Macro.quadEase(MasterOnOff_val, 255, 500);

              // disable dimmer temporarily:
              // brightnessDimmer_disableAmount = 300;
              // brightnessDimmer_disableTimer = millis();

              brightnessDimmer_enable = false;
            }
          }
          break;
        case IrCode_down:

          // Serial.println(F("Brightness Dim: DOWN"));

          if (brightnessDimmer_enable)
          {
            if (MasterOnOff_state)
            {
              // only run if brightness is greater than min value
              if (Brightness_val > brightness_minValue)
              {
                uint8_t target = Brightness_val;

                if (target < brightness_minValue + brightness_increment)
                  target = brightness_minValue;
                else
                  target -= brightness_increment;

                Macro.quadEase(Brightness_val, target, 90); // run very short(90ms) because dim controls are coming in constantly
              }
              else
              {
                brightnessDimmer_enable = false;
                if (ir_commandMacroState_on) // new state change command
                {
                  MasterOnOff_state = false;
                  // Serial.println(F("\tMaster: OFF"));

                  Macro.quadEase(MasterOnOff_val, 0, 300);
                  masterOff_Sequence();
                }
                else
                {
                  LedBuild[3].setPrioritySequence(DimmerFlash, 0, true);
                }
              }
            }
            else
            {
              // no point in dimming if currently off
            }
          }
          break;
        }
      }
    }
  }

  ////// Static Color Commands //////
  {
    if (ir_commandMacroState_on) // on state change
    {
      if (checkIrColor(IR_PressedButton) != -1) // check if valid color command
      {
        if (ColorEditState)
        {
          // Serial.print(F("\tStatic Color CMD: "));
          // Serial.println(checkIrColor(IR_PressedButton));

          runIrColor(IR_PressedButton, 500, 0);
        }
        else
        {
          switch (WorkMode)
          {
          case WorkMode_Static:
            // do nothing, already in static mode
            // Serial.print(F("Static Color CMD: "));
            // Serial.println(checkIrColor(IR_PressedButton));

            runIrColor(IR_PressedButton, 500, 0);
            break;
          case WorkMode_MacroRun:
            // exit macro run!
            // Serial.print(F("Static Color CMD: "));
            // Serial.println(checkIrColor(IR_PressedButton));
            WorkMode = WorkMode_Static;
            MacroSequence_Stop(); // Run-color must come AFTER this!
            // LedBuild[0].setSequence(MacroSequence_Stop, 0, true); // wont work because It interrupts other macros
            // Serial.println(F("\tMacroRun WorkMode: EXIT"));
            // Serial.println(F("\t\tStatic WorkMode: ENTER"));

            runIrColor(IR_PressedButton, 500, 0);

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false; // Exit Speed Adjust
              // Serial.println(F("\tMacro Speed Adjust: EXIT"));
            }
            break;
          case WorkMode_MacroStore:
            // static colors are used to choose macro sequences
            // Serial.print(F("\tStatic Color CMD: "));
            // Serial.println(checkIrColor(IR_PressedButton));

            runIrColor(IR_PressedButton, 500, 0);
            break;
          }
        }
        if (!MasterOnOff_state) // leds must turn on
        {
          MasterOnOff_state = true;
          // Serial.println(F("\tMaster: ON"));

          Macro.quadEase(MasterOnOff_val, 255, 1000);
        }
      }
    }
  }

  ////// Static White Color Commands //////
  {
    static uint8_t nextWhiteColor = ColorCode_white; // cycle between three types of white

    // Cycle command
    if (ir_commandMacroState_off) // check on release state change not to interfere with color edit mode
    {
      if (!ir_commandMacro.prevTriggered()) // make sure not previously triggered. When exiting color edit mode, macro is always triggered
      {
        if (!ColorEditState) // make sure not editing colors
        {
          switch (ir_releasedButton) // can't use 'IR_PressedButton' because button was just released
          {
          case IrCode_cont_white:
            ir_commandMacro.trigger();
            switch (WorkMode)
            {
            case WorkMode_Static:
              // do nothing, already in static mode
              // Serial.println(F("White Color Cycle CMD"));
              runColor(nextWhiteColor, 500, 0);
              nextWhiteColor++;
              if (nextWhiteColor > 2)
                nextWhiteColor = 0;

              break;
            case WorkMode_MacroRun:
              // exit macro run!
              // Serial.println(F("White Color Cycle CMD"));
              WorkMode = WorkMode_Static;
              MacroSequence_Stop();
              runColor(nextWhiteColor, 500, 0);
              nextWhiteColor++;
              if (nextWhiteColor > 2)
                nextWhiteColor = 0;

              // LedBuild[0].setSequence(MacroSequence_Stop, 0, true); // wont work because It interrupts other macros
              // Serial.println(F("\tMacroRun WorkMode: EXIT"));
              // Serial.println(F("\t\tStatic WorkMode: ENTER"));

              if (MacroSpeedAdjustState)
              {
                MacroSpeedAdjustState = false; // Exit Speed Adjust
                // Serial.println(F("\tMacro Speed Adjust: EXIT"));
              }
              break;
            case WorkMode_MacroStore:
              // static colors are used to choose macro sequences
              // Serial.println(F("\tWhite Color Cycle CMD"));
              runColor(nextWhiteColor, 500, 0);
              nextWhiteColor++;
              if (nextWhiteColor > 2)
                nextWhiteColor = 0;
              break;
            }
            if (!MasterOnOff_state) // leds must turn on
            {
              MasterOnOff_state = true;
              // Serial.println(F("\tMaster: ON"));

              Macro.quadEase(MasterOnOff_val, 255, 1000);
            }
            break;
          }
        }
      }
    }

    // status command (determine next White Color). TODO, Later. Something works, but not how I imagined:
    if (!ir_commandMacro && !ir_commandMacro.triggered())
    {
      if (RedLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_white].rgbw.red) &&
          GreenLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_white].rgbw.green) &&
          BlueLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_white].rgbw.blue) &&
          WhiteLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_white].rgbw.white)) // white
        nextWhiteColor = ColorCode_warmWhite;
      else if (RedLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_warmWhite].rgbw.red) &&
               GreenLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_warmWhite].rgbw.green) &&
               BlueLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_warmWhite].rgbw.blue) &&
               WhiteLed_val[0] == pgm_read_byte(ColorPalletteExtra[ColorCode_warmWhite].rgbw.white)) // warmWhite
        nextWhiteColor = ColorCode_coolWhite;
      else
        nextWhiteColor = ColorCode_white;
    }
  }

  ////// Color Edit Commands //////
  {
    const uint8_t colorEditTotal_minValue = 50;
    const uint8_t colorEdit_increment = 25;
    static bool colorDimmer_enable = true;
    const uint16_t colorEditHoldTime = 1000; // define function hold timer required to enter color edit mode
    const uint16_t colorEditTimeOut = 10000; // define how long after inactivity will color edit mode terminate

    if (!colorDimmer_enable && ir_commandMacroState_off)
    {
      colorDimmer_enable = true;
    }

    // Enter 'ColorEditState':
    if (!ColorEditState && MasterOnOff_state) // make sure not already in color edit mode and led's are on
    {
      switch (IR_PressedButton)
      {
        // make sure one of the color edit control buttons is pressed:
      case IrCode_cont_red:
      case IrCode_cont_green:
      case IrCode_cont_blue:
      case IrCode_cont_white:
        switch (WorkMode)
        {
        // don't edit colors in macro run mode:
        case WorkMode_MacroStore:
        case WorkMode_Static:
          if (ir_commandMacro && ir_commandMacro.interval() > colorEditHoldTime && !ir_commandMacro.triggered())
          {
            ir_commandMacro.trigger();
            ColorEditState = true;

            switch (IR_PressedButton)
            {
            case IrCode_cont_red:
              ColorEditMode = GetColor_red; // edit red
              break;
            case IrCode_cont_green:
              ColorEditMode = GetColor_green; // edit green
              break;
            case IrCode_cont_blue:
              ColorEditMode = GetColor_blue; // edit blue
              break;
            case IrCode_cont_white:
              ColorEditMode = GetColor_white; // edit white
              break;
            }

            LedBuild[2].reset();
            LedBuild[2].setPrioritySequence(ColorEditBegin, 0, true);

            // Serial.print(F("Color Edit: ENTER: "));
            switch (ColorEditMode)
            {
            case GetColor_red:
              // Serial.println(F("RED"));
              break;
            case GetColor_green:
              // Serial.println(F("GREEN"));
              break;
            case GetColor_blue:
              // Serial.println(F("BLUE"));
              break;
            case GetColor_white:
              // Serial.println(F("WHITE"));
              break;
            }
          }
          break;
        }
        break;
      }
    }

    // Exit 'ColorEditState' or Change 'ColorEditMode':
    if (ir_commandMacroState_on) // on state change. Exiting ColorEditState or changing ColorEditMode
    {
      if (ColorEditState)
      {
        switch (IR_PressedButton)
        {
          // make sure one of the color edit control buttons is pressed:
        case IrCode_cont_red:
        case IrCode_cont_green:
        case IrCode_cont_blue:
        case IrCode_cont_white:
          switch (IR_PressedButton)
          {
          case IrCode_cont_red:
            if (ColorEditMode == GetColor_red)
            {
              ColorEditState = false;
            }
            else
            {
              ColorEditMode = GetColor_red;
            }
            break;
          case IrCode_cont_green:
            if (ColorEditMode == GetColor_green)
            {
              ColorEditState = false;
            }
            else
            {
              ColorEditMode = GetColor_green;
            }
            break;
          case IrCode_cont_blue:
            if (ColorEditMode == GetColor_blue)
            {
              ColorEditState = false;
            }
            else
            {
              ColorEditMode = GetColor_blue;
            }
            break;
          case IrCode_cont_white:
            if (ColorEditMode == GetColor_white)
            {
              ColorEditState = false;
            }
            else
            {
              ColorEditMode = GetColor_white;
            }
            break;
          }
          ir_commandMacro.trigger(); // trigger so we don't re-enter ColorEditState
          if (ColorEditState)
          {
            LedBuild[2].reset();
            LedBuild[2].setPrioritySequence(ColorEditBegin, 0, true);

            // Serial.print(F("Color Edit: Change Mode: "));
            switch (ColorEditMode)
            {
            case GetColor_red:
              // Serial.println(F("RED"));
              break;
            case GetColor_green:
              // Serial.println(F("GREEN"));
              break;
            case GetColor_blue:
              // Serial.println(F("BLUE"));
              break;
            case GetColor_white:
              // Serial.println(F("WHITE"));
              break;
            }
          }
          else
          {
            LedBuild[2].setSequence(ColorEditFlash_endFlash, 0, true);
            // Serial.print(F("Color Edit: EXIT: "));
            switch (ColorEditMode)
            {
            case GetColor_red:
              // Serial.println(F("RED"));
              break;
            case GetColor_green:
              // Serial.println(F("GREEN"));
              break;
            case GetColor_blue:
              // Serial.println(F("BLUE"));
              break;
            case GetColor_white:
              // Serial.println(F("WHITE"));
              break;
            }
          }
          break;
        }
      }
    }

    // ColorEdit Dimmer commands:
    if (IR_NewCommand) // press and hold commands
    {
      if (ColorEditState)
      {
        switch (IR_PressedButton)
        {
        case IrCode_up:
          // Serial.print(F("\tColor Dim: UP: "));
          switch (ColorEditMode)
          {
          case GetColor_red:
            // Serial.println(F("RED"));
            break;
          case GetColor_green:
            // Serial.println(F("GREEN"));
            break;
          case GetColor_blue:
            // Serial.println(F("BLUE"));
            break;
          case GetColor_white:
            // Serial.println(F("WHITE"));
            break;
          }

          if (colorDimmer_enable)
          {
            uint8_t *ledEdit;
            switch (ColorEditMode)
            {
            case GetColor_red:
              ledEdit = &RedLed_val[0];
              break;
            case GetColor_green:
              ledEdit = &GreenLed_val[0];
              break;
            case GetColor_blue:
              ledEdit = &BlueLed_val[0];
              break;
            case GetColor_white:
              ledEdit = &WhiteLed_val[0];
              break;
            }

            // only run if led is less than max value
            if (*ledEdit < 255)
            {
              uint8_t target = *ledEdit;

              if (target > 255 - colorEdit_increment)
                target = 255;
              else
                target += colorEdit_increment;

              Macro.quadEase(*ledEdit, target, 90); // run very short(90ms) because dim controls are coming in constantly
            }
            else
            {
              colorDimmer_enable = false;                            // disable dimmer
              LedBuild[3].setPrioritySequence(DimmerFlash, 0, true); // pulse briefly
            }
          }
          break;
        case IrCode_down:
          // Serial.print(F("\tColor Dim: DOWN: "));
          switch (ColorEditMode)
          {
          case GetColor_red:
            // Serial.println(F("RED"));
            break;
          case GetColor_green:
            // Serial.println(F("GREEN"));
            break;
          case GetColor_blue:
            // Serial.println(F("BLUE"));
            break;
          case GetColor_white:
            // Serial.println(F("WHITE"));
            break;
          }

          if (colorDimmer_enable)
          {
            uint8_t *ledEdit;
            switch (ColorEditMode)
            {
            case GetColor_red:
              ledEdit = &RedLed_val[0];
              break;
            case GetColor_green:
              ledEdit = &GreenLed_val[0];
              break;
            case GetColor_blue:
              ledEdit = &BlueLed_val[0];
              break;
            case GetColor_white:
              ledEdit = &WhiteLed_val[0];
              break;
            }

            uint8_t minValue = 0;
            // calculate minValue:
            if (((RedLed_val[0] + GreenLed_val[0] + BlueLed_val[0] + WhiteLed_val[0]) - *ledEdit) < colorEditTotal_minValue)
              minValue = colorEditTotal_minValue - ((RedLed_val[0] + GreenLed_val[0] + BlueLed_val[0] + WhiteLed_val[0]) - *ledEdit);

            // only run if led is more than min value
            if (*ledEdit > minValue)
            {
              uint8_t target = *ledEdit;

              if (target < minValue + colorEdit_increment)
                target = minValue;
              else
                target -= colorEdit_increment;

              Macro.quadEase(*ledEdit, target, 90); // run very short(90ms) because dim controls are coming in constantly
            }
            else
            {
              colorDimmer_enable = false;                            // disable dimmer
              LedBuild[3].setPrioritySequence(DimmerFlash, 0, true); // pulse briefly
            }
          }
          break;
        }
      }
    }

    // 'ColorEditState' Timeout
    if (ColorEditState)
    {
      /*
        ToDo:
          sometimes never timesout because of random interferences.
          make sure only valid signals are processed
      */
      if (!ir_commandMacro) // button is currently released
      {
        if (ir_commandMacro.interval() > colorEditTimeOut) // no need to trigger, 'ColorEditState' will take care of that
        {
          ColorEditState = false;

          LedBuild[2].setSequence(ColorEditFlash_endFlash, 0, true);

          // Serial.print(F("TIMEOUT: Color Edit: EXIT: "));
          switch (ColorEditMode)
          {
          case GetColor_red:
            // Serial.println(F("RED"));
            break;
          case GetColor_green:
            // Serial.println(F("GREEN"));
            break;
          case GetColor_blue:
            // Serial.println(F("BLUE"));
            break;
          case GetColor_white:
            // Serial.println(F("WHITE"));
            break;
          }
        }
      }
    }

    // color edit cooldown macro function:
    if (ColorEditState)
    {
      if (IR_NewCommand)
      {
        // only valid codes, because of glitchy ir sensor:
        switch (IR_PressedButton)
        {
        case IrCode_on:
        case IrCode_off:
        case IrCode_up:
        case IrCode_down:

        case IrCode_cont_white:
        case IrCode_cont_red:
        case IrCode_cont_green:
        case IrCode_cont_blue:

        case IrCode_red:
        case IrCode_lightRed:
        case IrCode_orange:
        case IrCode_yellow:

        case IrCode_green:
        case IrCode_cyan:
        case IrCode_lightBlue1:
        case IrCode_lightBlue2:

        case IrCode_blue:
        case IrCode_darkPurple:
        case IrCode_purple:
        case IrCode_lightPurple:

        case IrCode_flash:
        case IrCode_strobe:
        case IrCode_fade:
        case IrCode_smooth:
          // restart / start cooldown
          LedBuild[2].setSequence(ColorEditFlashCooldown, 0, true);
          break;
        }
      }
    }
  }

  ////// Macro Run Commands //////
  {
    // Enter MacroRun Command:
    if (ir_commandMacroState_off) // check on release
    {
      if (!ir_commandMacro.prevTriggered()) // Make sure MacroStore wasn't just entered
      {
        if (!ColorEditState) // can't start macro while in color edit
        {
          switch (WorkMode)
          {
            // make sure in static mode or macro run mode
          case WorkMode_Static:
            switch (ir_releasedButton) // can't use 'IR_PressedButton' because button was just released
            {
              // make sure one of the correct buttons was pressed:
            case IrCode_flash:
            case IrCode_strobe:
            case IrCode_fade:
            case IrCode_smooth:
              switch (ir_releasedButton) // can't use 'IR_PressedButton' because button was just released
              {
              case IrCode_flash:
                MacroMode = MacroMode_Flash;
                LedBuild[0].setSequence(Flash_MacroSequence, 0, true);
                // Serial.print(F("MacroRun WorkMode: ENTER: "));
                // Serial.println(F("FLASH"));
                // Serial.println(F("\tStatic WorkMode: EXIT"));
                break;
              case IrCode_strobe:
                MacroMode = MacroMode_Strobe;
                LedBuild[0].setSequence(Strobe_MacroSequence, 0, true);
                // Serial.print(F("MacroRun WorkMode: ENTER: "));
                // Serial.println(F("STROBE"));
                // Serial.println(F("\tStatic WorkMode: EXIT"));
                break;
              case IrCode_fade:
                MacroMode = MacroMode_Fade;
                LedBuild[0].setSequence(Fade_MacroSequence, 0, true);
                // Serial.print(F("MacroRun WorkMode: ENTER: "));
                // Serial.println(F("FADE"));
                // Serial.println(F("\tStatic WorkMode: EXIT"));
                break;
              case IrCode_smooth:
                MacroMode = MacroMode_Smooth;
                LedBuild[0].setSequence(Smooth_MacroSequence, 0, true);
                // Serial.print(F("MacroRun WorkMode: ENTER: "));
                // Serial.println(F("SMOOTH"));
                // Serial.println(F("\tStatic WorkMode: EXIT"));
                break;
              }
              WorkMode = WorkMode_MacroRun;
              break;
            }
            break;
          }
        }
      }
    }

    // Change MacroRun Command:
    if (ir_commandMacroState_on) // check on press
    {
      switch (WorkMode)
      {
      case WorkMode_MacroRun:
        switch (IR_PressedButton)
        {
        case IrCode_flash:
          if (MacroMode != MacroMode_Flash) // Make sure the specified macro is not already running
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Flash;
            LedBuild[0].setSequence(Flash_MacroSequence, 0, true);
            // Serial.print(F("MacroRun WorkMode: CHANGE: "));
            // Serial.println(F("FLASH"));

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false; // Exit Speed Adjust on Macro run change
              // Serial.println(F("\tMacro Speed Adjust: EXIT")); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        case IrCode_strobe:
          if (MacroMode != MacroMode_Strobe) // check if already running this macro
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Strobe;
            LedBuild[0].setSequence(Strobe_MacroSequence, 0, true);
            // Serial.print(F("MacroRun WorkMode: CHANGE: "));
            // Serial.println(F("STROBE"));

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false; // Exit Speed Adjust on Macro run change
              // Serial.println(F("\tMacro Speed Adjust: EXIT")); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        case IrCode_fade:
          if (MacroMode != MacroMode_Fade) // check if already running this macro
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Fade;
            LedBuild[0].setSequence(Fade_MacroSequence, 0, true);
            // Serial.print(F("MacroRun WorkMode: CHANGE: "));
            // Serial.println(F("FADE"));

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false; // Exit Speed Adjust on Macro run change
              // Serial.println(F("\tMacro Speed Adjust: EXIT")); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        case IrCode_smooth:
          if (MacroMode != MacroMode_Smooth) // check if already running this macro
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Smooth;
            LedBuild[0].setSequence(Smooth_MacroSequence, 0, true);
            // Serial.print(F("MacroRun WorkMode: CHANGE: "));
            // Serial.println(F("SMOOTH"));

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false; // Exit Speed Adjust on Macro run change
              // Serial.println(F("\tMacro Speed Adjust: EXIT")); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        }
        break;
      }
    }
  }

  ////// Macro Store Commands //////
  {
    const uint16_t macroStoreStateHoldTime = 1000; // define function hold timer required to enter MacroStore state
    const uint16_t macroStoreColorHoldTime = 500;  // define function hold timer required to store a new Macro Color
    const uint16_t macroStoreTimeOut = 10000;      // define how much inactivity before MacroStore WorkMode Exits

    // enter command:
    if (!ColorEditState && MasterOnOff_state) // make sure not already in color edit mode and led's are on
    {
      switch (IR_PressedButton)
      {
      // make sure one of the color edit control buttons is pressed:
      case IrCode_flash:
      case IrCode_strobe:
      case IrCode_fade:
      case IrCode_smooth:
        switch (WorkMode)
        {
        // preferably start from static work mode
        // Make absolute sure not in MacroStore mode already:
        case WorkMode_Static:
          if (ir_commandMacro && ir_commandMacro.interval() > macroStoreStateHoldTime && !ir_commandMacro.triggered())
          {
            ir_commandMacro.trigger();
            WorkMode = WorkMode_MacroStore;
            LedBuild[1].setPrioritySequence(MacroStoreBegin, 0, true);

            // determine which button was pressed and set mode:
            switch (IR_PressedButton)
            {
            case IrCode_flash:
              MacroMode = MacroMode_Flash;
              EEPROM_Macro_Flash.beginStore();
              break;
            case IrCode_strobe:
              MacroMode = MacroMode_Strobe;
              EEPROM_Macro_Strobe.beginStore();
              break;
            case IrCode_fade:
              MacroMode = MacroMode_Fade;
              EEPROM_Macro_Fade.beginStore();
              break;
            case IrCode_smooth:
              MacroMode = MacroMode_Smooth;
              EEPROM_Macro_Smooth.beginStore();
              break;
            }

            // Serial.print(F("MacroStore WorkMode: ENTER: "));
            switch (MacroMode)
            {
            case MacroMode_Flash:
              // Serial.println(F("FLASH"));
              break;
            case MacroMode_Strobe:
              // Serial.println(F("STROBE"));
              break;
            case MacroMode_Fade:
              // Serial.println(F("FADE"));
              break;
            case MacroMode_Smooth:
              // Serial.println(F("SMOOTH"));
              break;
            }
            // Serial.println(F("\tStatic WorkMode: EXIT"));
          }
          break;
        }
        break;
      }
    }

    // exit command:
    if (ir_commandMacroState_off) // check on release
    {
      if (!ir_commandMacro.prevTriggered()) // Make sure MacroStore wasn't just entered
      {
        if (!ColorEditState) // can't interfere while in color edit
        {
          switch (WorkMode)
          {
            // make sure already in macro store mode
          case WorkMode_MacroStore:

            switch (ir_releasedButton) // can't use 'IR_PressedButton' because button was just released
            {
            case IrCode_flash:
              if (MacroMode == MacroMode_Flash)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
                EEPROM_Macro_Flash.endStore();
              }
              break;
            case IrCode_strobe:
              if (MacroMode == MacroMode_Strobe)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
                EEPROM_Macro_Strobe.endStore();
              }
              break;
            case IrCode_fade:
              if (MacroMode == MacroMode_Fade)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
                EEPROM_Macro_Fade.endStore();
              }
              break;
            case IrCode_smooth:
              if (MacroMode == MacroMode_Smooth)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
                EEPROM_Macro_Smooth.endStore();
              }
              break;
            }

            if (WorkMode != WorkMode_MacroStore)
            {
              LedBuild[1].setSequence(MacroStoreFlash_endFlash, 0, true);
              // Serial.print(F("MacroStore WorkMode: EXIT: "));
              switch (MacroMode)
              {
              case MacroMode_Flash:
                // Serial.println(F("FLASH"));
                break;
              case MacroMode_Strobe:
                // Serial.println(F("STROBE"));
                break;
              case MacroMode_Fade:
                // Serial.println(F("FADE"));
                break;
              case MacroMode_Smooth:
                // Serial.println(F("SMOOTH"));
                break;
              }
              // Serial.println(F("\tStatic WorkMode: ENTER"));
            }
            break;
          }
        }
      }
    }

    // store Static color to Macro Sequence:
    switch (WorkMode)
    {
    // only store to sequence when in macro store work mode:
    case WorkMode_MacroStore:
      switch (IR_PressedButton)
      {
      // make sure one of the color edit control buttons is pressed:
      case IrCode_flash:
      case IrCode_strobe:
      case IrCode_fade:
      case IrCode_smooth:
        if (ir_commandMacro && ir_commandMacro.interval() > macroStoreColorHoldTime && !ir_commandMacro.triggered())
        {
          ir_commandMacro.trigger();

          // make sure the correct button was pressed:
          switch (IR_PressedButton)
          {
          case IrCode_flash:
            if (MacroMode == MacroMode_Flash)
            {
              // Serial.println(F("MacroStore WorkMode: Store Color: FLASH"));

              byte b[] = {RedLed_val[0], GreenLed_val[0], BlueLed_val[0], WhiteLed_val[0]};
              if (!EEPROM_Macro_Flash.storeNext(b)) // no more memory left
              {
                EEPROM_Macro_Flash.endStore();

                LedBuild[1].setSequence(MacroStoreFlash_endFlash, 0, true);
                // Serial.print(F("MacroStore WorkMode: EXIT: FLASH"));
                // Serial.println(F("\tStatic WorkMode: ENTER"));
              }
              else
              {
                LedBuild[3].setPrioritySequence(MacroStoreFlash_Pulse, 0, true);
              }
            }
            break;
          case IrCode_strobe:
            if (MacroMode == MacroMode_Strobe)
            {
              // Serial.println(F("MacroStore WorkMode: Store Color: STROBE"));

              byte b[] = {RedLed_val[0], GreenLed_val[0], BlueLed_val[0], WhiteLed_val[0]};
              if (!EEPROM_Macro_Strobe.storeNext(b)) // no more memory left
              {
                EEPROM_Macro_Strobe.endStore();

                LedBuild[1].setSequence(MacroStoreFlash_endFlash, 0, true);
                // Serial.print(F("MacroStore WorkMode: EXIT: STROBE"));
                // Serial.println(F("\tStatic WorkMode: ENTER"));
              }
              else
              {
                LedBuild[3].setPrioritySequence(MacroStoreFlash_Pulse, 0, true);
              }
            }
            break;
          case IrCode_fade:
            if (MacroMode == MacroMode_Fade)
            {
              // Serial.println(F("MacroStore WorkMode: Store Color: FADE"));

              byte b[] = {RedLed_val[0], GreenLed_val[0], BlueLed_val[0], WhiteLed_val[0]};
              if (!EEPROM_Macro_Fade.storeNext(b)) // no more memory left
              {
                EEPROM_Macro_Fade.endStore();

                LedBuild[1].setSequence(MacroStoreFlash_endFlash, 0, true);
                // Serial.print(F("MacroStore WorkMode: EXIT: FADE"));
                // Serial.println(F("\tStatic WorkMode: ENTER"));
              }
              else
              {
                LedBuild[3].setPrioritySequence(MacroStoreFlash_Pulse, 0, true);
              }
            }
            break;
          case IrCode_smooth:
            if (MacroMode == MacroMode_Smooth)
            {
              // Serial.println(F("MacroStore WorkMode: Store Color: SMOOTH"));

              byte b[] = {RedLed_val[0], GreenLed_val[0], BlueLed_val[0], WhiteLed_val[0]};
              if (!EEPROM_Macro_Smooth.storeNext(b)) // no more memory left
              {
                EEPROM_Macro_Smooth.endStore();

                LedBuild[1].setSequence(MacroStoreFlash_endFlash, 0, true);
                // Serial.print(F("MacroStore WorkMode: EXIT: SMOOTH"));
                // Serial.println(F("\tStatic WorkMode: ENTER"));
              }
              else
              {
                LedBuild[3].setPrioritySequence(MacroStoreFlash_Pulse, 0, true);
              }
            }
            break;
          }
        }
        break;
      }
      break;
    }

    // MacroStore TimeOut:
    switch (WorkMode)
    {
    case WorkMode_MacroStore:
      if (!ir_commandMacro) // button released
      {
        if (ir_commandMacro.interval() > macroStoreTimeOut) // no need to trigger, 'WorkMode' will take care of that
        {
          WorkMode = WorkMode_Static;
          switch (MacroMode)
          {
          case MacroMode_Flash:
            EEPROM_Macro_Flash.endStore();
            break;
          case MacroMode_Strobe:
            EEPROM_Macro_Strobe.endStore();
            break;
          case MacroMode_Fade:
            EEPROM_Macro_Fade.endStore();
            break;
          case MacroMode_Smooth:
            EEPROM_Macro_Smooth.endStore();
            break;
          }

          LedBuild[1].setSequence(MacroStoreFlash_endFlash, 0, true);

          // Serial.print(F("TIMEOUT: MacroStore WorkMode: EXIT: "));
          switch (MacroMode)
          {
          case MacroMode_Flash:
            // Serial.println(F("FLASH"));
            break;
          case MacroMode_Strobe:
            // Serial.println(F("STROBE"));
            break;
          case MacroMode_Fade:
            // Serial.println(F("FADE"));
            break;
          case MacroMode_Smooth:
            // Serial.println(F("SMOOTH"));
            break;
          }
          // Serial.println(F("\tStatic WorkMode: ENTER"));

          if (ColorEditState) // color edit state
          {
            ColorEditState = false;
            // Serial.print(F("\tColor Edit: EXIT: "));
            switch (ColorEditMode)
            {
            case GetColor_red:
              // Serial.println(F("RED"));
              break;
            case GetColor_green:
              // Serial.println(F("GREEN"));
              break;
            case GetColor_blue:
              // Serial.println(F("BLUE"));
              break;
            case GetColor_white:
              // Serial.println(F("WHITE"));
              break;
            }
          }
        }
      }
      break;
    }

    // Macro Store cooldown macro function:
    switch (WorkMode)
    {
    case WorkMode_MacroStore:
      if (IR_NewCommand)
      {
        // restart / start cooldown
        LedBuild[1].setSequence(MacroStoreFlashCooldown, 0, true);
      }
      break;
    }
  }

  ////// Speed Adjust Commands //////
  {
    const uint8_t speedAdjust_increment = 15; // speed adjust increment amount, must be greater than 1
    const uint16_t macroSpeedTimeOut = 5000;  // define how much inactivity before MacroStore WorkMode Exits

    // Toggle Macro Speed adjust state Command:
    if (ir_commandMacroState_on) // check on press
    {
      if (!ir_commandMacro.triggered()) // make sure MacroRun wasn't just changed
      {
        switch (WorkMode)
        {
        case WorkMode_MacroRun:
          switch (IR_PressedButton)
          {
          case IrCode_flash:
            if (MacroMode == MacroMode_Flash) // Make sure the specified macro is not already running
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state

              if (MacroMode != MacroMode_Flash) // just becomes a mess
                if (MacroSpeedAdjustState)
                  LedBuild[3].setPrioritySequence(SpeedAdjustEnterFlash, 0, true);
                else
                  LedBuild[3].setPrioritySequence(SpeedAdjustExitFlash, 0, true);
              // Serial.print(F("\tMacro Speed Adjust: "));
              // Serial.print((MacroSpeedAdjustState ? F("ENTER") : F("EXIT")));
              // Serial.print(F(": "));
              // Serial.println("FLASH");
            }
            break;
          case IrCode_strobe:
            if (MacroMode == MacroMode_Strobe) // check if already running this macro
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state

              if (MacroMode != MacroMode_Flash) // just becomes a mess
                if (MacroSpeedAdjustState)
                  LedBuild[3].setPrioritySequence(SpeedAdjustEnterFlash, 0, true);
                else
                  LedBuild[3].setPrioritySequence(SpeedAdjustExitFlash, 0, true);
              // Serial.print(F("\tMacro Speed Adjust: "));
              // Serial.print((MacroSpeedAdjustState ? F("ENTER") : F("EXIT")));
              // Serial.print(F(": "));
              // Serial.println("STROBE");
            }
            break;
          case IrCode_fade:
            if (MacroMode == MacroMode_Fade) // check if already running this macro
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state

              if (MacroMode != MacroMode_Flash) // just becomes a mess
                if (MacroSpeedAdjustState)
                  LedBuild[3].setPrioritySequence(SpeedAdjustEnterFlash, 0, true);
                else
                  LedBuild[3].setPrioritySequence(SpeedAdjustExitFlash, 0, true);
              // Serial.print(F("\tMacro Speed Adjust: "));
              // Serial.print((MacroSpeedAdjustState ? F("ENTER") : F("EXIT")));
              // Serial.print(F(": "));
              // Serial.println("FADE");
            }
            break;
          case IrCode_smooth:
            if (MacroMode == MacroMode_Smooth) // check if already running this macro
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state

              if (MacroMode != MacroMode_Flash) // just becomes a mess
                if (MacroSpeedAdjustState)
                  LedBuild[3].setPrioritySequence(SpeedAdjustEnterFlash, 0, true);
                else
                  LedBuild[3].setPrioritySequence(SpeedAdjustExitFlash, 0, true);
              // Serial.print(F("\tMacro Speed Adjust: "));
              // Serial.print((MacroSpeedAdjustState ? F("ENTER") : F("EXIT")));
              // Serial.print(F(": "));
              // Serial.println("SMOOTH");
            }
            break;
          }
          break;
        }
      }
    }

    // Macro Speed Adjust Dimmer:
    if (IR_NewCommand) // << on and hold commands
    {
      if (MacroSpeedAdjustState)
      {
        switch (IR_PressedButton)
        {
        case IrCode_up:
          // only run if speed is less than max value
          if (SpeedAdjust_val < 255)
          {
            // Serial.println(F("Macro Speed: UP"));
            uint8_t target = SpeedAdjust_val;

            if (target > 255 - speedAdjust_increment)
              target = 255;
            else
              target += speedAdjust_increment;

            Macro.quadEase(SpeedAdjust_val, target, 90); // run very short(90ms) because dim controls are coming in constantly
          }
          break;
        case IrCode_down:
          // only run if brightness is greater than min value
          if (SpeedAdjust_val > 0)
          {
            // Serial.println(F("Macro Speed: DOWN"));
            uint8_t target = SpeedAdjust_val;

            if (target < speedAdjust_increment)
              target = 0;
            else
              target -= speedAdjust_increment;

            Macro.quadEase(SpeedAdjust_val, target, 90); // run very short(90ms) because dim controls are coming in constantly
          }
          break;
        }
      }
    }

    // MacroSpeedAdjust TimeOut:
    if (MacroSpeedAdjustState)
    {
      if (!ir_commandMacro)
      {
        if (ir_commandMacro.interval() > macroSpeedTimeOut) // no need to trigger, 'MacroSpeedAdjustState' will take care of that
        {
          if (MacroSpeedAdjustState)
          {
            MacroSpeedAdjustState = false; // Exit Speed Adjust
            // LedBuild[3].setPrioritySequence(SpeedAdjustExitFlash, 0, true);  // maybe?
            // Serial.println(F("TIMEOUT: Macro Speed Adjust: EXIT"));
          }
        }
      }
    }
  }

  /* !!'ir_releasedButton' must be updated at the end of the function for expected functionally!! */
  if (ir_commandMacroState_on)
    ir_releasedButton = IR_PressedButton;

#undef ir_commandMacroState_null
#undef ir_commandMacroState_on
#undef ir_commandMacroState_off
#undef ir_commandMacroState_both
}

/*
  SpeedAdjust_handle():
    determine if in macro run and which mode,
    adjust speed accordingly
*/
void SpeedAdjust_handle()
{
  switch (WorkMode)
  {
  case WorkMode_MacroRun:
    static uint32_t timer = millis();
    if (millis() - timer >= 10) // << 100hz
    {
      timer = millis();

      volatile float newSpeed = 1.0;
      switch (MacroMode)
      {
      case MacroMode_Flash:
        newSpeed = map_f(SpeedAdjust_val, 0.1, 10);
        break;
      case MacroMode_Strobe:
        newSpeed = map_f(SpeedAdjust_val, 0.1, 10);
        break;
      case MacroMode_Fade:
        newSpeed = map_f(SpeedAdjust_val, 0.1, 10);
        break;
      case MacroMode_Smooth:
        newSpeed = map_f(SpeedAdjust_val, 0.1, 10);
        break;
      }
      Macro.mod(RedLed_val[0], newSpeed);
      Macro.mod(GreenLed_val[0], newSpeed);
      Macro.mod(BlueLed_val[0], newSpeed);
      Macro.mod(WhiteLed_val[0], newSpeed);
      Macro.mod(GeneralLed_val[0], newSpeed);
    }
    break;
  }
}

/*
  masterOff_Sequence
    Seperate method for shutting off leds to keep it consistant*/
inline void masterOff_Sequence()
{
  //// check work states ////
  switch (WorkMode) // WorkMode states
  {
  case WorkMode_Static:
    // no need to do anything...
    break;
  case WorkMode_MacroRun:
    // keep macro running in background...
    break;
  case WorkMode_MacroStore:
    WorkMode = WorkMode_Static;
    switch (MacroMode)
    {
    case MacroMode_Flash:
      EEPROM_Macro_Flash.endStore();
      break;
    case MacroMode_Strobe:
      EEPROM_Macro_Strobe.endStore();
      break;
    case MacroMode_Fade:
      EEPROM_Macro_Fade.endStore();
      break;
    case MacroMode_Smooth:
      EEPROM_Macro_Smooth.endStore();
      break;
    }
    LedBuild[1].setSequence(MacroStoreFlash_abort, 0, true);
    // Serial.print(F("\tMacroStore WorkMode: EXIT: "));
    switch (MacroMode)
    {
    case MacroMode_Flash:
      // Serial.println(F("FLASH"));
      break;
    case MacroMode_Strobe:
      // Serial.println(F("STROBE"));
      break;
    case MacroMode_Fade:
      // Serial.println(F("FADE"));
      break;
    case MacroMode_Smooth:
      // Serial.println(F("SMOOTH"));
      break;
    }
    // Serial.println(F("\t\tStatic WorkMode: ENTER"));
    break;
  }

  // color edit exit:
  if (ColorEditState) // color edit state
  {
    ColorEditState = false;
    LedBuild[2].setSequence(ColorEditFlash_abort, 0, true);
    // Serial.print(F("\tColor Edit: EXIT: "));
    switch (ColorEditMode)
    {
    case GetColor_red:
      // Serial.println(F("RED"));
      break;
    case GetColor_green:
      // Serial.println(F("GREEN"));
      break;
    case GetColor_blue:
      // Serial.println(F("BLUE"));
      break;
    case GetColor_white:
      // Serial.println(F("WHITE"));
      break;
    }
  }

  // Speed Adjust exit:
  if (MacroSpeedAdjustState)
  {
    MacroSpeedAdjustState = false; // Exit Speed Adjust
    // Serial.println(F("\tMacro Speed Adjust: EXIT"));
  }
}

/*
  getIrColor:
    input ir code and color type to retrieve color value
    if ir code does not exist in color pallette, a '0' will be returned
    if color mode is invalid, a '0' will be returned
*/
uint8_t getIrColor(uint32_t irCode, uint8_t color)
{
  static const uint8_t palletteListSize = (sizeof(IrColorPallette) / sizeof(IrColorPallette[0]));

  // find correct ir code and return color:
  for (uint8_t x = 0; x < palletteListSize; x++)
  {
    if (pgm_read_dword(&IrColorPallette[x].irCode) == irCode)
    {
      switch (color)
      {
      case GetColor_red:
        return pgm_read_byte(&IrColorPallette[x].rgbw.red);
        break;
      case GetColor_green:
        return pgm_read_byte(&IrColorPallette[x].rgbw.green);
        break;
      case GetColor_blue:
        return pgm_read_byte(&IrColorPallette[x].rgbw.blue);
        break;
      case GetColor_white:
        return pgm_read_byte(&IrColorPallette[x].rgbw.white);
        break;
      default:
        return 0; // color mode invalid
        break;
      }
    }
  }
  return 0; // invalid ir code;
}

/*
  getIrColor:
    input ir code index and color type to retrieve color value
    if index is out of range, a '0' will be returned
    if color mode is invalid, a '0' will be returned

    this is meant to run faster than 'getIrColor'
    index can be obtained by 'checkIrColor'
*/
uint8_t getIrColorByIndex(uint8_t irIndex, uint8_t color)
{
  static const uint8_t palletteListSize = (sizeof(IrColorPallette) / sizeof(IrColorPallette[0]));

  if (irIndex < palletteListSize)
  {
    switch (color)
    {
    case GetColor_red:
      return pgm_read_byte(&IrColorPallette[irIndex].rgbw.red);
      break;
    case GetColor_green:
      return pgm_read_byte(&IrColorPallette[irIndex].rgbw.green);
      break;
    case GetColor_blue:
      return pgm_read_byte(&IrColorPallette[irIndex].rgbw.blue);
      break;
    case GetColor_white:
      return pgm_read_byte(&IrColorPallette[irIndex].rgbw.white);
      break;
    default:
      return 0; // color mode invalid
      break;
    }
  }
  return 0; // invalid ir index
}

/*
  checkIrColor:
    input ir code and check if ir color code exists
    if ir code does not exist in color pallette, '-1' will be returned
    if ir code exists, the index will be returned
*/
int16_t checkIrColor(uint32_t irCode)
{
  static const uint8_t palletteListSize = (sizeof(IrColorPallette) / sizeof(IrColorPallette[0]));

  for (uint8_t x = 0; x < palletteListSize; x++)
    if (pgm_read_dword(&IrColorPallette[x].irCode) == irCode)
      return x;
  return -1;
}

/*
  runIrColor:
    input ir code and which led values to run on
    if ir code is valid, function will fade led vars to new color
    led value mode can be selected
    returns true if valid ir code
 */
bool runIrColor(uint32_t irCode, uint32_t transitionTime, uint8_t macroSelect)
{
  int16_t irIndex = checkIrColor(irCode);
  if (irIndex != -1)
  {
    // fade to new color:
    Macro.quadEase(RedLed_val[macroSelect], getIrColorByIndex(irIndex, GetColor_red), transitionTime);
    Macro.quadEase(GreenLed_val[macroSelect], getIrColorByIndex(irIndex, GetColor_green), transitionTime);
    Macro.quadEase(BlueLed_val[macroSelect], getIrColorByIndex(irIndex, GetColor_blue), transitionTime);
    Macro.quadEase(WhiteLed_val[macroSelect], getIrColorByIndex(irIndex, GetColor_white), transitionTime);
    return true;
  }
  return false;
}

bool runColor(uint8_t index, uint32_t transitionTime, uint8_t macroSelect)
{
  static const uint8_t palletteListSize = (sizeof(ColorPalletteExtra) / sizeof(ColorPalletteExtra[0]));
  if (index < palletteListSize)
  {
    // fade to new color:
    Macro.quadEase(RedLed_val[macroSelect], pgm_read_byte(&ColorPalletteExtra[index].rgbw.red), transitionTime);
    Macro.quadEase(GreenLed_val[macroSelect], pgm_read_byte(&ColorPalletteExtra[index].rgbw.green), transitionTime);
    Macro.quadEase(BlueLed_val[macroSelect], pgm_read_byte(&ColorPalletteExtra[index].rgbw.blue), transitionTime);
    Macro.quadEase(WhiteLed_val[macroSelect], pgm_read_byte(&ColorPalletteExtra[index].rgbw.white), transitionTime);
    return true;
  }
  return false;
}

float map_f(uint8_t x, float out_min, float out_max)
{
  if (x < 128)
  {
    return mapfloat(x, 0, 127, out_min, 1.0);
  }
  else
  {
    return mapfloat(x, 128, 255, 1.0, out_max);
  }
}

float mapfloat(long x, long in_min, long in_max, float out_min, float out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}

/*
  Create a general Fixed EEPROM Storage solution
    each of the (maybe all four) sequences will have a unique starting address in EEPROM
    Each starting address will depend on how many slot are needed for a given sequence

    Each sequence memory will be mapped as follows:

      - First Byte(s) (probably two bytes) will be a check sum of active memory,
        there is no need to use extra time and resources calculated unused memory space

      - Second byte will be a count of total memory slots used. This can be a zero, indicating no sequence stored
        Each memory slot will be 4 bytes long to accommodate (R, G, B, W) values

      - The remaining bytes will be organized in 4 byte sections/slots.
        The bytes within the slots will be organized as follows: [R], [G], [B], [W]


    This mechanism can/should be created as a class.
    Each class instance will be responsible for its own sequence

    For storing, there will be a beginStore(), storeNext(), and endStore() methods
      - beginStore()
          resets necessary indexes. If endStore() is called now, no slots will be stored.
          It will be as an empty sequence

          What should happen if a sequence is called at this moment? just endStore()? Ignore?

      - storeNext()
          store in next memory slot, increment total slot count.
          Will only work if beginStore() was called, return store status, maybe slots available?

      - endStore()
          Finalize storing, calculate checkSum and total slots and store

          What to do if beginStore() was never called? Ignore?

    For reading sequences, there will be beginRead(), readNext(), and read()
      beginRead()
        Checks CheckSum and wether any slots are stored
        Returns number of slots available to read
        If checksum fails or no slots available, a '0' will be returned
        resets index for reading in a sequence

      readNext()
        will read next slot available, if any. If end is reached, it will loop back
        data can be returned as a char array[]

      read()
        Input a specific index and read from it. It will set global reading index too
        If index is invalid, default values will be returned '0' and read index will  not be altered
*/

/*
  DimmerFlash:
    pulse Brightness briefly

    Macro [3]
*/
SB_FUNCT(DimmerFlash, Macro.ready(RedLed_val[3]) &&
                          Macro.ready(GreenLed_val[3]) &&
                          Macro.ready(BlueLed_val[3]) &&
                          Macro.ready(WhiteLed_val[3]) &&
                          Macro.ready(GeneralLed_val[3]) &&
                          Macro.ready(RedLed_MixDown_val[3]) &&
                          Macro.ready(GreenLed_MixDown_val[3]) &&
                          Macro.ready(BlueLed_MixDown_val[3]) &&
                          Macro.ready(WhiteLed_MixDown_val[3]))
// Index 0: Initiate sequence
SB_STEP(
    RedLed_val[3] = 0;
    GreenLed_val[3] = 0;
    BlueLed_val[3] = 0;
    WhiteLed_val[3] = 0;

    // GeneralLed_val[3] = 0;

    RedLed_MixDown_val[3] = 0;
    GreenLed_MixDown_val[3] = 0;
    BlueLed_MixDown_val[3] = 0;
    WhiteLed_MixDown_val[3] = 0;

    Macro.stop(RedLed_val[3]);
    Macro.stop(GreenLed_val[3]);
    Macro.stop(BlueLed_val[3]);
    Macro.stop(WhiteLed_val[3]);

    Macro.stop(GeneralLed_val[3]);

    // Macro.stop(RedLed_MixDown_val[3]);
    // Macro.stop(GreenLed_MixDown_val[3]);
    // Macro.stop(BlueLed_MixDown_val[3]);
    // Macro.stop(WhiteLed_MixDown_val[3]);

    bitWrite(RedLed_valMask, 3, true);
    bitWrite(GreenLed_valMask, 3, true);
    bitWrite(BlueLed_valMask, 3, true);
    bitWrite(WhiteLed_valMask, 3, true);

    Macro.quadEase(RedLed_MixDown_val[3], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 3, false);
    bitWrite(GreenLed_valMask, 3, false);
    bitWrite(BlueLed_valMask, 3, false);
    bitWrite(WhiteLed_valMask, 3, false);

    Macro.delay(GeneralLed_val[3], 300);) // cool down
SB_STEP(
    _this->reset();)
SB_END

////////////////////////////////////////////////////////////////
//////////////////// Speed Adjust Sequences ////////////////////
////////////////////////////////////////////////////////////////

/*
  SpeedAdjustEnterFlash:
    pulse Green briefly

    Macro [3]
*/
SB_FUNCT(SpeedAdjustEnterFlash, Macro.ready(RedLed_val[3]) &&
                                    Macro.ready(GreenLed_val[3]) &&
                                    Macro.ready(BlueLed_val[3]) &&
                                    Macro.ready(WhiteLed_val[3]) &&
                                    Macro.ready(GeneralLed_val[3]) &&
                                    Macro.ready(RedLed_MixDown_val[3]) &&
                                    Macro.ready(GreenLed_MixDown_val[3]) &&
                                    Macro.ready(BlueLed_MixDown_val[3]) &&
                                    Macro.ready(WhiteLed_MixDown_val[3]))
// Index 0: Initiate sequence
SB_STEP(
    RedLed_val[3] = 0;
    GreenLed_val[3] = 255;
    BlueLed_val[3] = 0;
    WhiteLed_val[3] = 0;

    // GeneralLed_val[3] = 0;

    RedLed_MixDown_val[3] = 0;
    GreenLed_MixDown_val[3] = 0;
    BlueLed_MixDown_val[3] = 0;
    WhiteLed_MixDown_val[3] = 0;

    Macro.stop(RedLed_val[3]);
    Macro.stop(GreenLed_val[3]);
    Macro.stop(BlueLed_val[3]);
    Macro.stop(WhiteLed_val[3]);

    Macro.stop(GeneralLed_val[3]);

    // Macro.stop(RedLed_MixDown_val[3]);
    // Macro.stop(GreenLed_MixDown_val[3]);
    // Macro.stop(BlueLed_MixDown_val[3]);
    // Macro.stop(WhiteLed_MixDown_val[3]);

    bitWrite(RedLed_valMask, 3, true);
    bitWrite(GreenLed_valMask, 3, true);
    bitWrite(BlueLed_valMask, 3, true);
    bitWrite(WhiteLed_valMask, 3, true);

    Macro.quadEase(RedLed_MixDown_val[3], 0, 200);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 200);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 200);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 200);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 255, 250);
    Macro.quadEase(GreenLed_MixDown_val[3], 255, 250);
    Macro.quadEase(BlueLed_MixDown_val[3], 255, 250);
    Macro.quadEase(WhiteLed_MixDown_val[3], 255, 250);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 3, false);
    bitWrite(GreenLed_valMask, 3, false);
    bitWrite(BlueLed_valMask, 3, false);
    bitWrite(WhiteLed_valMask, 3, false);

    Macro.delay(GeneralLed_val[3], 300);) // cool down
SB_STEP(
    _this->reset();
    // LedBuild[2].setSequence(ColorEditFlashCooldown, 0, true); // not necessary (Actually dangerous)
)
SB_END

/*
  SpeedAdjustExitFlash:
    pulse Red briefly

    Macro [3]
*/
SB_FUNCT(SpeedAdjustExitFlash, Macro.ready(RedLed_val[3]) &&
                                   Macro.ready(GreenLed_val[3]) &&
                                   Macro.ready(BlueLed_val[3]) &&
                                   Macro.ready(WhiteLed_val[3]) &&
                                   Macro.ready(GeneralLed_val[3]) &&
                                   Macro.ready(RedLed_MixDown_val[3]) &&
                                   Macro.ready(GreenLed_MixDown_val[3]) &&
                                   Macro.ready(BlueLed_MixDown_val[3]) &&
                                   Macro.ready(WhiteLed_MixDown_val[3]))
// Index 0: Initiate sequence
SB_STEP(
    RedLed_val[3] = 255;
    GreenLed_val[3] = 0;
    BlueLed_val[3] = 0;
    WhiteLed_val[3] = 0;

    // GeneralLed_val[3] = 0;

    RedLed_MixDown_val[3] = 0;
    GreenLed_MixDown_val[3] = 0;
    BlueLed_MixDown_val[3] = 0;
    WhiteLed_MixDown_val[3] = 0;

    Macro.stop(RedLed_val[3]);
    Macro.stop(GreenLed_val[3]);
    Macro.stop(BlueLed_val[3]);
    Macro.stop(WhiteLed_val[3]);

    Macro.stop(GeneralLed_val[3]);

    // Macro.stop(RedLed_MixDown_val[3]);
    // Macro.stop(GreenLed_MixDown_val[3]);
    // Macro.stop(BlueLed_MixDown_val[3]);
    // Macro.stop(WhiteLed_MixDown_val[3]);

    bitWrite(RedLed_valMask, 3, true);
    bitWrite(GreenLed_valMask, 3, true);
    bitWrite(BlueLed_valMask, 3, true);
    bitWrite(WhiteLed_valMask, 3, true);

    Macro.quadEase(RedLed_MixDown_val[3], 0, 200);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 200);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 200);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 200);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 255, 250);
    Macro.quadEase(GreenLed_MixDown_val[3], 255, 250);
    Macro.quadEase(BlueLed_MixDown_val[3], 255, 250);
    Macro.quadEase(WhiteLed_MixDown_val[3], 255, 250);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 3, false);
    bitWrite(GreenLed_valMask, 3, false);
    bitWrite(BlueLed_valMask, 3, false);
    bitWrite(WhiteLed_valMask, 3, false);

    Macro.delay(GeneralLed_val[3], 300);) // cool down
SB_STEP(
    _this->reset();
    // LedBuild[2].setSequence(ColorEditFlashCooldown, 0, true); // not necessary (Actually dangerous)
)
SB_END

/////////////////////////////////////////////////////////////////////////
//////////////////// Color Edit Mode Color Sequences ////////////////////
/////////////////////////////////////////////////////////////////////////

/*
  ColorEditFlash:
    pulse Brightness briefly and reset color edit

    Macro [3]
*/
SB_FUNCT(ColorEditFlash_Pulse, Macro.ready(RedLed_val[3]) &&
                                   Macro.ready(GreenLed_val[3]) &&
                                   Macro.ready(BlueLed_val[3]) &&
                                   Macro.ready(WhiteLed_val[3]) &&
                                   Macro.ready(GeneralLed_val[3]) &&
                                   Macro.ready(RedLed_MixDown_val[3]) &&
                                   Macro.ready(GreenLed_MixDown_val[3]) &&
                                   Macro.ready(BlueLed_MixDown_val[3]) &&
                                   Macro.ready(WhiteLed_MixDown_val[3]))
// Index 0: Initiate sequence
SB_STEP(
    RedLed_val[3] = 0;
    GreenLed_val[3] = 0;
    BlueLed_val[3] = 0;
    WhiteLed_val[3] = 0;

    // GeneralLed_val[3] = 0;

    RedLed_MixDown_val[3] = 0;
    GreenLed_MixDown_val[3] = 0;
    BlueLed_MixDown_val[3] = 0;
    WhiteLed_MixDown_val[3] = 0;

    Macro.stop(RedLed_val[3]);
    Macro.stop(GreenLed_val[3]);
    Macro.stop(BlueLed_val[3]);
    Macro.stop(WhiteLed_val[3]);

    Macro.stop(GeneralLed_val[3]);

    // Macro.stop(RedLed_MixDown_val[3]);
    // Macro.stop(GreenLed_MixDown_val[3]);
    // Macro.stop(BlueLed_MixDown_val[3]);
    // Macro.stop(WhiteLed_MixDown_val[3]);

    bitWrite(RedLed_valMask, 3, true);
    bitWrite(GreenLed_valMask, 3, true);
    bitWrite(BlueLed_valMask, 3, true);
    bitWrite(WhiteLed_valMask, 3, true);

    Macro.quadEase(RedLed_MixDown_val[3], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 3, false);
    bitWrite(GreenLed_valMask, 3, false);
    bitWrite(BlueLed_valMask, 3, false);
    bitWrite(WhiteLed_valMask, 3, false);

    Macro.delay(GeneralLed_val[3], 300);) // cool down
SB_STEP(
    _this->reset();
    // LedBuild[2].setSequence(ColorEditFlashCooldown, 0, true); // not necessary (Actually dangerous)
)
SB_END

/*
  ColorEditBegin
    start sequence from here. Call as priority
    call each time new color mode is set
*/
SB_FUNCT(ColorEditBegin, Macro.ready(RedLed_val[2]) &&
                             Macro.ready(GreenLed_val[2]) &&
                             Macro.ready(BlueLed_val[2]) &&
                             Macro.ready(WhiteLed_val[2]) &&
                             Macro.ready(GeneralLed_val[2]) &&
                             Macro.ready(RedLed_MixDown_val[2]) &&
                             Macro.ready(GreenLed_MixDown_val[2]) &&
                             Macro.ready(BlueLed_MixDown_val[2]) &&
                             Macro.ready(WhiteLed_MixDown_val[2]))
SB_STEP(
    if ((bitRead(RedLed_valMask, 2) ||
         bitRead(GreenLed_valMask, 2) ||
         bitRead(BlueLed_valMask, 2) ||
         bitRead(WhiteLed_valMask, 2)) &&
        (RedLed_MixDown_val[2] != 0 ||
         GreenLed_MixDown_val[2] != 0 ||
         BlueLed_MixDown_val[2] != 0 ||
         WhiteLed_MixDown_val[2] != 0)) {
      Macro.quadEase(RedLed_val[2], 0, 100);
      Macro.quadEase(GreenLed_val[2], 0, 100);
      Macro.quadEase(BlueLed_val[2], 0, 100);
      Macro.quadEase(WhiteLed_val[2], 0, 100);

      Macro.stop(GeneralLed_val[2]);

      Macro.quadEase(RedLed_MixDown_val[2], 0, 100);
      Macro.quadEase(GreenLed_MixDown_val[2], 0, 100);
      Macro.quadEase(BlueLed_MixDown_val[2], 0, 100);
      Macro.quadEase(WhiteLed_MixDown_val[2], 0, 100);
    } else {
      Macro.stop(RedLed_val[2]);
      Macro.stop(GreenLed_val[2]);
      Macro.stop(BlueLed_val[2]);
      Macro.stop(WhiteLed_val[2]);

      Macro.stop(GeneralLed_val[2]);

      Macro.stop(RedLed_MixDown_val[2]);
      Macro.stop(GreenLed_MixDown_val[2]);
      Macro.stop(BlueLed_MixDown_val[2]);
      Macro.stop(WhiteLed_MixDown_val[2]);
    })
SB_STEP(
    RedLed_val[2] = 0;
    GreenLed_val[2] = 0;
    BlueLed_val[2] = 0;
    WhiteLed_val[2] = 0;

    // GeneralLed_val[2] = 0;

    RedLed_val[2] = 0;
    GreenLed_MixDown_val[2] = 0;
    BlueLed_MixDown_val[2] = 0;
    WhiteLed_MixDown_val[2] = 0;

    Macro.stop(RedLed_val[2]);
    Macro.stop(GreenLed_val[2]);
    Macro.stop(BlueLed_val[2]);
    Macro.stop(WhiteLed_val[2]);

    Macro.stop(GeneralLed_val[2]);

    Macro.stop(RedLed_MixDown_val[2]);
    Macro.stop(GreenLed_MixDown_val[2]);
    Macro.stop(BlueLed_MixDown_val[2]);
    Macro.stop(WhiteLed_MixDown_val[2]);

    bitWrite(RedLed_valMask, 2, true);
    bitWrite(GreenLed_valMask, 2, true);
    bitWrite(BlueLed_valMask, 2, true);
    bitWrite(WhiteLed_valMask, 2, true);

    Macro.quadEase(RedLed_MixDown_val[2], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[2], (ColorEditMode == GetColor_red) ? 255 : 0, 150);
    Macro.quadEase(GreenLed_val[2], (ColorEditMode == GetColor_green) ? 255 : 0, 150);
    Macro.quadEase(BlueLed_val[2], (ColorEditMode == GetColor_blue) ? 255 : 0, 150);
    Macro.quadEase(WhiteLed_val[2], (ColorEditMode == GetColor_white) ? 255 : 0, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[2], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 0, 150);)
SB_STEP(
    _this->reset();
    _this->setSequence(ColorEditFlashCooldown, 0, true);)
SB_END

/*
  ColorEditFlash
    determines which color is in edit mode and flash it

  Macro [2]
*/
SB_FUNCT(ColorEditFlash, Macro.ready(RedLed_val[2]) &&
                             Macro.ready(GreenLed_val[2]) &&
                             Macro.ready(BlueLed_val[2]) &&
                             Macro.ready(WhiteLed_val[2]) &&
                             Macro.ready(GeneralLed_val[2]) &&
                             Macro.ready(RedLed_MixDown_val[2]) &&
                             Macro.ready(GreenLed_MixDown_val[2]) &&
                             Macro.ready(BlueLed_MixDown_val[2]) &&
                             Macro.ready(WhiteLed_MixDown_val[2]))
// SB_STEP(
//     if (bitRead(RedLed_valMask, 2) ||
//         bitRead(GreenLed_valMask, 2) ||
//         bitRead(BlueLed_valMask, 2) ||
//         bitRead(WhiteLed_valMask, 2)) {
//       Macro.quadEase(RedLed_val[2], 0, 100);
//       Macro.quadEase(GreenLed_val[2], 0, 100);
//       Macro.quadEase(BlueLed_val[2], 0, 100);
//       Macro.quadEase(WhiteLed_val[2], 0, 100);

//       Macro.stop(GeneralLed_val[2]);

//       Macro.quadEase(RedLed_MixDown_val[2], 0, 100);
//       Macro.quadEase(GreenLed_MixDown_val[2], 0, 100);
//       Macro.quadEase(BlueLed_MixDown_val[2], 0, 100);
//       Macro.quadEase(WhiteLed_MixDown_val[2], 0, 100);
//     })
SB_STEP(
    RedLed_val[2] = 0;
    GreenLed_val[2] = 0;
    BlueLed_val[2] = 0;
    WhiteLed_val[2] = 0;

    // GeneralLed_val[2] = 0;

    RedLed_MixDown_val[2] = 0;
    GreenLed_MixDown_val[2] = 0;
    BlueLed_MixDown_val[2] = 0;
    WhiteLed_MixDown_val[2] = 0;

    Macro.stop(RedLed_val[2]);
    Macro.stop(GreenLed_val[2]);
    Macro.stop(BlueLed_val[2]);
    Macro.stop(WhiteLed_val[2]);

    Macro.stop(GeneralLed_val[2]);

    Macro.stop(RedLed_MixDown_val[2]);
    Macro.stop(GreenLed_MixDown_val[2]);
    Macro.stop(BlueLed_MixDown_val[2]);
    Macro.stop(WhiteLed_MixDown_val[2]);

    bitWrite(RedLed_valMask, 2, true);
    bitWrite(GreenLed_valMask, 2, true);
    bitWrite(BlueLed_valMask, 2, true);
    bitWrite(WhiteLed_valMask, 2, true);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[2], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[2], (ColorEditMode == GetColor_red) ? 255 : 0, 150);
    Macro.quadEase(GreenLed_val[2], (ColorEditMode == GetColor_green) ? 255 : 0, 150);
    Macro.quadEase(BlueLed_val[2], (ColorEditMode == GetColor_blue) ? 255 : 0, 150);
    Macro.quadEase(WhiteLed_val[2], (ColorEditMode == GetColor_white) ? 255 : 0, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[2], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 0, 150);)
SB_STEP(
    Macro.delay(GeneralLed_val[2], 1000);)
SB_STEP(
    _this->loop(0);)
SB_END

/*
  ColorEditFlashCooldown

  Macro [2]
*/
SB_FUNCT(ColorEditFlashCooldown, Macro.ready(GeneralLed_val[2]) &&
                                     Macro.ready(RedLed_MixDown_val[2]) &&
                                     Macro.ready(GreenLed_MixDown_val[2]) &&
                                     Macro.ready(BlueLed_MixDown_val[2]) &&
                                     Macro.ready(WhiteLed_MixDown_val[2]))
SB_STEP(
    Macro.stop(GeneralLed_val[2]);

    if (RedLed_MixDown_val[2] != 0 ||
        GreenLed_MixDown_val[2] != 0 ||
        BlueLed_MixDown_val[2] != 0 ||
        WhiteLed_MixDown_val[2] != 0) {
      Macro.quadEase(RedLed_MixDown_val[2], 0, 100);
      Macro.quadEase(GreenLed_MixDown_val[2], 0, 100);
      Macro.quadEase(BlueLed_MixDown_val[2], 0, 100);
      Macro.quadEase(WhiteLed_MixDown_val[2], 0, 100);
    } else {
      Macro.stop(RedLed_MixDown_val[2]);
      Macro.stop(GreenLed_MixDown_val[2]);
      Macro.stop(BlueLed_MixDown_val[2]);
      Macro.stop(WhiteLed_MixDown_val[2]);
    })
SB_STEP(
    _this->reset();
    _this->setSequence(ColorEditFlashCooldown_2, 0, true);)
SB_END

/*
  ColorEditFlashCooldown_2

  Macro [2]
*/
SB_FUNCT(ColorEditFlashCooldown_2, Macro.ready(GeneralLed_val[2]))
SB_STEP(
    Macro.delay(GeneralLed_val[2], 3000);)
SB_STEP(
    _this->reset();
    _this->setSequence(ColorEditFlash, 0, true);)
SB_END

/*
  ColorEditFlash_endFlash

  Macro [2]
*/
SB_FUNCT(ColorEditFlash_endFlash, Macro.ready(RedLed_val[2]) &&
                                      Macro.ready(GreenLed_val[2]) &&
                                      Macro.ready(BlueLed_val[2]) &&
                                      Macro.ready(WhiteLed_val[2]) &&
                                      Macro.ready(GeneralLed_val[2]) &&
                                      Macro.ready(RedLed_MixDown_val[2]) &&
                                      Macro.ready(GreenLed_MixDown_val[2]) &&
                                      Macro.ready(BlueLed_MixDown_val[2]) &&
                                      Macro.ready(WhiteLed_MixDown_val[2]))
SB_STEP(
    Macro.stop(RedLed_val[2]);
    Macro.stop(GreenLed_val[2]);
    Macro.stop(BlueLed_val[2]);
    Macro.stop(WhiteLed_val[2]);

    Macro.stop(GeneralLed_val[2]);

    Macro.stop(RedLed_MixDown_val[2]);
    Macro.stop(GreenLed_MixDown_val[2]);
    Macro.stop(BlueLed_MixDown_val[2]);
    Macro.stop(WhiteLed_MixDown_val[2]);

    Macro.quadEase(RedLed_val[2], 0, 150);
    Macro.quadEase(GreenLed_val[2], 0, 150);
    Macro.quadEase(BlueLed_val[2], 0, 150);
    Macro.quadEase(WhiteLed_val[2], 0, 150);

    Macro.quadEase(RedLed_MixDown_val[2], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[2], (ColorEditMode == GetColor_red) ? 255 : 0, 150);
    Macro.quadEase(GreenLed_val[2], (ColorEditMode == GetColor_green) ? 255 : 0, 150);
    Macro.quadEase(BlueLed_val[2], (ColorEditMode == GetColor_blue) ? 255 : 0, 150);
    Macro.quadEase(WhiteLed_val[2], (ColorEditMode == GetColor_white) ? 255 : 0, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[2], 0, 150);
    Macro.quadEase(GreenLed_val[2], 0, 150);
    Macro.quadEase(BlueLed_val[2], 0, 150);
    Macro.quadEase(WhiteLed_val[2], 0, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[2], (ColorEditMode == GetColor_red) ? 255 : 0, 150);
    Macro.quadEase(GreenLed_val[2], (ColorEditMode == GetColor_green) ? 255 : 0, 150);
    Macro.quadEase(BlueLed_val[2], (ColorEditMode == GetColor_blue) ? 255 : 0, 150);
    Macro.quadEase(WhiteLed_val[2], (ColorEditMode == GetColor_white) ? 255 : 0, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[2], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 2, false);
    bitWrite(GreenLed_valMask, 2, false);
    bitWrite(BlueLed_valMask, 2, false);
    bitWrite(WhiteLed_valMask, 2, false);

    _this->reset();

    if (WorkMode == WorkMode_MacroStore) {
      // reset macro store macro
      LedBuild[1].setSequence(MacroStoreFlashCooldown, 0, true);
    })
SB_END

/*
  ColorEditFlash_abort

  Macro [2]
*/
SB_FUNCT(ColorEditFlash_abort, Macro.ready(RedLed_val[2]) &&
                                   Macro.ready(GreenLed_val[2]) &&
                                   Macro.ready(BlueLed_val[2]) &&
                                   Macro.ready(WhiteLed_val[2]) &&
                                   Macro.ready(GeneralLed_val[2]) &&
                                   Macro.ready(RedLed_MixDown_val[2]) &&
                                   Macro.ready(GreenLed_MixDown_val[2]) &&
                                   Macro.ready(BlueLed_MixDown_val[2]) &&
                                   Macro.ready(WhiteLed_MixDown_val[2]))

SB_STEP(
    Macro.stop(RedLed_val[2]);
    Macro.stop(GreenLed_val[2]);
    Macro.stop(BlueLed_val[2]);
    Macro.stop(WhiteLed_val[2]);

    Macro.stop(GeneralLed_val[2]);

    Macro.stop(RedLed_MixDown_val[2]);
    Macro.stop(GreenLed_MixDown_val[2]);
    Macro.stop(BlueLed_MixDown_val[2]);
    Macro.stop(WhiteLed_MixDown_val[2]);

    Macro.quadEase(RedLed_MixDown_val[2], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[2], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[2], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[2], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 2, false);
    bitWrite(GreenLed_valMask, 2, false);
    bitWrite(BlueLed_valMask, 2, false);
    bitWrite(WhiteLed_valMask, 2, false);

    _this->reset();)
SB_END

//////////////////////////////////////////////////////////////////////////
//////////////////// Macro Store Mode Color Sequences ////////////////////
//////////////////////////////////////////////////////////////////////////

/*
  MacroStoreFlash_Pulse:
    pulse to signal new stored value, reset Macro store

    Macro [3]
*/
SB_FUNCT(MacroStoreFlash_Pulse, Macro.ready(RedLed_val[3]) &&
                                    Macro.ready(GreenLed_val[3]) &&
                                    Macro.ready(BlueLed_val[3]) &&
                                    Macro.ready(WhiteLed_val[3]) &&
                                    Macro.ready(GeneralLed_val[3]) &&
                                    Macro.ready(RedLed_MixDown_val[3]) &&
                                    Macro.ready(GreenLed_MixDown_val[3]) &&
                                    Macro.ready(BlueLed_MixDown_val[3]) &&
                                    Macro.ready(WhiteLed_MixDown_val[3]))
// Index 0: Initiate sequence
SB_STEP(
    RedLed_val[3] = 0;
    GreenLed_val[3] = 0;
    BlueLed_val[3] = 0;
    WhiteLed_val[3] = 0;

    // GeneralLed_val[3] = 0;

    RedLed_MixDown_val[3] = 0;
    GreenLed_MixDown_val[3] = 0;
    BlueLed_MixDown_val[3] = 0;
    WhiteLed_MixDown_val[3] = 0;

    Macro.stop(RedLed_val[3]);
    Macro.stop(GreenLed_val[3]);
    Macro.stop(BlueLed_val[3]);
    Macro.stop(WhiteLed_val[3]);

    Macro.stop(GeneralLed_val[3]);

    // Macro.stop(RedLed_MixDown_val[3]);
    // Macro.stop(GreenLed_MixDown_val[3]);
    // Macro.stop(BlueLed_MixDown_val[3]);
    // Macro.stop(WhiteLed_MixDown_val[3]);

    bitWrite(RedLed_valMask, 3, true);
    bitWrite(GreenLed_valMask, 3, true);
    bitWrite(BlueLed_valMask, 3, true);
    bitWrite(WhiteLed_valMask, 3, true);

    Macro.quadEase(RedLed_MixDown_val[3], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[3], 255, 100);
    Macro.quadEase(GreenLed_val[3], 0, 100);
    Macro.quadEase(BlueLed_val[3], 0, 100);
    Macro.quadEase(WhiteLed_val[3], 0, 100);)
SB_STEP(
    Macro.quadEase(RedLed_val[3], 0, 100);
    Macro.quadEase(GreenLed_val[3], 255, 100);
    Macro.quadEase(BlueLed_val[3], 0, 100);
    Macro.quadEase(WhiteLed_val[3], 0, 100);)
SB_STEP(
    Macro.quadEase(RedLed_val[3], 0, 100);
    Macro.quadEase(GreenLed_val[3], 0, 100);
    Macro.quadEase(BlueLed_val[3], 255, 100);
    Macro.quadEase(WhiteLed_val[3], 0, 100);)
SB_STEP(
    Macro.quadEase(RedLed_val[3], 0, 100);
    Macro.quadEase(GreenLed_val[3], 0, 100);
    Macro.quadEase(BlueLed_val[3], 0, 100);
    Macro.quadEase(WhiteLed_val[3], 255, 100);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[3], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[3], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[3], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[3], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 3, false);
    bitWrite(GreenLed_valMask, 3, false);
    bitWrite(BlueLed_valMask, 3, false);
    bitWrite(WhiteLed_valMask, 3, false);

    Macro.delay(GeneralLed_val[3], 300);) // cool down
SB_STEP(
    _this->reset();
    // LedBuild[2].setSequence(, 0, true); // not necessary (Actually dangerous)
)
SB_END

/*
  MacroStoreBegin
    start sequence from here.  Call as priority
*/
SB_FUNCT(MacroStoreBegin, Macro.ready(RedLed_val[1]) &&
                              Macro.ready(GreenLed_val[1]) &&
                              Macro.ready(BlueLed_val[1]) &&
                              Macro.ready(WhiteLed_val[1]) &&
                              Macro.ready(GeneralLed_val[1]) &&
                              Macro.ready(RedLed_MixDown_val[1]) &&
                              Macro.ready(GreenLed_MixDown_val[1]) &&
                              Macro.ready(BlueLed_MixDown_val[1]) &&
                              Macro.ready(WhiteLed_MixDown_val[1]))
SB_STEP(
    if ((bitRead(RedLed_valMask, 1) ||
         bitRead(GreenLed_valMask, 1) ||
         bitRead(BlueLed_valMask, 1) ||
         bitRead(WhiteLed_valMask, 1)) &&
        (RedLed_MixDown_val[1] != 0 ||
         GreenLed_MixDown_val[1] != 0 ||
         BlueLed_MixDown_val[1] != 0 ||
         WhiteLed_MixDown_val[1] != 0)) {
      Macro.quadEase(RedLed_val[1], 0, 100);
      Macro.quadEase(GreenLed_val[1], 0, 100);
      Macro.quadEase(BlueLed_val[1], 0, 100);
      Macro.quadEase(WhiteLed_val[1], 0, 100);

      Macro.stop(GeneralLed_val[1]);

      Macro.quadEase(RedLed_MixDown_val[1], 0, 100);
      Macro.quadEase(GreenLed_MixDown_val[1], 0, 100);
      Macro.quadEase(BlueLed_MixDown_val[1], 0, 100);
      Macro.quadEase(WhiteLed_MixDown_val[1], 0, 100);
    } else {
      Macro.stop(RedLed_val[1]);
      Macro.stop(GreenLed_val[1]);
      Macro.stop(BlueLed_val[1]);
      Macro.stop(WhiteLed_val[1]);

      Macro.stop(GeneralLed_val[1]);

      Macro.stop(RedLed_MixDown_val[1]);
      Macro.stop(GreenLed_MixDown_val[1]);
      Macro.stop(BlueLed_MixDown_val[1]);
      Macro.stop(WhiteLed_MixDown_val[1]);
    })
SB_STEP(
    RedLed_val[1] = 0;
    GreenLed_val[1] = 0;
    BlueLed_val[1] = 0;
    WhiteLed_val[1] = 0;

    // GeneralLed_val[1] = 0;

    RedLed_val[1] = 0;
    GreenLed_MixDown_val[1] = 0;
    BlueLed_MixDown_val[1] = 0;
    WhiteLed_MixDown_val[1] = 0;

    Macro.stop(RedLed_val[1]);
    Macro.stop(GreenLed_val[1]);
    Macro.stop(BlueLed_val[1]);
    Macro.stop(WhiteLed_val[1]);

    Macro.stop(GeneralLed_val[1]);

    Macro.stop(RedLed_MixDown_val[1]);
    Macro.stop(GreenLed_MixDown_val[1]);
    Macro.stop(BlueLed_MixDown_val[1]);
    Macro.stop(WhiteLed_MixDown_val[1]);

    bitWrite(RedLed_valMask, 1, true);
    bitWrite(GreenLed_valMask, 1, true);
    bitWrite(BlueLed_valMask, 1, true);
    bitWrite(WhiteLed_valMask, 1, true);

    Macro.quadEase(RedLed_MixDown_val[1], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[1], 255, 150);
    Macro.quadEase(GreenLed_val[1], 255, 150);
    Macro.quadEase(BlueLed_val[1], 255, 150);
    Macro.quadEase(WhiteLed_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[1], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 0, 150);)
SB_STEP(
    _this->reset();
    _this->setSequence(MacroStoreFlashCooldown, 0, true);)
SB_END

/*
  MacroStoreFlash
    determines which color is in edit mode and flash it

  Macro [1]
*/
SB_FUNCT(MacroStoreFlash, Macro.ready(RedLed_val[1]) &&
                              Macro.ready(GreenLed_val[1]) &&
                              Macro.ready(BlueLed_val[1]) &&
                              Macro.ready(WhiteLed_val[1]) &&
                              Macro.ready(GeneralLed_val[1]) &&
                              Macro.ready(RedLed_MixDown_val[1]) &&
                              Macro.ready(GreenLed_MixDown_val[1]) &&
                              Macro.ready(BlueLed_MixDown_val[1]) &&
                              Macro.ready(WhiteLed_MixDown_val[1]))
// SB_STEP(
//     if (bitRead(RedLed_valMask, 1) ||
//         bitRead(GreenLed_valMask, 1) ||
//         bitRead(BlueLed_valMask, 1) ||
//         bitRead(WhiteLed_valMask, 1)) {
//       Macro.quadEase(RedLed_val[1], 0, 100);
//       Macro.quadEase(GreenLed_val[1], 0, 100);
//       Macro.quadEase(BlueLed_val[1], 0, 100);
//       Macro.quadEase(WhiteLed_val[1], 0, 100);

//       Macro.stop(GeneralLed_val[1]);

//       Macro.quadEase(RedLed_MixDown_val[1], 0, 100);
//       Macro.quadEase(GreenLed_MixDown_val[1], 0, 100);
//       Macro.quadEase(BlueLed_MixDown_val[1], 0, 100);
//       Macro.quadEase(WhiteLed_MixDown_val[1], 0, 100);
//     })
SB_STEP(
    RedLed_val[1] = 0;
    GreenLed_val[1] = 0;
    BlueLed_val[1] = 0;
    WhiteLed_val[1] = 0;

    // GeneralLed_val[1] = 0;

    RedLed_MixDown_val[1] = 0;
    GreenLed_MixDown_val[1] = 0;
    BlueLed_MixDown_val[1] = 0;
    WhiteLed_MixDown_val[1] = 0;

    Macro.stop(RedLed_val[1]);
    Macro.stop(GreenLed_val[1]);
    Macro.stop(BlueLed_val[1]);
    Macro.stop(WhiteLed_val[1]);

    Macro.stop(GeneralLed_val[1]);

    Macro.stop(RedLed_MixDown_val[1]);
    Macro.stop(GreenLed_MixDown_val[1]);
    Macro.stop(BlueLed_MixDown_val[1]);
    Macro.stop(WhiteLed_MixDown_val[1]);

    bitWrite(RedLed_valMask, 1, true);
    bitWrite(GreenLed_valMask, 1, true);
    bitWrite(BlueLed_valMask, 1, true);
    bitWrite(WhiteLed_valMask, 1, true);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[1], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[1], 255, 150);
    Macro.quadEase(GreenLed_val[1], 255, 150);
    Macro.quadEase(BlueLed_val[1], 255, 150);
    Macro.quadEase(WhiteLed_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[1], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 0, 150);)
SB_STEP(
    Macro.delay(GeneralLed_val[1], 1000);)
SB_STEP(
    _this->loop(0);)
SB_END

/*
  MacroStoreFlashCooldown

  Macro [1]
*/
SB_FUNCT(MacroStoreFlashCooldown, Macro.ready(GeneralLed_val[1]) &&
                                      Macro.ready(RedLed_MixDown_val[1]) &&
                                      Macro.ready(GreenLed_MixDown_val[1]) &&
                                      Macro.ready(BlueLed_MixDown_val[1]) &&
                                      Macro.ready(WhiteLed_MixDown_val[1]))
SB_STEP(
    Macro.stop(GeneralLed_val[1]);

    if (RedLed_MixDown_val[1] != 0 ||
        GreenLed_MixDown_val[1] != 0 ||
        BlueLed_MixDown_val[1] != 0 ||
        WhiteLed_MixDown_val[1] != 0) {
      Macro.quadEase(RedLed_MixDown_val[1], 0, 100);
      Macro.quadEase(GreenLed_MixDown_val[1], 0, 100);
      Macro.quadEase(BlueLed_MixDown_val[1], 0, 100);
      Macro.quadEase(WhiteLed_MixDown_val[1], 0, 100);
    } else {
      Macro.stop(RedLed_MixDown_val[1]);
      Macro.stop(GreenLed_MixDown_val[1]);
      Macro.stop(BlueLed_MixDown_val[1]);
      Macro.stop(WhiteLed_MixDown_val[1]);
    })
SB_STEP(
    _this->reset();
    _this->setSequence(MacroStoreFlashCooldown_2, 0, true);)
SB_END

/*
  MacroStoreFlashCooldown_2

  Macro [1]
*/
SB_FUNCT(MacroStoreFlashCooldown_2, Macro.ready(GeneralLed_val[1]))
SB_STEP(
    Macro.delay(GeneralLed_val[1], 3000);)
SB_STEP(
    _this->reset();
    _this->setSequence(MacroStoreFlash, 0, true);)
SB_END

/*
  MacroStoreFlash_endFlash

  Macro [1]
*/
SB_FUNCT(MacroStoreFlash_endFlash, Macro.ready(RedLed_val[1]) &&
                                       Macro.ready(GreenLed_val[1]) &&
                                       Macro.ready(BlueLed_val[1]) &&
                                       Macro.ready(WhiteLed_val[1]) &&
                                       Macro.ready(GeneralLed_val[1]) &&
                                       Macro.ready(RedLed_MixDown_val[1]) &&
                                       Macro.ready(GreenLed_MixDown_val[1]) &&
                                       Macro.ready(BlueLed_MixDown_val[1]) &&
                                       Macro.ready(WhiteLed_MixDown_val[1]))
SB_STEP(
    Macro.stop(RedLed_val[1]);
    Macro.stop(GreenLed_val[1]);
    Macro.stop(BlueLed_val[1]);
    Macro.stop(WhiteLed_val[1]);

    Macro.stop(GeneralLed_val[1]);

    Macro.stop(RedLed_MixDown_val[1]);
    Macro.stop(GreenLed_MixDown_val[1]);
    Macro.stop(BlueLed_MixDown_val[1]);
    Macro.stop(WhiteLed_MixDown_val[1]);

    Macro.quadEase(RedLed_val[1], 0, 100);
    Macro.quadEase(GreenLed_val[1], 0, 100);
    Macro.quadEase(BlueLed_val[1], 0, 100);
    Macro.quadEase(WhiteLed_val[1], 0, 100);)
// A little bit was changed here, TODO: check
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[1], 255, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 255, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 255, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[1], 255, 150);
    Macro.quadEase(GreenLed_val[1], 255, 150);
    Macro.quadEase(BlueLed_val[1], 255, 150);
    Macro.quadEase(WhiteLed_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[1], 0, 150);
    Macro.quadEase(GreenLed_val[1], 0, 150);
    Macro.quadEase(BlueLed_val[1], 0, 150);
    Macro.quadEase(WhiteLed_val[1], 0, 150);)
SB_STEP(
    Macro.quadEase(RedLed_val[1], 255, 150);
    Macro.quadEase(GreenLed_val[1], 255, 150);
    Macro.quadEase(BlueLed_val[1], 255, 150);
    Macro.quadEase(WhiteLed_val[1], 255, 150);)
SB_STEP(
    Macro.quadEase(RedLed_MixDown_val[1], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 1, false);
    bitWrite(GreenLed_valMask, 1, false);
    bitWrite(BlueLed_valMask, 1, false);
    bitWrite(WhiteLed_valMask, 1, false);

    _this->reset();

    // This is only needed for Color Edit:
    // if (WorkMode == WorkMode_MacroStore){
    //     // reset macro store macro
    // }
)
SB_END

/*
  MacroStoreFlash_abort

  Macro [1]
*/
SB_FUNCT(MacroStoreFlash_abort, Macro.ready(RedLed_val[1]) &&
                                    Macro.ready(GreenLed_val[1]) &&
                                    Macro.ready(BlueLed_val[1]) &&
                                    Macro.ready(WhiteLed_val[1]) &&
                                    Macro.ready(GeneralLed_val[1]) &&
                                    Macro.ready(RedLed_MixDown_val[1]) &&
                                    Macro.ready(GreenLed_MixDown_val[1]) &&
                                    Macro.ready(BlueLed_MixDown_val[1]) &&
                                    Macro.ready(WhiteLed_MixDown_val[1]))

SB_STEP(
    Macro.stop(RedLed_val[1]);
    Macro.stop(GreenLed_val[1]);
    Macro.stop(BlueLed_val[1]);
    Macro.stop(WhiteLed_val[1]);

    Macro.stop(GeneralLed_val[1]);

    Macro.stop(RedLed_MixDown_val[1]);
    Macro.stop(GreenLed_MixDown_val[1]);
    Macro.stop(BlueLed_MixDown_val[1]);
    Macro.stop(WhiteLed_MixDown_val[1]);

    Macro.quadEase(RedLed_MixDown_val[1], 0, 150);
    Macro.quadEase(GreenLed_MixDown_val[1], 0, 150);
    Macro.quadEase(BlueLed_MixDown_val[1], 0, 150);
    Macro.quadEase(WhiteLed_MixDown_val[1], 0, 150);)
SB_STEP(
    bitWrite(RedLed_valMask, 1, false);
    bitWrite(GreenLed_valMask, 1, false);
    bitWrite(BlueLed_valMask, 1, false);
    bitWrite(WhiteLed_valMask, 1, false);

    _this->reset();)
SB_END

/////////////////////////////////////////////////////////
////////////////// Macro Run Sequences //////////////////
/////////////////////////////////////////////////////////

/*
  Flash_MacroSequence
    flashing on and off changing colors while doing so
      Pattern:
        1, off, 2, off, 3, off...
*/
const uint8_t _Flash_MacroSequence_PalletteListSize = (sizeof(MacroMode_Flash_DefaultPallette) / sizeof(MacroMode_Flash_DefaultPallette[0]));
uint8_t _Flash_MacroSequence_Index = 0;

const uint16_t _Flash_MacroSequence_DelayOn = 300;
const uint16_t _Flash_MacroSequence_DelayOff = 300;

SB_FUNCT(Flash_MacroSequence, Macro.ready(RedLed_val[0]) &&
                                  Macro.ready(GreenLed_val[0]) &&
                                  Macro.ready(BlueLed_val[0]) &&
                                  Macro.ready(WhiteLed_val[0]) &&
                                  Macro.ready(GeneralLed_val[0]))
SB_STEP(
    Macro.stop(RedLed_val[0]);
    Macro.stop(GreenLed_val[0]);
    Macro.stop(BlueLed_val[0]);
    Macro.stop(WhiteLed_val[0]);

    Macro.stop(GeneralLed_val[0]);

    _Flash_MacroSequence_Index = 0;

    SpeedAdjust_val = 127;

    // check EEPROM:
    if (EEPROM_Macro_Flash.beginRead() > 0) {
      _this->loop(4);
    }) // reset speed adjuster
SB_STEP(
    RedLed_val[0] = pgm_read_byte(&MacroMode_Flash_DefaultPallette[_Flash_MacroSequence_Index].rgbw.red);
    GreenLed_val[0] = pgm_read_byte(&MacroMode_Flash_DefaultPallette[_Flash_MacroSequence_Index].rgbw.green);
    BlueLed_val[0] = pgm_read_byte(&MacroMode_Flash_DefaultPallette[_Flash_MacroSequence_Index].rgbw.blue);
    WhiteLed_val[0] = pgm_read_byte(&MacroMode_Flash_DefaultPallette[_Flash_MacroSequence_Index].rgbw.white);

    Macro.delay(GeneralLed_val[0], _Flash_MacroSequence_DelayOn);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    RedLed_val[0] = 0;
    GreenLed_val[0] = 0;
    BlueLed_val[0] = 0;
    WhiteLed_val[0] = 0;

    Macro.delay(GeneralLed_val[0], _Flash_MacroSequence_DelayOff);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _Flash_MacroSequence_Index++;
    if (_Flash_MacroSequence_Index >= _Flash_MacroSequence_PalletteListSize)
        _Flash_MacroSequence_Index = 0;
    _this->loop(1);)

// EEPROM Section:
SB_STEP(
    byte b[4];
    EEPROM_Macro_Flash.readNext(b);

    RedLed_val[0] = b[0];
    GreenLed_val[0] = b[1];
    BlueLed_val[0] = b[2];
    WhiteLed_val[0] = b[3];

    Macro.delay(GeneralLed_val[0], _Flash_MacroSequence_DelayOn);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    RedLed_val[0] = 0;
    GreenLed_val[0] = 0;
    BlueLed_val[0] = 0;
    WhiteLed_val[0] = 0;

    Macro.delay(GeneralLed_val[0], _Flash_MacroSequence_DelayOff);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _this->loop(4);)
SB_END

/*
  Strobe_MacroSequence
    Rapid flashing alternating colors
      Pattern:
      1, 2, 3,  2, 3, 4,  3, 4, 5,  4, 5, 1,  5, 1, 2...*/
const uint8_t _Strobe_MacroSequence_PalletteListSize = (sizeof(MacroMode_Strobe_DefaultPallette) / sizeof(MacroMode_Strobe_DefaultPallette[0]));
uint8_t _Strobe_MacroSequence_EEPROM_SequenceSize = 0;
uint8_t _Strobe_MacroSequence_Index = 0;

const uint16_t _Strobe_MacroSequence_Delay = 300;

SB_FUNCT(Strobe_MacroSequence, Macro.ready(RedLed_val[0]) &&
                                   Macro.ready(GreenLed_val[0]) &&
                                   Macro.ready(BlueLed_val[0]) &&
                                   Macro.ready(WhiteLed_val[0]) &&
                                   Macro.ready(GeneralLed_val[0]))
SB_STEP(
    Macro.stop(RedLed_val[0]);
    Macro.stop(GreenLed_val[0]);
    Macro.stop(BlueLed_val[0]);
    Macro.stop(WhiteLed_val[0]);

    Macro.stop(GeneralLed_val[0]);

    _Strobe_MacroSequence_Index = 0;

    SpeedAdjust_val = 127;

    // check EEPROM:
    _Strobe_MacroSequence_EEPROM_SequenceSize = EEPROM_Macro_Strobe.beginRead();
    if (_Strobe_MacroSequence_EEPROM_SequenceSize > 1) {
      _this->loop(5);
    }) // reset speed adjuster
SB_STEP(
    uint8_t tempIndex = _Strobe_MacroSequence_Index;

    RedLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.red);
    GreenLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.green);
    BlueLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.blue);
    WhiteLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.white);

    Macro.delay(GeneralLed_val[0], _Strobe_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    uint8_t tempIndex = _Strobe_MacroSequence_Index + 1;
    if (tempIndex >= _Strobe_MacroSequence_PalletteListSize)
        tempIndex -= _Strobe_MacroSequence_PalletteListSize;

    RedLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.red);
    GreenLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.green);
    BlueLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.blue);
    WhiteLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.white);

    Macro.delay(GeneralLed_val[0], _Strobe_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    uint8_t tempIndex = _Strobe_MacroSequence_Index + 2;
    if (tempIndex >= _Strobe_MacroSequence_PalletteListSize)
        tempIndex -= _Strobe_MacroSequence_PalletteListSize;

    RedLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.red);
    GreenLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.green);
    BlueLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.blue);
    WhiteLed_val[0] = pgm_read_byte(&MacroMode_Strobe_DefaultPallette[tempIndex].rgbw.white);

    Macro.delay(GeneralLed_val[0], _Strobe_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _Strobe_MacroSequence_Index++;
    if (_Strobe_MacroSequence_Index >= _Strobe_MacroSequence_PalletteListSize)
        _Strobe_MacroSequence_Index = 0;
    _this->loop(1);)

// EEPROM Section:
SB_STEP(
    uint8_t tempIndex = _Strobe_MacroSequence_Index;
    byte b[4];
    EEPROM_Macro_Strobe.read(tempIndex, b);

    RedLed_val[0] = b[0];
    GreenLed_val[0] = b[1];
    BlueLed_val[0] = b[2];
    WhiteLed_val[0] = b[3];

    Macro.delay(GeneralLed_val[0], _Strobe_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    uint8_t tempIndex = _Strobe_MacroSequence_Index + 1;
    if (tempIndex >= _Strobe_MacroSequence_EEPROM_SequenceSize)
        tempIndex -= _Strobe_MacroSequence_EEPROM_SequenceSize;

    byte b[4];
    EEPROM_Macro_Strobe.read(tempIndex, b);

    RedLed_val[0] = b[0];
    GreenLed_val[0] = b[1];
    BlueLed_val[0] = b[2];
    WhiteLed_val[0] = b[3];

    Macro.delay(GeneralLed_val[0], _Strobe_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    uint8_t tempIndex = _Strobe_MacroSequence_Index + 2;
    if (tempIndex >= _Strobe_MacroSequence_EEPROM_SequenceSize)
        tempIndex -= _Strobe_MacroSequence_EEPROM_SequenceSize;

    byte b[4];
    EEPROM_Macro_Strobe.read(tempIndex, b);

    RedLed_val[0] = b[0];
    GreenLed_val[0] = b[1];
    BlueLed_val[0] = b[2];
    WhiteLed_val[0] = b[3];
    Macro.delay(GeneralLed_val[0], _Strobe_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _Strobe_MacroSequence_Index++;
    if (_Strobe_MacroSequence_Index >= _Strobe_MacroSequence_EEPROM_SequenceSize)
        _Strobe_MacroSequence_Index = 0;
    _this->loop(5);)
SB_END

/*
  Fade_MacroSequence
    fade in and out while alternating colors
      Pattern:
        1, off, 2, off, 3, off...
*/
const uint8_t _Fade_MacroSequence_PalletteListSize = (sizeof(MacroMode_Fade_DefaultPallette) / sizeof(MacroMode_Fade_DefaultPallette[0]));
uint8_t _Fade_MacroSequence_Index = 0;

const uint16_t _Fade_MacroSequence_TransitionOn = 2000;
const uint16_t _Fade_MacroSequence_TransitionOff = 3000;
const uint16_t _Fade_MacroSequence_DelayOn = 2000;
const uint16_t _Fade_MacroSequence_DelayOff = 0;

SB_FUNCT(Fade_MacroSequence, Macro.ready(RedLed_val[0]) &&
                                 Macro.ready(GreenLed_val[0]) &&
                                 Macro.ready(BlueLed_val[0]) &&
                                 Macro.ready(WhiteLed_val[0]) &&
                                 Macro.ready(GeneralLed_val[0]))
SB_STEP(
    Macro.stop(RedLed_val[0]);
    Macro.stop(GreenLed_val[0]);
    Macro.stop(BlueLed_val[0]);
    Macro.stop(WhiteLed_val[0]);

    Macro.stop(GeneralLed_val[0]);

    _Fade_MacroSequence_Index = 0;

    SpeedAdjust_val = 127;

    // check EEPROM:
    if (EEPROM_Macro_Fade.beginRead() > 0) {
      _this->loop(6);
    }) // reset speed adjuster
SB_STEP(
    Macro.quadEase(RedLed_val[0], pgm_read_byte(&MacroMode_Fade_DefaultPallette[_Fade_MacroSequence_Index].rgbw.red), _Fade_MacroSequence_TransitionOn);
    Macro.quadEase(GreenLed_val[0], pgm_read_byte(&MacroMode_Fade_DefaultPallette[_Fade_MacroSequence_Index].rgbw.green), _Fade_MacroSequence_TransitionOn);
    Macro.quadEase(BlueLed_val[0], pgm_read_byte(&MacroMode_Fade_DefaultPallette[_Fade_MacroSequence_Index].rgbw.blue), _Fade_MacroSequence_TransitionOn);
    Macro.quadEase(WhiteLed_val[0], pgm_read_byte(&MacroMode_Fade_DefaultPallette[_Fade_MacroSequence_Index].rgbw.white), _Fade_MacroSequence_TransitionOn);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.delay(GeneralLed_val[0], _Fade_MacroSequence_DelayOn);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.quadEase(RedLed_val[0], 0, _Fade_MacroSequence_TransitionOff);
    Macro.quadEase(GreenLed_val[0], 0, _Fade_MacroSequence_TransitionOff);
    Macro.quadEase(BlueLed_val[0], 0, _Fade_MacroSequence_TransitionOff);
    Macro.quadEase(WhiteLed_val[0], 0, _Fade_MacroSequence_TransitionOff);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.delay(GeneralLed_val[0], _Fade_MacroSequence_DelayOff);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _Fade_MacroSequence_Index++;
    if (_Fade_MacroSequence_Index >= _Fade_MacroSequence_PalletteListSize)
        _Fade_MacroSequence_Index = 0;
    _this->loop(1);)

// EEPROM Section:
SB_STEP(
    byte b[4];
    EEPROM_Macro_Fade.readNext(b);

    Macro.quadEase(RedLed_val[0], b[0], _Fade_MacroSequence_TransitionOn);
    Macro.quadEase(GreenLed_val[0], b[1], _Fade_MacroSequence_TransitionOn);
    Macro.quadEase(BlueLed_val[0], b[2], _Fade_MacroSequence_TransitionOn);
    Macro.quadEase(WhiteLed_val[0], b[3], _Fade_MacroSequence_TransitionOn);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.delay(GeneralLed_val[0], _Fade_MacroSequence_DelayOn);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.quadEase(RedLed_val[0], 0, _Fade_MacroSequence_TransitionOff);
    Macro.quadEase(GreenLed_val[0], 0, _Fade_MacroSequence_TransitionOff);
    Macro.quadEase(BlueLed_val[0], 0, _Fade_MacroSequence_TransitionOff);
    Macro.quadEase(WhiteLed_val[0], 0, _Fade_MacroSequence_TransitionOff);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.delay(GeneralLed_val[0], _Fade_MacroSequence_DelayOff);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _this->loop(6);)
SB_END

/*
  Smooth_MacroSequence
    fade alternating colors
      Pattern:
        1, 2, 3...
*/
const uint8_t _Smooth_MacroSequence_PalletteListSize = (sizeof(MacroMode_Smooth_DefaultPallette) / sizeof(MacroMode_Smooth_DefaultPallette[0]));
uint8_t _Smooth_MacroSequence_Index = 0;

const uint16_t _Smooth_MacroSequence_Transition = 4000; // in ms
const uint16_t _Smooth_MacroSequence_Delay = 2000;      // in ms

SB_FUNCT(Smooth_MacroSequence, Macro.ready(RedLed_val[0]) &&
                                   Macro.ready(GreenLed_val[0]) &&
                                   Macro.ready(BlueLed_val[0]) &&
                                   Macro.ready(WhiteLed_val[0]) &&
                                   Macro.ready(GeneralLed_val[0]))
SB_STEP(
    Macro.stop(RedLed_val[0]);
    Macro.stop(GreenLed_val[0]);
    Macro.stop(BlueLed_val[0]);
    Macro.stop(WhiteLed_val[0]);

    Macro.stop(GeneralLed_val[0]);

    _Smooth_MacroSequence_Index = 0;

    SpeedAdjust_val = 127;

    // check EEPROM:
    if (EEPROM_Macro_Smooth.beginRead() > 1) {
      _this->loop(4);
    }) // reset speed adjuster
SB_STEP(
    Macro.quadEase(RedLed_val[0], pgm_read_byte(&MacroMode_Smooth_DefaultPallette[_Smooth_MacroSequence_Index].rgbw.red), _Smooth_MacroSequence_Transition);
    Macro.quadEase(GreenLed_val[0], pgm_read_byte(&MacroMode_Smooth_DefaultPallette[_Smooth_MacroSequence_Index].rgbw.green), _Smooth_MacroSequence_Transition);
    Macro.quadEase(BlueLed_val[0], pgm_read_byte(&MacroMode_Smooth_DefaultPallette[_Smooth_MacroSequence_Index].rgbw.blue), _Smooth_MacroSequence_Transition);
    Macro.quadEase(WhiteLed_val[0], pgm_read_byte(&MacroMode_Smooth_DefaultPallette[_Smooth_MacroSequence_Index].rgbw.white), _Smooth_MacroSequence_Transition);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.delay(GeneralLed_val[0], _Smooth_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _Smooth_MacroSequence_Index++;
    if (_Smooth_MacroSequence_Index >= _Smooth_MacroSequence_PalletteListSize)
        _Smooth_MacroSequence_Index = 0;
    _this->loop(1);)

// EEPROM Section:
SB_STEP(
    byte b[4];
    EEPROM_Macro_Smooth.readNext(b);

    Macro.quadEase(RedLed_val[0], b[0], _Smooth_MacroSequence_Transition);
    Macro.quadEase(GreenLed_val[0], b[1], _Smooth_MacroSequence_Transition);
    Macro.quadEase(BlueLed_val[0], b[2], _Smooth_MacroSequence_Transition);
    Macro.quadEase(WhiteLed_val[0], b[3], _Smooth_MacroSequence_Transition);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    Macro.delay(GeneralLed_val[0], _Smooth_MacroSequence_Delay);

    SpeedAdjust_handle();) // run speed adjust handler
SB_STEP(
    _this->loop(4);)
SB_END

/*
  MacroSequence_Stop
    Stop all sequences safely
*/
void MacroSequence_Stop()
{
  Macro.stop(RedLed_val[0]);
  Macro.stop(GreenLed_val[0]);
  Macro.stop(BlueLed_val[0]);
  Macro.stop(WhiteLed_val[0]);

  Macro.stop(GeneralLed_val[0]);

  Macro.stop(RedLed_MixDown_val[0]);
  Macro.stop(GreenLed_MixDown_val[0]);
  Macro.stop(BlueLed_MixDown_val[0]);
  Macro.stop(WhiteLed_MixDown_val[0]);

  LedBuild[0].reset();
}