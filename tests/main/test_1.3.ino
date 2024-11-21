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

//// generate function prototypes ////
void LED_handle();
void IR_handle();
void Logic_handle();
void StatusLed_handle();
uint8_t getIrColor(uint32_t, uint8_t);
uint8_t getIrColorByIndex(uint8_t, uint8_t);
int16_t checkIrColor(uint32_t);
bool runIrColor(uint32_t, uint16_t, uint8_t);
bool runColor(uint8_t, uint16_t, uint8_t);

uint8_t DimmerFlash(uint8_t, SequenceBuild *);
uint8_t ColorEditFlash(uint8_t, SequenceBuild *);

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
  'MasterOnOff_state':
    0: ('Brightness_val' == 0
    1: 'Brightness_val' == 255
    2: 255 > 'Brightness_val' > 0
*/
uint8_t MasterOnOff_state = 1;

#define WorkMode_Static 0
#define WorkMode_MacroRun 1
#define WorkMode_MacroStore 2

uint8_t WorkMode = WorkMode_Static;

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
  IR_LastButtonPressed:
    Similar to 'IR_PressedButton'
    retains pressed button value even after button is released
    Initializes with '0'
*/
uint32_t IR_LastButtonPressed = 0;

/*
  IR_NewCommand:
    0: no command
    1: new command(first time)
    2: hold command
   updates each time 'IR_handle()' is called
*/

uint8_t IR_NewCommand = 0;

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

void setup()
{
  // debugging:
  Serial.begin(115200);
  Serial.println("Brighness,Red,Green,Blue,White");

  // init IR reciever:
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK /*(pin 13 will blink)*/);

  Macro.fps(60); // redundant but for readability

  //// Init Led Pins ////
  pinMode(WhiteLed_Pin, OUTPUT);
  pinMode(RedLed_Pin, OUTPUT);
  pinMode(GreenLed_Pin, OUTPUT);
  pinMode(BlueLed_Pin, OUTPUT);

  // Development purposes, values are reversed:
  analogWrite(WhiteLed_Pin, 255);
  analogWrite(RedLed_Pin, 255);
  analogWrite(GreenLed_Pin, 255);
  analogWrite(BlueLed_Pin, 255);

  // run default led color:
  runColor(ColorCode_default, 120, 0);
}

void loop()
{
  // run macros and sequence builders
  Macro.run();
  LedBuild[0].run();
  LedBuild[1].run();

  // run all handles:
  LED_handle();
  IR_handle();
  Logic_handle();
  StatusLed_handle();
}

void LED_handle()
{
  uint8_t brightness_val_temp;
  uint8_t color_val_temp;

  // calculate master brightness:
  brightness_val_temp = map(Brightness_val[Brightness_valSelect], 0, 255, 0, MasterOnOff_val);

  // determine correct value for each led:
  // White:
  // analogWrite(WhiteLed_Pin, map(WhiteLed_val[WhiteLed_valSelect], 0, 255, 0, brightness_val_temp));
  analogWrite(WhiteLed_Pin, map(map(WhiteLed_val[WhiteLed_valSelect], 0, 255, 0, brightness_val_temp), 0, 255, 255, 0)); // development

  // Red:
  // analogWrite(RedLed_Pin, map(RedLed_val[RedLed_valSelect], 0, 255, 0, brightness_val_temp));
  analogWrite(RedLed_Pin, map(map(RedLed_val[RedLed_valSelect], 0, 255, 0, brightness_val_temp), 0, 255, 255, 0)); // development

  // Green:
  // analogWrite(GreenLed_Pin, map(GreenLed_val[GreenLed_valSelect], 0, 255, 0, brightness_val_temp));
  analogWrite(GreenLed_Pin, map(map(GreenLed_val[GreenLed_valSelect], 0, 255, 0, brightness_val_temp), 0, 255, 255, 0)); // development

  // Blue:
  // analogWrite(BlueLed_Pin, map(BlueLed_val[BlueLed_valSelect], 0, 255, 0, brightness_val_temp));
  analogWrite(BlueLed_Pin, map(map(BlueLed_val[BlueLed_valSelect], 0, 255, 0, brightness_val_temp), 0, 255, 255, 0)); // development
}

