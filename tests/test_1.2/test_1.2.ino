#include "Arduino.h"

#include "IR_config.h" // Define macros for input and output pin etc.
#include "IR_codes.h"
#include <IRremote.hpp>

#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\SequenceBuild\SequenceBuild_1.0.4.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\LedMacro\LedMacro_1.0.0.h"

#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\InputMacro\InputMacro_1.0.1.h"

#define WhiteLedPin 6
#define BlueLedPin 10
#define GreenLedPin 5
#define RedLedPin 9

SequenceBuild LedBuild;
SequenceBuild LedBuild2;

LedMacro _Macro[10];
LedMacroManager Macro(_Macro, 10);

// master brightness value:
uint8_t OnOffBrightnessVal = 255;

// these are the general values:
uint8_t WhiteLedVal = 0;
uint8_t RedLedVal = 0;
uint8_t GreenLedVal = 0;
uint8_t BlueLedVal = 0;
uint8_t BrightnessVal = 255;

// these secondary values will be used for flashing status
uint8_t WhiteLedVal2 = 0;
uint8_t RedLedVal2 = 0;
uint8_t GreenLedVal2 = 0;
uint8_t BlueLedVal2 = 0;
uint8_t BrightnessVal2 = 255;

// 0 = primary Led Macros, 1 = Secondary Led Macros:
uint8_t WhiteLedMacro = 0;
uint8_t RedLedMacro = 0;
uint8_t GreenLedMacro = 0;
uint8_t BlueLedMacro = 0;
uint8_t BrightnessMacro = 0;

const uint16_t defaultFade = 60; // In Frames. Default is 60fps

uint32_t IrPressedButton = 0; // 0 = no button pressed. Any other value is the button being actively held down. this assumes only one button can be held at a time

uint8_t WhiteMode = 0; // 0 = soft white(white + RGB), 1 = warm white(white), 2 = cool white(RGB)

#define LedMode_Main 0
#define LedMode_RedProg 1
#define LedMode_GreenProg 2
#define LedMode_BlueProg 3
#define LedMode_WhiteProg 4
#define LedMode_FadeProg 5
uint8_t LedMode = LedMode_Main;

bool DimEnable = true;

void setup()
{
  Serial.begin(115200);
  Serial.println("init");
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK /*(pin 13 will blink)*/);
  Macro.fps(60);

  LedBuild.reset();

  analogWrite(WhiteLedPin, 255);
  analogWrite(RedLedPin, 255);
  analogWrite(GreenLedPin, 255);
  analogWrite(BlueLedPin, 255);

  pinMode(WhiteLedPin, OUTPUT);
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);

  // default state:
  Macro.quadEase(WhiteLedVal, 255, defaultFade);
  Macro.quadEase(RedLedVal, 255, defaultFade);
  Macro.quadEase(GreenLedVal, 255, defaultFade);
  Macro.quadEase(BlueLedVal, 255, defaultFade);
}

