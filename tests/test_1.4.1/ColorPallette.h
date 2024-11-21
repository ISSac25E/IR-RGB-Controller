#ifndef colorPallette_h
#define colorPallette_h

#include "IR_codes.h"

struct IrColorPallette_struct
{
  uint32_t irCode;

  union rgbw_union
  {
    uint8_t rgbw[4];
    struct
    {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
      uint8_t white;
    };
  };
  rgbw_union rgbw;
};

struct ColorPallette_struct
{
  union rgbw_union
  {
    uint8_t rgbw[4];
    struct
    {
      uint8_t red;
      uint8_t green;
      uint8_t blue;
      uint8_t white;
    };
  };
  rgbw_union rgbw;
};

/*
  {IR_color_code, {red, green, blue, white}}
*/
const IrColorPallette_struct IrColorPallette[] PROGMEM =
    {{IrCode_red,
      {255, 0, 0, 0}},

     {IrCode_lightRed,
      {255, 50, 0, 0}},

     {IrCode_orange,
      {255, 80, 0, 0}},

     {IrCode_yellow,
      {255, 127, 0, 0}},

     {IrCode_green,
      {0, 255, 0, 0}},

     {IrCode_cyan,
      {0, 255, 255, 0}},

     {IrCode_lightBlue1, // ToDo
      {0, 0, 0, 0}},

     {IrCode_lightBlue2, // ToDo
      {0, 0, 0, 0}},

     {IrCode_blue,
      {0, 0, 255, 0}},

     {IrCode_darkPurple, // ToDo
      {0, 0, 0, 0}},

     {IrCode_purple,
      {100, 0, 255, 0}},

     {IrCode_lightPurple, // ToDo
      {0, 0, 0, 0}}};

// define color code relative to their index:
#define ColorCode_default 0 // color to be run on startup

#define ColorCode_white 0
#define ColorCode_warmWhite 1
#define ColorCode_coolWhite 2

/*
  {red, green, blue, white}
*/
const ColorPallette_struct ColorPalletteExtra[] PROGMEM =
    {{255, 255, 255, 255}, // 0: white
     {0, 0, 0, 255},       // 1: warmWhite
     {255, 255, 255, 0}};  // 2: coolWhite

const ColorPallette_struct MacroMode_Flash_DefaultPallette[] PROGMEM =
    {{255, 80, 0, 0},      // orange
     {0, 255, 255, 0},     // cyan
     {0, 255, 0, 0},       // green
     {255, 255, 255, 255}, // white
     {255, 0, 0, 0},       // red
     {0, 0, 255, 0}};      // blue

const ColorPallette_struct MacroMode_Strobe_DefaultPallette[] PROGMEM =
    {{255, 80, 0, 0},      // orange
     {0, 255, 255, 0},     // cyan
     {0, 255, 0, 0},       // green
     {255, 255, 255, 255}, // white
     {255, 0, 0, 0},       // red
     {0, 0, 255, 0}};      // blue

const ColorPallette_struct MacroMode_Fade_DefaultPallette[] PROGMEM =
    {{255, 80, 0, 0},      // orange
     {0, 255, 255, 0},     // cyan
     {0, 255, 0, 0},       // green
     {255, 255, 255, 255}, // white
     {255, 0, 0, 0},       // red
     {0, 0, 255, 0}};      // blue

const ColorPallette_struct MacroMode_Smooth_DefaultPallette[] PROGMEM =
    {{255, 80, 0, 0},      // orange
     {0, 255, 255, 0},     // cyan
     {0, 255, 0, 0},       // green
     {255, 255, 255, 255}, // white
     {255, 0, 0, 0},       // red
     {0, 0, 255, 0}};      // blue

// extra useful definitions:
#define GetColor_red 0
#define GetColor_green 1
#define GetColor_blue 2
#define GetColor_white 3

#endif