#include "Arduino.h"

#include "IR_config.h" // Define macros for input and output pin etc.
#include <IRremote.hpp> // this library must be

#include "src/SequenceBuild/SequenceBuild_1.0.3.h"
#include "src/LedMacro/LedMacro_1.0.0.h"

SequenceBuild LedBuild;

LedMacro _Macro[5];
LedMacroManager Macro(_Macro, 5);

#define WhiteLedPin 6
#define BlueLedPin 10
#define GreenLedPin 5
#define RedLedPin 9

uint8_t WhiteLedVal = 0;
uint8_t RedLedVal = 0;
uint8_t GreenLedVal = 0;
uint8_t BlueLedVal = 0;
uint8_t BrightnessVal = 255;

const uint16_t defaultFade = 60;

void setup()
{
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  Macro.fps(60);

  // LedBuild.setSequence(stop, 0, true);
  // Macro.quadEase(BrightnessVal, 255, defaultFade);

  pinMode(WhiteLedPin, OUTPUT);
  pinMode(RedLedPin, OUTPUT);
  pinMode(GreenLedPin, OUTPUT);
  pinMode(BlueLedPin, OUTPUT);

  Macro.quadEase(WhiteLedVal, 255, defaultFade);
  Macro.quadEase(RedLedVal, 255, defaultFade);
  Macro.quadEase(GreenLedVal, 255, defaultFade);
  Macro.quadEase(BlueLedVal, 255, defaultFade);
}

void loop()
{
  Macro.run();
  LedBuild.run();

  analogWrite(WhiteLedPin, map(WhiteLedVal, 0, 255, 0, BrightnessVal));
  analogWrite(RedLedPin, map(RedLedVal, 0, 255, 0, BrightnessVal));
  analogWrite(GreenLedPin, map(GreenLedVal, 0, 255, 0, BrightnessVal));
  analogWrite(BlueLedPin, map(BlueLedVal, 0, 255, 0, BrightnessVal));

  // analogWrite(WhiteLedPin, WhiteLedVal);
  // analogWrite(RedLedPin, RedLedVal);
  // analogWrite(GreenLedPin, map(GreenLedVal, 0, 255, 0, BrightnessVal));
  // analogWrite(BlueLedPin, map(BlueLedVal, 0, 255, 0, BrightnessVal));

  if (IrReceiver.decode())
  {
    IrReceiver.resume();
    switch (IrReceiver.decodedIRData.decodedRawData)
    {
    case 4278251264:
      // on
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);
      break;
    case 4261539584:
      // off
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 0, defaultFade);
      break;
    case 4244827904:
      // reset
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 255, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 4228116224:
      // Dim Up
      if (BrightnessVal < 255)
      {
        uint8_t inc = 30;
        uint8_t target = BrightnessVal;

        if (target > 255 - inc)
          target = 255;
        else
          target += inc;
        Macro.quadEase(BrightnessVal, target, defaultFade);
      }
      else
      {
        LedBuild.setSequence(Flash, 0, true);
      }
      break;

    case 4211404544:
      // Cool White
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 4194692864:
      // Soft White
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 255, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 4177981184:
      // Warm White
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 255, defaultFade);
      Macro.quadEase(RedLedVal, 0, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4161269504:
      // Dim Down
      if (BrightnessVal > 30)
      {
        uint8_t inc = 30;
        uint8_t target = BrightnessVal;

        if (target < 30)
          target = 0;
        else
          target -= inc;
        Macro.quadEase(BrightnessVal, target, defaultFade);
      }
      break;

    case 4144557824:
      // Red
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4127846144:
      // Fire
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 50, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4111134464:
      // Orange
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 80, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4094422784:
      // Yelow
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 100, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4077711104:
      // Green
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 0, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4060999424:
      // Light Green
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 100, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 0, defaultFade);
      break;
    case 4044287744:
      // Cyan
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 0, defaultFade);
      Macro.quadEase(GreenLedVal, 255, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 4027576064:
      // Light Blue
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 0, defaultFade);
      Macro.quadEase(GreenLedVal, 100, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;

    case 4010864384:
      // Blue
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 0, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 3994152704:
      // Purple
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 100, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 3977441024:
      // light Purple
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 150, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
      break;
    case 3960729344:
      // Pink
      LedBuild.setSequence(stop, 0, true);
      Macro.quadEase(BrightnessVal, 255, defaultFade);

      Macro.quadEase(WhiteLedVal, 0, defaultFade);
      Macro.quadEase(RedLedVal, 255, defaultFade);
      Macro.quadEase(GreenLedVal, 0, defaultFade);
      Macro.quadEase(BlueLedVal, 255, defaultFade);
    case 3944017664:
      // Flash
      LedBuild.setSequence(stop, 0, true);
      LedBuild.setSequence(Smooth, 0, true);
      break;
    case 3927305984:
      // Strobe
      LedBuild.setSequence(stop, 0, true);
      LedBuild.setSequence(Smooth, 0, true);
      break;
    case 3910594304:
      // Fade
      LedBuild.setSequence(stop, 0, true);
      LedBuild.setSequence(Smooth, 0, true);
      break;
    case 3328630234:  // Correct Code: 3893882624
      // Smooth
      LedBuild.setSequence(stop, 0, true);
      LedBuild.setSequence(Smooth, 0, true);
      break;
    default:
      break;
    }
  }
}

