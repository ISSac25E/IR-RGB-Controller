#include "Arduino.h"

#include "IR_config.h" // Define macros for input and output pin etc.
#include "IR_codes.h"

#include "ColorPallette.h"

// include IR Remote for parsing IR Signals:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\IRremote\IRremote.h"

// include output macros and sequence build for LED rendering:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\SequenceBuild\SequenceBuild_1.0.4.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\LedMacro\LedMacro_1.0.0.h"

// include input macro and VarPar for help with IR Inputs:
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\InputMacro\InputMacro_1.0.1.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\VarPar\VarPar_1.0.2.h"

//// Pin Definitions ////
#define WhiteLed_Pin 6
#define RedLed_Pin 9
#define GreenLed_Pin 5
#define BlueLed_Pin 10
/* Warning: Do NOT use PWM pin 11. Compare Match "OC2A" interferes with IRremote Library */
/* Note: IR Pin is pinned to 'D2' by default for use of hardware interrupts */

//////////////////////////////////
//// Output Macro Definitions ////
SequenceBuild LedBuild[2] = {SequenceBuild(), SequenceBuild()}; // primary and secondary sequence build for flashing LED status

// give enough macros in the macro-pool for all led values:
LedMacro _Macro[10];
LedMacroManager Macro(_Macro, (sizeof(_Macro) / sizeof(_Macro[0])));

///////////////////////////
//// LED output Values ////
uint8_t MasterOnOff_val = 255; // master on/off brightness value for all LED's. Initializes 'on'

// primary Led Values:
uint8_t WhiteLed_val[2] = {0, 0};
uint8_t RedLed_val[2] = {0, 0};
uint8_t GreenLed_val[2] = {0, 0};
uint8_t BlueLed_val[2] = {0, 0};
uint8_t Brightness_val[2] = {255, 0}; // initializes with full brightness for primary value

/*
  led value select
   0: primary led values
   1: secondary led values
*/
uint8_t WhiteLed_valSelect = 0;
uint8_t RedLed_valSelect = 0;
uint8_t GreenLed_valSelect = 0;
uint8_t BlueLed_valSelect = 0;
uint8_t Brightness_valSelect = 0;

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
    >0: Button Actively Pressed

  ToDo: Store only necessary values relative to the buttons on the remote?
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

void setup()
{
  // debugging:
  Serial.begin(115200);
  Serial.println("Init");

  // init IR reciever:
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK /*(pin 13 will blink)*/);

  Serial.println("Init Default Color");
}

void loop()
{
  // run all handles:
  IR_handle();
  Logic_handle();
  StatusLed_handle();
}

void IR_handle()
{
  const uint16_t ir_buttonHoldTimeout = 120; // ms between '0' commands for button to timeout
  static uint32_t ir_buttonTimer = 0;

  // reset Ir command flag:
  IR_NewCommand = 0;

  // check if held button has timedout:
  if (IR_PressedButton != 0 && millis() - ir_buttonTimer > ir_buttonHoldTimeout)
    IR_PressedButton = 0; // set to "no button actively pressed"

  // check for new IR commands:
  if (IrReceiver.decode())
  {
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

    IrReceiver.resume();
  }
}