void IR_handle()
{
  const uint16_t ir_buttonHoldTimeout = 250; // ms between '0' commands for button to timeout
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
      IR_LastButtonPressed = IR_PressedButton;
    }

    // determine whether new command or hold command:
    if (!IrReceiver.decodedIRData.decodedRawData && IR_PressedButton) // hold command
      IR_NewCommand = 2;                                              // raise hold flag
    else                                                              // new command
      IR_NewCommand = 1;                                              // raise new command flag

    IrReceiver.resume();
  }
}

/*
  Logic_handle:
    handle IR Commands, each button functions, different LED modes.
    overall, the messiest but most vital part of the program
*/
void Logic_handle()
{
  /* check and handle direct ir commands first
     this should only be used by functions that use simple ir commands
     buttons with multiple possible functions should use the Input Macro below: */
  if (IR_NewCommand == 1) // new ir command
  {
    switch (IR_PressedButton)
    {
    /* on off commands are always accepted regardless of workmode: */
    case IrCode_on:
      // ToDo!
      break;
    case IrCode_off:
      // if (ColorEditState)
      //   ColorEditState = false; // ToDo: run other actions?
      // ToDo!
      break;
    default:
      // for static colors:
      switch (WorkMode)
      {
      case WorkMode_Static:
        if (checkIrColor(IR_PressedButton) != -1)
        {
          const uint16_t framesToFade = 30;
          // run static color:
          // LedBuild[0].reset();  // redundant
          runIrColor(IR_PressedButton, framesToFade, 0); // run new color

          // ToDo!:
          // if (MasterOnOff_state != 1)
          //   ; // make sure leds are on
        }
        break;
      case WorkMode_MacroRun: // buttons that are held down long enough will be stored into the macro sequence later:
        if (checkIrColor(IR_PressedButton) != -1)
        {
          const uint16_t framesToFade = 30;
          WorkMode = WorkMode_Static;                    // switch to static work mode
          LedBuild[0].reset();                           // make sure no macros are running anymore
          runIrColor(IR_PressedButton, framesToFade, 0); // run new color

          // ToDo!:
          // if (MasterOnOff_state != 1)
          //   ; // make sure leds are on
        }
        break;
      case WorkMode_MacroStore: // run colors that will be used to store to new macro
        if (checkIrColor(IR_PressedButton) != -1)
        {
          const uint16_t framesToFade = 15; // fade faster than usual
          // LedBuild[0].reset(); // shouldn't be necessary
          runIrColor(IR_PressedButton, framesToFade, 0); // run new color

          // ToDo? led's should never be off during macro edit:
          // if (MasterOnOff_state != 1)
          //   ; // make sure leds are on
        }
        break;
      }
      break;
    }
  }
  // else if (IR_NewCommand == 2) // repeat/hold ir command
  // {
  // }

  if (IR_NewCommand) // for actions that need both(new/hold commands) regardless
  {
    switch (IR_PressedButton)
    {
    //// dimer buttons for brightness and color edit ////
    case IrCode_up:
      if (ColorEditState)
      {
        // reset sequence to give room for user to edit colors:
        LedBuild[1].reset();
        LedBuild[1].setPrioritySequence(ColorEditFlash, 0);

        const uint8_t increment = 20; // increment amount, must be greater than 1

        uint8_t *ledEdit;
        if (ColorEditMode == GetColor_red)
          ledEdit = &RedLed_val[0];
        else if (ColorEditMode == GetColor_green)
          ledEdit = &GreenLed_val[0];
        else if (ColorEditMode == GetColor_blue)
          ledEdit = &BlueLed_val[0];
        else if (ColorEditMode == GetColor_white)
          ledEdit = &WhiteLed_val[0];

        // only run if led is less than max value
        if (*ledEdit < 255)
        {
          uint8_t target = *ledEdit;

          if (target > 255 - increment)
            target = 255;
          else
            target += increment;

          Macro.quadEase(*ledEdit, target, 5); // run very short(5 frames) because dim controls are coming constantly
        }
      }
      else
      {
        if (DimmerEnable)
        {
          const uint8_t increment = 20; // increment amount, must be greater than 1

          // only run if brightness is less than max value
          if (Brightness_val[0] < 255)
          {
            uint8_t target = Brightness_val[0];

            if (target > 255 - increment)
              target = 255;
            else
              target += increment;

            Macro.quadEase(Brightness_val[0], target, 5); // run very short(5 frames) because dim controls are coming constantly
          }
          else
          {
            DimmerEnable = false;
            LedBuild[1].setPrioritySequence(DimmerFlash, 0);
          }
        }
      }
      break;
    case IrCode_down:
      if (ColorEditState)
      {
        // reset sequence to give room for user to edit colors:
        LedBuild[1].reset();
        LedBuild[1].setPrioritySequence(ColorEditFlash, 0);

        const uint8_t increment = 20;     // increment amount, must be greater than 1
        const uint8_t minValueTotal = 50; // total combined minimume value of RGBW. Must never be '0'

        uint8_t *ledEdit;
        if (ColorEditMode == GetColor_red)
          ledEdit = &RedLed_val[0];
        else if (ColorEditMode == GetColor_green)
          ledEdit = &GreenLed_val[0];
        else if (ColorEditMode == GetColor_blue)
          ledEdit = &BlueLed_val[0];
        else if (ColorEditMode == GetColor_white)
          ledEdit = &WhiteLed_val[0];

        uint8_t minValue = 0;
        // calculate minValue:
        if (((RedLed_val[0] + GreenLed_val[0] + BlueLed_val[0] + WhiteLed_val[0]) - *ledEdit) < minValueTotal)
          minValue = minValueTotal - ((RedLed_val[0] + GreenLed_val[0] + BlueLed_val[0] + WhiteLed_val[0]) - *ledEdit);

        // only run if led is more than min value
        if (*ledEdit > minValue)
        {
          uint8_t target = *ledEdit;

          if (target < minValue + increment)
            target = minValue;
          else
            target -= increment;

          Macro.quadEase(*ledEdit, target, 5); // run very short(5 frames) because dim controls are coming constantly
        }
      }
      else
      {
        if (DimmerEnable)
        {
          const uint8_t increment = 20; // increment amount, must be greater than 1
          const uint8_t minValue = 20;  // set minimume brightness level

          // only run if brightness is less than max value
          if (Brightness_val[0] > minValue)
          {
            uint8_t target = Brightness_val[0];

            if (target < minValue + increment)
              target = minValue;
            else
              target -= increment;

            Macro.quadEase(Brightness_val[0], target, 5); // run very short(5 frames) because dim controls are coming constantly
          }
          else
          {
            DimmerEnable = false;
            LedBuild[1].setPrioritySequence(DimmerFlash, 0);
          }
        }
      }
      break;
    }
  }

  ///////////////////////////////
  //// Input Macro Functions ////

  static InputMacro ir_commandMacro(false); // this is only used for the timers, state change will be checked manually
  static uint32_t ir_lastButtonPressed = 0; // use this because 'IR_LastButtonPressed' will change prematurely for our purposes

  if (ir_commandMacro && (IR_PressedButton == 0 || IR_NewCommand == 1)) // check 'off' state change
  {
    ir_commandMacro(false);

    DimmerEnable = true; // reenable dimmer on button release

    //// white led cycle ////
    /* this has to be done on the lifting of the button so as to not interfere with other functions */
    if (!ColorEditState && WhiteCycleEnable)
    {
      switch (ir_lastButtonPressed) // can't use 'IR_PressedButton' because button was just released
      {
      case IrCode_cont_white:
        switch (WorkMode)
        {
        case WorkMode_Static:
        {
          const uint16_t framesToFade = 30;
          // run static color:
          // LedBuild[0].reset();  // redundant

          /* for now, white color aligns with 'ColorPalletteExtra'. In the future, a switch statement may be needed */
          runColor(WhiteColorCycle, framesToFade, 0); // run new white color

          // ToDo!:
          // if (MasterOnOff_state != 1)
        }
        break;
        case WorkMode_MacroRun:
        {
          const uint16_t framesToFade = 30;
          WorkMode = WorkMode_Static; // switch to static work mode
          LedBuild[0].reset();        // make sure no macros are running anymore

          /* for now, white color aligns with 'ColorPalletteExtra'. In the future, a switch statement may be needed */
          runColor(WhiteColorCycle, framesToFade, 0); // run new color

          // ToDo!:
          // if (MasterOnOff_state != 1)
          //   ; // make sure leds are on
        }
        break;
        case WorkMode_MacroStore:
        {
          // run colors that will be used to store to new macro
          const uint16_t framesToFade = 15; // fade faster than usual
          // LedBuild[0].reset(); // shouldn't be necessary
          runIrColor(WhiteColorCycle, framesToFade, 0); // run new color

          // ToDo? led's should never be off during macro edit:
          // if (MasterOnOff_state != 1)
          //   ; // make sure leds are on
        }
        break;
        }

        // increment 'WhiteColorCycle':
        WhiteColorCycle++;
        WhiteColorCycle %= 3;
        break;
      }
    }
    if (!ColorEditState)
      WhiteCycleEnable = true; // reenable cycle mode
  }
  if (!ir_commandMacro && IR_NewCommand == 1) // check 'on' state change
  {
    ir_commandMacro(true);
    ir_lastButtonPressed = IR_PressedButton;

    //// color edit mode buttons ////
    if (ColorEditState)
    {
      switch (IR_PressedButton)
      {
        // make sure only one of the color edit control buttons is pressed:
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
        ir_commandMacro.trigger();
        if (ColorEditState) // check if still in color edit state
        {
          LedBuild[1].reset();
          LedBuild[1].setPrioritySequence(ColorEditFlash, 1);
        }
        else
        {
          // do end sequence for edit mode:
          LedBuild[1].reset();
          LedBuild[1].setPrioritySequence(ColorEditFlash, 6);
        }
        break;
      }
    }
  }

  //// color edit mode buttons ////
  const uint16_t editColorHoldTime = 1000;
  switch (IR_PressedButton)
  {
  // make sure only one of the color edit control buttons is pressed:
  case IrCode_cont_red:
  case IrCode_cont_green:
  case IrCode_cont_blue:
  case IrCode_cont_white:
    switch (WorkMode)
    {
    // don't edit colors in macro run mode:
    case WorkMode_MacroStore:
    case WorkMode_Static:
      if (!ColorEditState && MasterOnOff_state == 1) // make sure master power is on and not currently in color edit state
      {
        if (ir_commandMacro && ir_commandMacro.interval() > editColorHoldTime && !ir_commandMacro.triggered())
        {

          ir_commandMacro.trigger();
          ColorEditState = true;
          LedBuild[1].reset();
          LedBuild[1].setPrioritySequence(ColorEditFlash, 1);
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
        }
      }
      break;
    }
    break;
  }
}