void loop()
{
  // Serial.print(Macro.ready(BrightnessVal2));
  // Serial.print(", ");
  // Serial.print(LedBuild2.index());
  // Serial.print(", ");
  // Serial.println(Flash(255, &LedBuild2));
  IrHandle();

  Macro.run();
  LedBuild.run();
  LedBuild2.run();

  // Write to LEDs:
  uint8_t brightnessLevel;
  if (BrightnessMacro == 0)
    brightnessLevel = BrightnessVal;
  else
    brightnessLevel = BrightnessVal2;

  // set master brightness val:
  brightnessLevel = map(brightnessLevel, 0, 255, 0, OnOffBrightnessVal);

  if (WhiteLedMacro == 0)
    analogWrite(WhiteLedPin, map(map(WhiteLedVal, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));
  else
    analogWrite(WhiteLedPin, map(map(WhiteLedVal2, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));

  if (RedLedMacro == 0)
    analogWrite(RedLedPin, map(map(RedLedVal, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));
  else
    analogWrite(RedLedPin, map(map(RedLedVal2, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));
  if (GreenLedMacro == 0)
    analogWrite(GreenLedPin, map(map(GreenLedVal, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));
  else
    analogWrite(GreenLedPin, map(map(GreenLedVal2, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));
  if (BlueLedMacro == 0)
    analogWrite(BlueLedPin, map(map(BlueLedVal, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));
  else
    analogWrite(BlueLedPin, map(map(BlueLedVal2, 0, 255, 0, brightnessLevel), 0, 255, 255, 0));

  // analogWrite(WhiteLedPin, WhiteLedVal);
  // analogWrite(RedLedPin, RedLedVal);
  // analogWrite(GreenLedPin, map(GreenLedVal, 0, 255, 0, BrightnessVal));
  // analogWrite(BlueLedPin, map(BlueLedVal, 0, 255, 0, BrightnessVal));

  // if (IrReceiver.decode())
  // {
  //   IrReceiver.resume();
  //   runIrAction(IrReceiver.decodedIRData.decodedRawData);
  // }
}

/*
 handle incoming commands and current hold status for buttons
*/
inline void IrHandle()
{
  static uint16_t buttonHoldTimeout = 250;
  static uint32_t buttonHoldTimer = 0;

  static uint32_t lastPressedButton = 0;

  if (IrPressedButton != 0 && millis() - buttonHoldTimer > buttonHoldTimeout)
  {
    IrPressedButton = 0; // button timed-out
  }

  if (IrReceiver.decode())
  {
    buttonHoldTimer = millis(); // reset hold timer
    if (IrReceiver.decodedIRData.decodedRawData != 0)
    {
      IrPressedButton = IrReceiver.decodedIRData.decodedRawData;
      lastPressedButton = IrPressedButton;
    }

    // run commands:
    if (IrReceiver.decodedIRData.decodedRawData == 0 && IrPressedButton) // hold command
    {
      switch (IrPressedButton)
      {
      case IrCode_up:
      case IrCode_down: // not every button and function benefits from holding the button
        runIrAction(IrPressedButton);
        break;
      }
    }
    else // new command
    {
      runIrAction(IrReceiver.decodedIRData.decodedRawData);
    }
    IrReceiver.resume();
  }

  if (!DimEnable && IrPressedButton != IrCode_up && IrPressedButton != IrCode_down)
    DimEnable = true;

  if (lastPressedButton != IrCode_cont_white)
  {
    if (WhiteLedVal == 255 && RedLedVal == 255 && GreenLedVal == 255 && BlueLedVal == 255)
      WhiteMode = 1; // select a different white
    else if (WhiteLedVal == 255 && RedLedVal == 0 && GreenLedVal == 0 && BlueLedVal == 0)
      WhiteMode = 2; // select a different white
    else
      WhiteMode = 0;
  }

  static InputMacro redProgMacro(false);
  static InputMacro greenProgMacro(false);
  static InputMacro blueProgMacro(false);
  //
  //
  //  Red:
  if (redProgMacro(IrPressedButton == IrCode_cont_red)) // run macro and detect change
  {
    if (redProgMacro) // button pressed
    {
      if (LedMode == LedMode_RedProg)
      {
        Serial.println("Exit redProg Mode");
        LedMode = LedMode_Main;
        redProgMacro.trigger();
        LedBuild2.loop(5);
      }
      else if (LedMode == LedMode_BlueProg || LedMode == LedMode_GreenProg)
      {
        Serial.println("redProg Mode");
        redProgMacro.trigger();
        LedMode = LedMode_RedProg;
        LedBuild2.setPrioritySequence(RGB_ProgFlash, 1);
      }
    }
  }
  if (LedMode != LedMode_RedProg)
  {
    if (redProgMacro && redProgMacro.interval() > 1000 && !redProgMacro.triggered())
    {
      Serial.println("redProg Mode");
      redProgMacro.trigger();
      LedMode = LedMode_RedProg;
      LedBuild2.setPrioritySequence(RGB_ProgFlash, 1);
    }
  }
  //
  //
  //  Green:
  if (greenProgMacro(IrPressedButton == IrCode_cont_green)) // run macro and detect change
  {
    if (greenProgMacro) // button pressed
    {
      if (LedMode == LedMode_GreenProg)
      {
        Serial.println("Exit greenProg Mode");
        LedMode = LedMode_Main;
        greenProgMacro.trigger();
        LedBuild2.loop(5);
      }
      else if (LedMode == LedMode_BlueProg || LedMode == LedMode_RedProg)
      {
        Serial.println("greenProg Mode");
        greenProgMacro.trigger();
        LedMode = LedMode_GreenProg;
        LedBuild2.setPrioritySequence(RGB_ProgFlash, 1);
      }
    }
  }
  if (LedMode != LedMode_GreenProg)
  {
    if (greenProgMacro && greenProgMacro.interval() > 1000 && !greenProgMacro.triggered())
    {
      Serial.println("greenProg Mode");
      greenProgMacro.trigger();
      LedMode = LedMode_GreenProg;
      LedBuild2.setPrioritySequence(RGB_ProgFlash, 1);
    }
  }
  //
  //
  //  Blue:
  if (blueProgMacro(IrPressedButton == IrCode_cont_blue)) // run macro and detect change
  {
    if (blueProgMacro) // button pressed
    {
      if (LedMode == LedMode_BlueProg)
      {
        Serial.println("Exit blueProg Mode");
        LedMode = LedMode_Main;
        blueProgMacro.trigger();
        LedBuild2.loop(5);
      }
      else if (LedMode == LedMode_RedProg || LedMode == LedMode_GreenProg)
      {
        Serial.println("blueProg Mode");
        blueProgMacro.trigger();
        LedMode = LedMode_BlueProg;
        LedBuild2.setPrioritySequence(RGB_ProgFlash, 1);
      }
    }
  }
  if (LedMode != LedMode_BlueProg)
  {
    if (blueProgMacro && blueProgMacro.interval() > 1000 && !blueProgMacro.triggered())
    {
      Serial.println("blueProg Mode");
      blueProgMacro.trigger();
      LedMode = LedMode_BlueProg;
      LedBuild2.setPrioritySequence(RGB_ProgFlash, 1);
    }
  }
}

void runIrAction(uint32_t irCode)
{
  switch (irCode)
  {
  case IrCode_on:
    // on
    // LedBuild.stop();
    Macro.quadEase(OnOffBrightnessVal, 255, 120);
    break;
  case IrCode_off:
    // off
    Macro.quadEase(OnOffBrightnessVal, 0, 30);
    break;
  // case IrCode_reset:
  //   // reset
  //   LedBuild.stop();
  //   Macro.quadEase(BrightnessVal, 255, defaultFade);

  //   Macro.quadEase(WhiteLedVal, 255, defaultFade);
  //   Macro.quadEase(RedLedVal, 255, defaultFade);
  //   Macro.quadEase(GreenLedVal, 255, defaultFade);
  //   Macro.quadEase(BlueLedVal, 255, defaultFade);
  //   break;
  case IrCode_up:
    // Dim Up
    if (LedMode == LedMode_Main)
    {
      if (DimEnable)
      {
        if (BrightnessVal < 255)
        {
          const uint8_t inc = 30;
          uint8_t target = BrightnessVal;

          if (target > 255 - inc)
            target = 255;
          else
            target += inc;
          // BrightnessVal = target;
          Macro.quadEase(BrightnessVal, target, 5);
        }
        else
        {
          DimEnable = false;
          LedBuild2.setPrioritySequence(Flash, 0, true);
        }
      }
    }
    else if (LedMode == LedMode_RedProg || LedMode == LedMode_GreenProg || LedMode == LedMode_BlueProg)
    {
      uint8_t *ledProg = &BlueLedVal;
      if (LedMode == LedMode_RedProg)
        ledProg = &RedLedVal;
      if (LedMode == LedMode_GreenProg)
        ledProg = &GreenLedVal;

      LedBuild2.start(0, true);
      if (*ledProg < 255)
      {
        const uint8_t inc = 30;
        uint8_t target = *ledProg;

        if (target > 255 - inc)
          target = 255;
        else
          target += inc;
        // RedLedVal = target;
        Macro.quadEase(*ledProg, target, 5);
      }
    }
    break;
  case IrCode_down:
    // Dim Down
    if (LedMode == LedMode_Main)
    {
      const uint8_t minValue = 20;
      if (BrightnessVal > minValue)
      {
        const uint8_t inc = 30;
        uint8_t target = BrightnessVal;

        if (target < minValue + inc)
          target = minValue;
        else
          target -= inc;
        // BrightnessVal = target;
        Macro.quadEase(BrightnessVal, target, 5);
      }
    }
    else if (LedMode == LedMode_RedProg || LedMode == LedMode_GreenProg || LedMode == LedMode_BlueProg)
    {
      const uint8_t totalMinVal = 50;
      uint8_t minValue = 0;
      uint8_t *ledProg = &BlueLedVal;

      if (LedMode == LedMode_RedProg)
      {
        ledProg = &RedLedVal;
        if ((GreenLedVal + BlueLedVal + WhiteLedVal) < totalMinVal)
          minValue = totalMinVal - (GreenLedVal + BlueLedVal + WhiteLedVal);
      }
      else if (LedMode == LedMode_GreenProg)
      {
        ledProg = &GreenLedVal;
        if ((RedLedVal + BlueLedVal + WhiteLedVal) < totalMinVal)
          minValue = totalMinVal - (RedLedVal + BlueLedVal + WhiteLedVal);
      }
      else
      {
        if ((RedLedVal + GreenLedVal + WhiteLedVal) < totalMinVal)
          minValue = totalMinVal - (RedLedVal + GreenLedVal + WhiteLedVal);
      }

      LedBuild2.start(0, true);
      if (*ledProg > minValue)
      {
        const uint8_t inc = 30;
        uint8_t target = *ledProg;

        if (target < minValue + inc)
          target = minValue;
        else
          target -= inc;

        Macro.quadEase(*ledProg, target, 5);
      }
    }
    break;

  // case IrCode_coolwhite:
  //   // Cool White
  //   LedBuild.stop();
  //   Macro.quadEase(BrightnessVal, 255, defaultFade);

  //   Macro.quadEase(WhiteLedVal, 0, defaultFade);
  //   Macro.quadEase(RedLedVal, 255, defaultFade);
  //   Macro.quadEase(GreenLedVal, 255, defaultFade);
  //   Macro.quadEase(BlueLedVal, 255, defaultFade);
  //   break;
  case IrCode_cont_white:
    // Soft White
    LedBuild.reset();
    switch (WhiteMode)
    {
    case 0:
      // soft white(white + RGB)

      Macro.quadEase(WhiteLedVal, 255, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 1:
      // warm white(white)
      Macro.quadEase(WhiteLedVal, 255, defaultFade);
      Macro.quadEase(RedLedVal, 0, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 2:
      // cool white(RGB)
      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    }
    (++WhiteMode) %= 3;
    break;
    // case IrCode_warmwhite:
    //   // Warm White
    //   LedBuild.stop();
    //   Macro.quadEase(BrightnessVal, 255, defaultFade);

    //   Macro.quadEase(WhiteLedVal, 255, defaultFade);
    //   Macro.quadEase(RedLedVal, 0, defaultFade);
    //   Macro.quadEase(GreenLedVal, 0, defaultFade);
    //   Macro.quadEase(BlueLedVal, 0, defaultFade);
    //   break;

  case IrCode_red:
    // Red
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 255, defaultFade);
    Macro.quadEase(GreenLedVal, 0, defaultFade);
    Macro.quadEase(BlueLedVal, 0, defaultFade);
    break;
  case IrCode_lightred:
    // Fire
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 255, defaultFade);
    Macro.quadEase(GreenLedVal, 50, defaultFade);
    Macro.quadEase(BlueLedVal, 0, defaultFade);
    break;
  case IrCode_orange:
    // Orange
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 255, defaultFade);
    Macro.quadEase(GreenLedVal, 80, defaultFade);
    Macro.quadEase(BlueLedVal, 0, defaultFade);
    break;
  case IrCode_yellow:
    // Yelow
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 255, defaultFade);
    Macro.quadEase(GreenLedVal, 100, defaultFade);
    Macro.quadEase(BlueLedVal, 0, defaultFade);
    break;
  case IrCode_green:
    // Green
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 0, defaultFade);
    Macro.quadEase(GreenLedVal, 255, defaultFade);
    Macro.quadEase(BlueLedVal, 0, defaultFade);
    break;
  // case IrCode_lightgreen:
  //   // Light Green
  //   LedBuild.stop();
  //   Macro.quadEase(BrightnessVal, 255, defaultFade);

  //   Macro.quadEase(WhiteLedVal, 0, defaultFade);
  //   Macro.quadEase(RedLedVal, 100, defaultFade);
  //   Macro.quadEase(GreenLedVal, 255, defaultFade);
  //   Macro.quadEase(BlueLedVal, 0, defaultFade);
  //   break;
  case IrCode_cyan:
    // Cyan
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 0, defaultFade);
    Macro.quadEase(GreenLedVal, 255, defaultFade);
    Macro.quadEase(BlueLedVal, 255, defaultFade);
    break;
  case IrCode_lightblue:
    // Light Blue
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 0, defaultFade);
    Macro.quadEase(GreenLedVal, 100, defaultFade);
    Macro.quadEase(BlueLedVal, 255, defaultFade);
    break;

  case IrCode_blue:
    // Blue
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 0, defaultFade);
    Macro.quadEase(GreenLedVal, 0, defaultFade);
    Macro.quadEase(BlueLedVal, 255, defaultFade);
    break;
  case IrCode_purple:
    // Purple
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 100, defaultFade);
    Macro.quadEase(GreenLedVal, 0, defaultFade);
    Macro.quadEase(BlueLedVal, 255, defaultFade);
    break;
  case IrCode_lightpurple:
    // light Purple
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 150, defaultFade);
    Macro.quadEase(GreenLedVal, 0, defaultFade);
    Macro.quadEase(BlueLedVal, 255, defaultFade);
    break;
  case IrCode_pink:
    // Pink
    LedBuild.stop();
    Macro.quadEase(BrightnessVal, 255, defaultFade);

    Macro.quadEase(WhiteLedVal, 0, defaultFade);
    Macro.quadEase(RedLedVal, 255, defaultFade);
    Macro.quadEase(GreenLedVal, 0, defaultFade);
    Macro.quadEase(BlueLedVal, 255, defaultFade);
    break;
  case IrCode_flash:
    // Flash
    LedBuild.reset();
    LedBuild.setSequence(Smooth, 0, true);
    break;
  case IrCode_strobe:
    // Strobe
    LedBuild.reset();
    LedBuild.setSequence(Smooth, 0, true);
    break;
  case IrCode_fade:
    // Fade
    LedBuild.reset();
    LedBuild.setSequence(Smooth, 0, true);
    break;
  case IrCode_smooth:
    // Smooth
    LedBuild.reset();
    LedBuild.setSequence(Smooth, 0, true);
    break;
  default:
    break;
  }
}

