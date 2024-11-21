#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\FixedList_EEPROM\FixedList_EEPROM_1.0.0.h"

// carful when running this test too many times, the EEPROM is burned on each reset of the board
FixedList_EEPROM sequA(20, 100, 4); // a random start and stop address, 4-byte slots

void setup()
{
  Serial.begin(115200);
  Serial.println(sequA.beginStore());
  { byte b[] = {1, 2, 3, 4};
    Serial.println(sequA.storeNext(b));
  }
  { byte b[] = {11, 12, 13, 14};
    Serial.println(sequA.storeNext(b));
  }
  { byte b[] = {9, 8, 7, 6};
    Serial.println(sequA.storeNext(b));
  }

  Serial.println(sequA.endStore());
  Serial.println(sequA.beginRead());

  {
    byte b[5];
    sequA.readNext(b);
    printarr(b, 5);
  }

  {
    byte b[5];
    sequA.readNext(b);
    printarr(b, 5);
  }

  {
    byte b[5];
    sequA.readNext(b);
    printarr(b, 5);
  }

  // {
  //   byte b[5];
  //   sequA.read(1, b);
  //   printarr(b, 5);
  // }

  {
    byte b[5];
    sequA.readNext(b);
    printarr(b, 5);
  }
}

void loop()
{
}

void printarr(byte b[], uint8_t size)
{
  for (uint8_t x = 0; x < size; x++) {
    Serial.print(b[x]);
    Serial.print(',');
  }
  Serial.println();
}