void StatusLed_handle()
{
  //// white color cycle ////
  if (IR_LastButtonPressed != IrCode_cont_white || !WhiteCycleEnable) // if white control button is/was pressed, let it handle color cycling
  {
    // if(MasterOnOff_state == 1) // ToDo? make sure colors dont change when led's are off. if 'IrCode_cont_white' is pressed, just go to current white mode, not next
    if (RedLed_val[0] == 255 && GreenLed_val[0] == 255 && BlueLed_val[0] == 255 && WhiteLed_val[0] == 255) // ColorCode_white
      WhiteColorCycle = 1;                                                                                 // change to ColorCode_warmWhite
    else if (RedLed_val[0] == 0 && GreenLed_val[0] == 0 && BlueLed_val[0] == 0 && WhiteLed_val[0] == 255)  // ColorCode_warmWhite
      WhiteColorCycle = 2;                                                                                 // change to ColorCode_coolWhite
    else
      WhiteColorCycle = 0; // set to default(ColorCode_white)
  }
  if (ColorEditState)
    WhiteCycleEnable = false; // disable white color cycle mode
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
bool runIrColor(uint32_t irCode, uint16_t framesToFade, uint8_t varMode)
{
  int16_t irIndex = checkIrColor(irCode);
  if (irIndex != -1)
  {
    // fade to new color:
    Macro.quadEase(RedLed_val[varMode], getIrColorByIndex(irIndex, GetColor_red), framesToFade);
    Macro.quadEase(GreenLed_val[varMode], getIrColorByIndex(irIndex, GetColor_green), framesToFade);
    Macro.quadEase(BlueLed_val[varMode], getIrColorByIndex(irIndex, GetColor_blue), framesToFade);
    Macro.quadEase(WhiteLed_val[varMode], getIrColorByIndex(irIndex, GetColor_white), framesToFade);
    return true;
  }
  return false;
}

bool runColor(uint8_t index, uint16_t framesToFade, uint8_t varMode)
{
  static const uint8_t palletteListSize = (sizeof(ColorPalletteExtra) / sizeof(ColorPalletteExtra[0]));
  if (index < palletteListSize)
  {
    // fade to new color:
    Macro.quadEase(RedLed_val[varMode], pgm_read_byte(&ColorPalletteExtra[index].rgbw.red), framesToFade);
    Macro.quadEase(GreenLed_val[varMode], pgm_read_byte(&ColorPalletteExtra[index].rgbw.green), framesToFade);
    Macro.quadEase(BlueLed_val[varMode], pgm_read_byte(&ColorPalletteExtra[index].rgbw.blue), framesToFade);
    Macro.quadEase(WhiteLed_val[varMode], pgm_read_byte(&ColorPalletteExtra[index].rgbw.white), framesToFade);
    return true;
  }
  return false;
}

/*
  DimmerFlash:
    pulse brightness shortly, cooldown after 500ms
*/
SB_FUNCT(DimmerFlash, Macro.ready(Brightness_val[1]))
SB_STEP(Brightness_valSelect = 1;
        Brightness_val[1] = Brightness_val[0];
        //         Macro.quadEase(Brightness_val[1], 0, 10);)
        // SB_STEP(Macro.delay(Brightness_val[1], 100);)
        // SB_STEP(Macro.quadEase(Brightness_val[1], Brightness_val[1], 10);)
        // SB_STEP(Brightness_valSelect = 0;
        //         Macro.delay(Brightness_val[1], 500);)
        Macro.set(Brightness_val[1], 1, 100);)
SB_STEP(Macro.set(Brightness_val[1], Brightness_val[0], 500);
        Brightness_valSelect = 0;)
SB_STEP(_this->reset();)
SB_END

/*
  ColorEditFlash
    determines which color is in edit mode and flash it
    once 'ColorEditState' has been disabled, this sequence will terminate automatically
*/
SB_FUNCT(ColorEditFlash, Macro.ready(Brightness_val[1]) &&
                             Macro.ready(RedLed_val[1]) &&
                             Macro.ready(GreenLed_val[1]) &&
                             Macro.ready(BlueLed_val[1]) &&
                             Macro.ready(WhiteLed_val[1]))
// index 0:
SB_STEP(
    Brightness_valSelect = 0;
    RedLed_valSelect = 0;
    GreenLed_valSelect = 0;
    BlueLed_valSelect = 0;
    WhiteLed_valSelect = 0;
    Macro.stop(Brightness_val[1]);
    Macro.delay(RedLed_val[1], 1000);
    Macro.delay(GreenLed_val[1], 1000);
    Macro.delay(BlueLed_val[1], 1000);
    Macro.delay(WhiteLed_val[1], 1000);)
// index 1:
SB_STEP(
    Brightness_valSelect = 0;
    RedLed_valSelect = 1;
    GreenLed_valSelect = 1;
    BlueLed_valSelect = 1;
    WhiteLed_valSelect = 1;
    const uint16_t setDelay = 200;
    Macro.stop(Brightness_val[1]);
    Macro.set(RedLed_val[1], (ColorEditMode == GetColor_red) ? 255 : 0, setDelay);
    Macro.set(GreenLed_val[1], (ColorEditMode == GetColor_green) ? 255 : 0, setDelay);
    Macro.set(BlueLed_val[1], (ColorEditMode == GetColor_blue) ? 255 : 0, setDelay);
    Macro.set(WhiteLed_val[1], (ColorEditMode == GetColor_white) ? 255 : 0, setDelay);)
// index 2:
SB_STEP(
    const uint16_t setDelay = 200;
    Macro.stop(Brightness_val[1]);
    Macro.set(RedLed_val[1], 0, setDelay);
    Macro.set(GreenLed_val[1], 0, setDelay);
    Macro.set(BlueLed_val[1], 0, setDelay);
    Macro.set(WhiteLed_val[1], 0, setDelay);)
// index 3:
SB_STEP(
    Brightness_valSelect = 0;
    RedLed_valSelect = 0;
    GreenLed_valSelect = 0;
    BlueLed_valSelect = 0;
    WhiteLed_valSelect = 0;
    const uint16_t setDelay = 800;
    Macro.stop(Brightness_val[1]);
    Macro.delay(RedLed_val[1], setDelay);
    Macro.delay(GreenLed_val[1], setDelay);
    Macro.delay(BlueLed_val[1], setDelay);
    Macro.delay(WhiteLed_val[1], setDelay);)
// index 4:
SB_STEP(if (ColorEditState) _this->loop(1);)
// index 5:
SB_STEP(
    Brightness_valSelect = 0;
    RedLed_valSelect = 0;
    GreenLed_valSelect = 0;
    BlueLed_valSelect = 0;
    WhiteLed_valSelect = 0;
    _this->reset();)
// begin from here to end edit mode with a double flash:
// index 6:
SB_STEP(
    Brightness_valSelect = 0;
    RedLed_valSelect = 1;
    GreenLed_valSelect = 1;
    BlueLed_valSelect = 1;
    WhiteLed_valSelect = 1;
    const uint16_t setDelay = 100;
    Macro.stop(Brightness_val[1]);
    Macro.set(RedLed_val[1], 0, setDelay);
    Macro.set(GreenLed_val[1], 0, setDelay);
    Macro.set(BlueLed_val[1], 0, setDelay);
    Macro.set(WhiteLed_val[1], 0, setDelay);)
// index 7:
SB_STEP(
    const uint16_t setDelay = 100;
    Macro.stop(Brightness_val[1]);
    Macro.set(RedLed_val[1], (ColorEditMode == GetColor_red) ? 255 : 0, setDelay);
    Macro.set(GreenLed_val[1], (ColorEditMode == GetColor_green) ? 255 : 0, setDelay);
    Macro.set(BlueLed_val[1], (ColorEditMode == GetColor_blue) ? 255 : 0, setDelay);
    Macro.set(WhiteLed_val[1], (ColorEditMode == GetColor_white) ? 255 : 0, setDelay);)
// index 8:
SB_STEP(
    const uint16_t setDelay = 100;
    Macro.stop(Brightness_val[1]);
    Macro.set(RedLed_val[1], 0, setDelay);
    Macro.set(GreenLed_val[1], 0, setDelay);
    Macro.set(BlueLed_val[1], 0, setDelay);
    Macro.set(WhiteLed_val[1], 0, setDelay);)
// index 9:
SB_STEP(
    const uint16_t setDelay = 100;
    Macro.stop(Brightness_val[1]);
    Macro.set(RedLed_val[1], (ColorEditMode == GetColor_red) ? 255 : 0, setDelay);
    Macro.set(GreenLed_val[1], (ColorEditMode == GetColor_green) ? 255 : 0, setDelay);
    Macro.set(BlueLed_val[1], (ColorEditMode == GetColor_blue) ? 255 : 0, setDelay);
    Macro.set(WhiteLed_val[1], (ColorEditMode == GetColor_white) ? 255 : 0, setDelay);)
// index 10:
SB_STEP(
    Brightness_valSelect = 0;
    RedLed_valSelect = 0;
    GreenLed_valSelect = 0;
    BlueLed_valSelect = 0;
    WhiteLed_valSelect = 0;
    _this->reset();)
// // begin from here to flash quickly indicating edge values, will resume at index 0:
// // index 11:
// SB_STEP(
//     Brightness_valSelect = 1;
//     RedLed_valSelect = 0;
//     GreenLed_valSelect = 0;
//     BlueLed_valSelect = 0;
//     WhiteLed_valSelect = 0;
//     Macro.quadEase(Brightness_val[1], 0, 10);
//     Macro.stop(RedLed_val[1]);
//     Macro.stop(GreenLed_val[1]);
//     Macro.stop(BlueLed_val[1]);
//     Macro.stop(WhiteLed_val[1]);)
// // index 12:
// SB_STEP(Macro.delay(Brightness_val[1], 100);)
// // index 13:
// SB_STEP(Macro.quadEase(Brightness_val[1], Brightness_val[1], 10);)
// // index 14:
// SB_STEP(_this->loop(0);)
SB_END

// /*
//   ColorEdgeFlash
//     Flash leds quickly and return to
// */