bool allReady()
{
  return (Macro.ready(WhiteLedVal) &&
          Macro.ready(RedLedVal) &&
          Macro.ready(GreenLedVal) &&
          Macro.ready(BlueLedVal) &&
          Macro.ready(BrightnessVal) &&

          Macro.ready(WhiteLedVal2) &&
          Macro.ready(RedLedVal2) &&
          Macro.ready(GreenLedVal2) &&
          Macro.ready(BlueLedVal2) &&
          Macro.ready(BrightnessVal2))
             ? true
             : false;
}

SB_FUNCT(Init, Macro.ready(WhiteLedVal) && Macro.ready(RedLedVal) && Macro.ready(GreenLedVal) && Macro.ready(BlueLedVal))
SB_STEP(Macro.quadEase(WhiteLedVal, 255, defaultFade);
        Macro.quadEase(RedLedVal, 255, defaultFade);
        Macro.quadEase(GreenLedVal, 255, defaultFade);
        Macro.quadEase(BlueLedVal, 255, defaultFade);)
SB_END

const uint16_t fadeFrame = 240;
const uint16_t delayTime = 1000;

SB_FUNCT(Smooth, Macro.ready(WhiteLedVal) && Macro.ready(RedLedVal) && Macro.ready(GreenLedVal) && Macro.ready(BlueLedVal))
SB_STEP(Macro.quadEase(WhiteLedVal, 255, fadeFrame); // soft white
        Macro.quadEase(RedLedVal, 255, fadeFrame);
        Macro.quadEase(GreenLedVal, 255, fadeFrame);
        Macro.quadEase(BlueLedVal, 255, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // red
        Macro.quadEase(RedLedVal, 255, fadeFrame);
        Macro.quadEase(GreenLedVal, 0, fadeFrame);
        Macro.quadEase(BlueLedVal, 0, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // yellow
        Macro.quadEase(RedLedVal, 255, fadeFrame);
        Macro.quadEase(GreenLedVal, 100, fadeFrame);
        Macro.quadEase(BlueLedVal, 0, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 255, fadeFrame); // warm white
        Macro.quadEase(RedLedVal, 0, fadeFrame);
        Macro.quadEase(GreenLedVal, 0, fadeFrame);
        Macro.quadEase(BlueLedVal, 0, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // light blue
        Macro.quadEase(RedLedVal, 0, fadeFrame);
        Macro.quadEase(GreenLedVal, 100, fadeFrame);
        Macro.quadEase(BlueLedVal, 255, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // purple
        Macro.quadEase(RedLedVal, 100, fadeFrame);
        Macro.quadEase(GreenLedVal, 0, fadeFrame);
        Macro.quadEase(BlueLedVal, 255, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // cyan
        Macro.quadEase(RedLedVal, 0, fadeFrame);
        Macro.quadEase(GreenLedVal, 255, fadeFrame);
        Macro.quadEase(BlueLedVal, 255, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // orange
        Macro.quadEase(RedLedVal, 255, fadeFrame);
        Macro.quadEase(GreenLedVal, 80, fadeFrame);
        Macro.quadEase(BlueLedVal, 0, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, fadeFrame); // green
        Macro.quadEase(RedLedVal, 0, fadeFrame);
        Macro.quadEase(GreenLedVal, 255, fadeFrame);
        Macro.quadEase(BlueLedVal, 0, fadeFrame);)
SB_STEP(Macro.delay(BrightnessVal, delayTime);)
SB_STEP(_this->loop(0);)
SB_END

SB_FUNCT(Flash, Macro.ready(BrightnessVal2))
SB_STEP(BrightnessMacro = 1;
        Macro.set(BrightnessVal2, 0, 200);)
SB_STEP(Macro.set(BrightnessVal2, BrightnessVal, 500);
        BrightnessMacro = 0;)
SB_STEP(_this->reset();)
SB_END

SB_FUNCT(RGB_ProgFlash, Macro.ready(BrightnessVal2) && Macro.ready(RedLedVal2) && Macro.ready(GreenLedVal2) && Macro.ready(BlueLedVal2))
SB_STEP(BrightnessMacro = 0;
        WhiteLedMacro = 0;
        RedLedMacro = 0;
        GreenLedMacro = 0;
        BlueLedMacro = 0;
        Macro.delay(BrightnessVal2, 1000);
        Macro.delay(WhiteLedVal2, 1000);
        Macro.delay(RedLedVal2, 1000);
        Macro.delay(GreenLedVal2, 1000);
        Macro.delay(BlueLedVal2, 1000);)
// SB_STEP(BrightnessMacro = 1;
//         WhiteLedMacro = 1;
//         RedLedMacro = 1;
//         GreenLedMacro = 1;
//         BlueLedMacro = 1;
//         Macro.set(BrightnessVal2, 0, 200);
//         Macro.set(WhiteLedVal2, 0, 200);
//         Macro.set(RedLedVal2, 0, 200);
//         Macro.set(GreenLedVal2, 0, 200);
//         Macro.set(BlueLedVal2, 0, 200);)
SB_STEP(
    BrightnessMacro = 1;
    WhiteLedMacro = 1;
    RedLedMacro = 1;
    GreenLedMacro = 1;
    BlueLedMacro = 1;
    Macro.set(BrightnessVal2, BrightnessVal, 200);
    Macro.set(WhiteLedVal2, 0, 200);
    Macro.set(RedLedVal2, (LedMode == LedMode_RedProg) ? 255 : 0, 200);
    Macro.set(GreenLedVal2, (LedMode == LedMode_GreenProg) ? 255 : 0, 200);
    Macro.set(BlueLedVal2, (LedMode == LedMode_BlueProg) ? 255 : 0, 200);)
SB_STEP(
    Macro.set(BrightnessVal2, 0, 100);
    Macro.set(WhiteLedVal2, 0, 200);
    Macro.set(RedLedVal2, 0, 200);
    Macro.set(GreenLedVal2, 0, 200);
    Macro.set(BlueLedVal2, 0, 200);)
SB_STEP(BrightnessMacro = 0;
        WhiteLedMacro = 0;
        RedLedMacro = 0;
        GreenLedMacro = 0;
        BlueLedMacro = 0;
        Macro.delay(BrightnessVal2, 800);
        Macro.delay(WhiteLedVal2, 800);
        Macro.delay(RedLedVal2, 800);
        Macro.delay(GreenLedVal2, 800);
        Macro.delay(BlueLedVal2, 800);)
SB_STEP(_this->loop(1);)
SB_STEP(BrightnessMacro = 0; // abort and end sequence
        WhiteLedMacro = 0;
        RedLedMacro = 0;
        GreenLedMacro = 0;
        BlueLedMacro = 0;
        _this->reset();)
SB_END

// SB_FUNCT(stop, true)
// SB_STEP(Macro.stop(WhiteLedVal);
//         Macro.stop(RedLedVal);
//         Macro.stop(GreenLedVal);
//         Macro.stop(BlueLedVal);
//         Macro.stop(BrightnessVal);)
// SB_END