void Logic_handle()
{
  ///////////////////////////////
  ////// Macro Preparation //////
  static InputMacro ir_commandMacro(false); // this is only used for the timers, state change will be checked manually
  static uint32_t ir_lastPressedButton = 0; // use this because 'IR_LastPressedButton' will change prematurely when new ir command is received

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
    Use 'ir_lastPressedButton' for Off state changes
    Use 'IR_PressedButton' for On state changes
    Use 'ir_lastPressedButton' or 'IR_LastPressedButton' for 'off' intervals
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
  ////// Run all Macros and Commands //////

  ////// Master On/Off Commands //////
  {
    if (ir_commandMacroState_on)
    {
      switch (IR_PressedButton)
      {
      case IrCode_on:
        MasterOnOff_state = true;
        Serial.println("Master: ON");

        break;
      case IrCode_off:
        MasterOnOff_state = false;
        Serial.println("Master: OFF");

        //// check work states ////
        switch (WorkMode) // WorkMode states
        {
        case WorkMode_Static:
          // no need to do anything
          break;
        case WorkMode_MacroRun:
          // keep macro running in background
          break;
        case WorkMode_MacroStore:
          WorkMode = WorkMode_Static;
          Serial.print("\tMacroStore WorkMode: EXIT: ");
          switch (MacroMode)
          {
          case MacroMode_Flash:
            Serial.println("FLASH");
            break;
          case MacroMode_Strobe:
            Serial.println("STROBE");
            break;
          case MacroMode_Fade:
            Serial.println("FADE");
            break;
          case MacroMode_Smooth:
            Serial.println("SMOOTH");
            break;
          }
          Serial.println("\t\tStatic WorkMode: ENTER");
          break;
        }

        if (ColorEditState) // color edit state
        {
          ColorEditState = false;
          Serial.print("\tColor Edit: EXIT: ");
          switch (ColorEditMode)
          {
          case GetColor_red:
            Serial.println("RED");
            break;
          case GetColor_green:
            Serial.println("GREEN");
            break;
          case GetColor_blue:
            Serial.println("BLUE");
            break;
          case GetColor_white:
            Serial.println("WHITE");
            break;
          }
        }

        if (MacroSpeedAdjustState)
        {
          MacroSpeedAdjustState = false; // Exit Speed Adjust
          Serial.println("\tMacro Speed Adjust: EXIT");
        }
        break;
      }
    }
  }

  ////// Dimmer Commands //////
  {
    if (IR_NewCommand)
    {
      ////// Dimmer Controls //////
      switch (IR_PressedButton)
      {
      case IrCode_up:
        if (ColorEditState)
        {
          Serial.print("\tColor Dim: UP: ");
          switch (ColorEditMode)
          {
          case GetColor_red:
            Serial.println("RED");
            break;
          case GetColor_green:
            Serial.println("GREEN");
            break;
          case GetColor_blue:
            Serial.println("BLUE");
            break;
          case GetColor_white:
            Serial.println("WHITE");
            break;
          }
        }
        else
        {
          if (MacroSpeedAdjustState)
          {
            Serial.println("Macro Speed: UP");
          }
          else
          {
            Serial.println("Brightness Dim: UP");
          }
        }
        break;
      case IrCode_down:
        if (ColorEditState)
        {
          Serial.print("\tColor Dim: DOWN: ");
          switch (ColorEditMode)
          {
          case GetColor_red:
            Serial.println("RED");
            break;
          case GetColor_green:
            Serial.println("GREEN");
            break;
          case GetColor_blue:
            Serial.println("BLUE");
            break;
          case GetColor_white:
            Serial.println("WHITE");
            break;
          }
        }
        else
        {
          if (MacroSpeedAdjustState)
          {
            Serial.println("Macro Speed: DOWN");
          }
          else
          {
            Serial.println("Brightness Dim: DOWN");
          }
        }
        break;
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
          Serial.print("\tStatic Color CMD: ");
          Serial.println(checkIrColor(IR_PressedButton));
        }
        else
        {
          switch (WorkMode)
          {
          case WorkMode_Static:
            // do nothing, already in static mode
            Serial.print("Static Color CMD: ");
            Serial.println(checkIrColor(IR_PressedButton));
            break;
          case WorkMode_MacroRun:
            // exit macro run!
            Serial.print("Static Color CMD: ");
            Serial.println(checkIrColor(IR_PressedButton));
            WorkMode = WorkMode_Static;
            Serial.println("\tMacroRun WorkMode: EXIT");
            Serial.println("\t\tStatic WorkMode: ENTER");

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false; // Exit Speed Adjust
              Serial.println("\tMacro Speed Adjust: EXIT");
            }
            break;
          case WorkMode_MacroStore:
            // static colors are used to choose macro sequences
            Serial.print("\tStatic Color CMD: ");
            Serial.println(checkIrColor(IR_PressedButton));
            break;
          }
        }
        if (!MasterOnOff_state) // leds must turn on
        {
          MasterOnOff_state = true;
          Serial.println("\tMaster: ON");
        }
      }
    }
  }

  ////// Static White Color Commands //////
  {
    if (ir_commandMacroState_off) // check on release state change not to interfere with color edit mode
    {
      if (!ir_commandMacro.prevTriggered()) // make sure not previously triggered. When exiting color edit mode, macro is always triggered
      {
        if (!ColorEditState) // make sure not editing colors
        {
          switch (ir_lastPressedButton) // can't use 'IR_PressedButton' because button was just released
          {
          case IrCode_cont_white:
            switch (WorkMode)
            {
            case WorkMode_Static:
              // do nothing, already in static mode
              Serial.println("White Color Cycle CMD");
              break;
            case WorkMode_MacroRun:
              // exit macro run!
              Serial.println("White Color Cycle CMD");
              WorkMode = WorkMode_Static;
              Serial.println("\tMacroRun WorkMode: EXIT");
              Serial.println("\t\tStatic WorkMode: ENTER");

              if (MacroSpeedAdjustState)
              {
                MacroSpeedAdjustState = false; // Exit Speed Adjust
                Serial.println("\tMacro Speed Adjust: EXIT");
              }
              break;
            case WorkMode_MacroStore:
              // static colors are used to choose macro sequences
              Serial.println("\tWhite Color Cycle CMD");
              break;
            }
            if (!MasterOnOff_state) // leds must turn on
            {
              MasterOnOff_state = true;
              Serial.println("\tMaster: ON");
            }
            break;
          }
        }
      }
    }
  }

  ////// Color Edit Commands //////
  {
    const uint16_t colorEditHoldTime = 1000; // define function hold timer required to enter color edit mode
    const uint16_t colorEditTimeOut = 10000; // define how long after inactivity will color edit mode terminate

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

            Serial.print("Color Edit: ENTER: ");
            switch (ColorEditMode)
            {
            case GetColor_red:
              Serial.println("RED");
              break;
            case GetColor_green:
              Serial.println("GREEN");
              break;
            case GetColor_blue:
              Serial.println("BLUE");
              break;
            case GetColor_white:
              Serial.println("WHITE");
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
            Serial.print("Color Edit: Change Mode: ");
            switch (ColorEditMode)
            {
            case GetColor_red:
              Serial.println("RED");
              break;
            case GetColor_green:
              Serial.println("GREEN");
              break;
            case GetColor_blue:
              Serial.println("BLUE");
              break;
            case GetColor_white:
              Serial.println("WHITE");
              break;
            }
          }
          else
          {
            Serial.print("Color Edit: EXIT: ");
            switch (ColorEditMode)
            {
            case GetColor_red:
              Serial.println("RED");
              break;
            case GetColor_green:
              Serial.println("GREEN");
              break;
            case GetColor_blue:
              Serial.println("BLUE");
              break;
            case GetColor_white:
              Serial.println("WHITE");
              break;
            }
          }
          break;
        }
      }
    }

    // 'ColorEditState' Timeout
    if (ColorEditState)
    {
      if (!ir_commandMacro) // button is currently released
      {
        if (ir_commandMacro.interval() > colorEditTimeOut) // no need to trigger, 'ColorEditState' will take care of that
        {
          ColorEditState = false;

          Serial.print("TIMEOUT: Color Edit: EXIT: ");
          switch (ColorEditMode)
          {
          case GetColor_red:
            Serial.println("RED");
            break;
          case GetColor_green:
            Serial.println("GREEN");
            break;
          case GetColor_blue:
            Serial.println("BLUE");
            break;
          case GetColor_white:
            Serial.println("WHITE");
            break;
          }
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
            switch (ir_lastPressedButton) // can't use 'IR_PressedButton' because button was just released
            {
              // make sure one of the correct buttons was pressed:
            case IrCode_flash:
            case IrCode_strobe:
            case IrCode_fade:
            case IrCode_smooth:
              switch (ir_lastPressedButton) // can't use 'IR_PressedButton' because button was just released
              {
              case IrCode_flash:
                MacroMode = MacroMode_Flash;
                Serial.print("MacroRun WorkMode: ENTER: ");
                Serial.println("FLASH");
                Serial.println("\tStatic WorkMode: EXIT");
                break;
              case IrCode_strobe:
                MacroMode = MacroMode_Strobe;
                Serial.print("MacroRun WorkMode: ENTER: ");
                Serial.println("STROBE");
                Serial.println("\tStatic WorkMode: EXIT");
                break;
              case IrCode_fade:
                MacroMode = MacroMode_Fade;
                Serial.print("MacroRun WorkMode: ENTER: ");
                Serial.println("FADE");
                Serial.println("\tStatic WorkMode: EXIT");
                break;
              case IrCode_smooth:
                MacroMode = MacroMode_Smooth;
                Serial.print("MacroRun WorkMode: ENTER: ");
                Serial.println("SMOOTH");
                Serial.println("\tStatic WorkMode: EXIT");
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
            Serial.print("MacroRun WorkMode: CHANGE: ");
            Serial.println("FLASH");

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false;                // Exit Speed Adjust on Macro run change
              Serial.println("\tMacro Speed Adjust: EXIT"); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        case IrCode_strobe:
          if (MacroMode != MacroMode_Strobe) // check if already running this macro
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Strobe;
            Serial.print("MacroRun WorkMode: CHANGE: ");
            Serial.println("STROBE");

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false;                // Exit Speed Adjust on Macro run change
              Serial.println("\tMacro Speed Adjust: EXIT"); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        case IrCode_fade:
          if (MacroMode != MacroMode_Fade) // check if already running this macro
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Fade;
            Serial.print("MacroRun WorkMode: CHANGE: ");
            Serial.println("FADE");

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false;                // Exit Speed Adjust on Macro run change
              Serial.println("\tMacro Speed Adjust: EXIT"); // Speed adjust settings should be preserved between macro run changes
            }
          }
          break;
        case IrCode_smooth:
          if (MacroMode != MacroMode_Smooth) // check if already running this macro
          {
            ir_commandMacro.trigger();
            MacroMode = MacroMode_Smooth;
            Serial.print("MacroRun WorkMode: CHANGE: ");
            Serial.println("SMOOTH");

            if (MacroSpeedAdjustState)
            {
              MacroSpeedAdjustState = false;                // Exit Speed Adjust on Macro run change
              Serial.println("\tMacro Speed Adjust: EXIT"); // Speed adjust settings should be preserved between macro run changes
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

            // determine which button was pressed and set mode:
            switch (IR_PressedButton)
            {
            case IrCode_flash:
              MacroMode = MacroMode_Flash;
              break;
            case IrCode_strobe:
              MacroMode = MacroMode_Strobe;
              break;
            case IrCode_fade:
              MacroMode = MacroMode_Fade;
              break;
            case IrCode_smooth:
              MacroMode = MacroMode_Smooth;
              break;
            }

            Serial.print("MacroStore WorkMode: ENTER: ");
            switch (MacroMode)
            {
            case MacroMode_Flash:
              Serial.println("FLASH");
              break;
            case MacroMode_Strobe:
              Serial.println("STROBE");
              break;
            case MacroMode_Fade:
              Serial.println("FADE");
              break;
            case MacroMode_Smooth:
              Serial.println("SMOOTH");
              break;
            }
            Serial.println("\tStatic WorkMode: EXIT");
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

            switch (ir_lastPressedButton) // can't use 'IR_PressedButton' because button was just released
            {
            case IrCode_flash:
              if (MacroMode == MacroMode_Flash)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
              }
              break;
            case IrCode_strobe:
              if (MacroMode == MacroMode_Strobe)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
              }
              break;
            case IrCode_fade:
              if (MacroMode == MacroMode_Fade)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
              }
              break;
            case IrCode_smooth:
              if (MacroMode == MacroMode_Smooth)
              {
                // exit macroStore mode
                WorkMode = WorkMode_Static; // let the current color stay on
              }
              break;
            }

            if (WorkMode != WorkMode_MacroStore)
            {
              Serial.print("MacroStore WorkMode: EXIT: ");
              switch (MacroMode)
              {
              case MacroMode_Flash:
                Serial.println("FLASH");
                break;
              case MacroMode_Strobe:
                Serial.println("STROBE");
                break;
              case MacroMode_Fade:
                Serial.println("FADE");
                break;
              case MacroMode_Smooth:
                Serial.println("SMOOTH");
                break;
              }
              Serial.println("\tStatic WorkMode: ENTER");
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
              Serial.println("MacroStore WorkMode: Store Color: FLASH");
            }
            break;
          case IrCode_strobe:
            if (MacroMode == MacroMode_Strobe)
            {
              Serial.println("MacroStore WorkMode: Store Color: STROBE");
            }
            break;
          case IrCode_fade:
            if (MacroMode == MacroMode_Fade)
            {
              Serial.println("MacroStore WorkMode: Store Color: FADE");
            }
            break;
          case IrCode_smooth:
            if (MacroMode == MacroMode_Smooth)
            {
              Serial.println("MacroStore WorkMode: Store Color: SMOOTH");
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

          Serial.print("TIMEOUT: MacroStore WorkMode: EXIT: ");
          switch (MacroMode)
          {
          case MacroMode_Flash:
            Serial.println("FLASH");
            break;
          case MacroMode_Strobe:
            Serial.println("STROBE");
            break;
          case MacroMode_Fade:
            Serial.println("FADE");
            break;
          case MacroMode_Smooth:
            Serial.println("SMOOTH");
            break;
          }
          Serial.println("\tStatic WorkMode: ENTER");

          if (ColorEditState) // color edit state
          {
            ColorEditState = false;
            Serial.print("\tColor Edit: EXIT: ");
            switch (ColorEditMode)
            {
            case GetColor_red:
              Serial.println("RED");
              break;
            case GetColor_green:
              Serial.println("GREEN");
              break;
            case GetColor_blue:
              Serial.println("BLUE");
              break;
            case GetColor_white:
              Serial.println("WHITE");
              break;
            }
          }
        }
      }
      break;
    }
  }

  ////// Macro Speed Change Commands //////
  {
    const uint16_t macroSpeedTimeOut = 5000; // define how much inactivity before MacroStore WorkMode Exits

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
              Serial.print("\tMacro Speed Adjust: " + (String)(MacroSpeedAdjustState ? "ENTER" : "EXIT") + ": ");
              Serial.println("FLASH");
            }
            break;
          case IrCode_strobe:
            if (MacroMode == MacroMode_Strobe) // check if already running this macro
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state
              Serial.print("\tMacro Speed Adjust: " + (String)(MacroSpeedAdjustState ? "ENTER" : "EXIT") + ": ");
              Serial.println("STROBE");
            }
            break;
          case IrCode_fade:
            if (MacroMode == MacroMode_Fade) // check if already running this macro
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state
              Serial.print("\tMacro Speed Adjust: " + (String)(MacroSpeedAdjustState ? "ENTER" : "EXIT") + ": ");
              Serial.println("FADE");
            }
            break;
          case IrCode_smooth:
            if (MacroMode == MacroMode_Smooth) // check if already running this macro
            {
              MacroSpeedAdjustState = !MacroSpeedAdjustState; // toggle speed adjust state
              Serial.print("\tMacro Speed Adjust: " + (String)(MacroSpeedAdjustState ? "ENTER" : "EXIT") + ": ");
              Serial.println("SMOOTH");
            }
            break;
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
            Serial.println("TIMEOUT: Macro Speed Adjust: EXIT");
          }
        }
      }
    }
  }

  /* !!'ir_lastPressedButton' must be updated at the end of the function for expected functionally!! */
  if (ir_commandMacroState_on)
    ir_lastPressedButton = IR_PressedButton;

#undef ir_commandMacroState_null
#undef ir_commandMacroState_on
#undef ir_commandMacroState_off
#undef ir_commandMacroState_both
}

void StatusLed_handle()
{
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