bool allReady()
{
  return (Macro.ready(WhiteLedVal) &&
          Macro.ready(RedLedVal) &&
          Macro.ready(GreenLedVal) &&
          Macro.ready(BlueLedVal) &&
          Macro.ready(BrightnessVal))
             ? true
             : false;
}

SB_FUNCT(Init, allReady())
SB_STEP(Macro.quadEase(WhiteLedVal, 255, defaultFade);
        Macro.quadEase(RedLedVal, 255, defaultFade);
        Macro.quadEase(GreenLedVal, 255, defaultFade);
        Macro.quadEase(BlueLedVal, 255, defaultFade);)
SB_END

const uint16_t fadeFrame = 240;
const uint16_t delayTime = 1000;

SB_FUNCT(Smooth, allReady())
SB_STEP(Macro.quadEase(BrightnessVal, 255, 30);)
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
SB_STEP(LedBuild.loop(1);)
SB_END

SB_FUNCT(Flash, allReady())
SB_STEP(Macro.set(BrightnessVal, 0, 200);)
SB_STEP(Macro.set(BrightnessVal, 255, 200);)
SB_STEP(Macro.set(BrightnessVal, 0, 200);)
SB_STEP(Macro.set(BrightnessVal, 255, 0);
        LedBuild.setSequence(stop, 0, true);)
SB_END

SB_FUNCT(stop, true)
SB_STEP(Macro.stop(WhiteLedVal);
        Macro.stop(RedLedVal);
        Macro.stop(GreenLedVal);
        Macro.stop(BlueLedVal);
        Macro.stop(BrightnessVal);)
SB_END

SB_FUNCT(Demo, allReady())
SB_STEP(Macro.quadEase(WhiteLedVal, 255, 60);
        Macro.quadEase(RedLedVal, 0, 60);
        Macro.quadEase(GreenLedVal, 0, 60);
        Macro.quadEase(BlueLedVal, 0, 60);)
SB_STEP(Macro.delay(WhiteLedVal, 100);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, 60);
        Macro.quadEase(RedLedVal, 255, 60);
        Macro.quadEase(GreenLedVal, 0, 60);
        Macro.quadEase(BlueLedVal, 0, 60);)
SB_STEP(Macro.delay(RedLedVal, 100);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, 60);
        Macro.quadEase(RedLedVal, 0, 60);
        Macro.quadEase(GreenLedVal, 255, 60);
        Macro.quadEase(BlueLedVal, 0, 60);)
SB_STEP(Macro.delay(GreenLedVal, 100);)
SB_STEP(Macro.quadEase(WhiteLedVal, 0, 60);
        Macro.quadEase(RedLedVal, 0, 60);
        Macro.quadEase(GreenLedVal, 0, 60);
        Macro.quadEase(BlueLedVal, 255, 60);)
SB_STEP(Macro.delay(BlueLedVal, 100);)
SB_STEP(Macro.quadEase(WhiteLedVal, 255, 60);
        Macro.quadEase(RedLedVal, 255, 60);
        Macro.quadEase(GreenLedVal, 255, 60);
        Macro.quadEase(BlueLedVal, 255, 60);)
SB_STEP(Macro.delay(BlueLedVal, 100);)
SB_STEP(LedBuild.loop(0);)
SB_END