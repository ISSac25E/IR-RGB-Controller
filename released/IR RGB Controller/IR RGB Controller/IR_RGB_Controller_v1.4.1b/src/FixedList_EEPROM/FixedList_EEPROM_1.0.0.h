// FixedList_EEPROM
//.h
/*
  Warning!
    no extra checks have been implemented
    This is a quick library wrote up that is meant to fill a requirement quickly and just work!
    It is the responsibility of the user to implement it safely and correctly into their program

    There may be Bugs!! Preliminary testing showed promising results 
*/
#ifndef FixedList_EEPROM_h
#define FixedList_EEPROM_h

#include "Arduino.h"
#include "EEPROM.h"

#define FixedList_EEPROM_EEPROM_Size 0 // define size, otherwise default will be calculated

#if FixedList_EEPROM_EEPROM_Size <= 0
#undef FixedList_EEPROM_EEPROM_Size
#define FixedList_EEPROM_EEPROM_Size EEPROM.length()
#endif

class FixedList_EEPROM
{
public:
  /*
    FixedList_EEPROM()
      initiate class

      input: EEPROM start Address(actual), EEPROM End Address(actual), slot size (bytes) MUST BE GREATER THAN 0
  */
  FixedList_EEPROM(uint16_t, uint16_t, uint8_t);

  /* Storing Section: */

  /*
    beginStore()
      resets necessary indexes. If endStore() is called now, no slots will be stored.
      It will be as an empty sequence
      If called again without endStore(), will restart storing sequence

      endStore() must be called at this point!

      output: slots available to store
  */
  uint8_t beginStore(void);

  /*
    storeNext()
      store in next memory slot, increment total slot count.
      Will only work if beginStore() was called, return slots available
      If too many slots are being stored, will be ignored.

      input: data byte[] (must be the same size as defined slot size)
      output: slots available to store(after this one)
  */
  uint8_t storeNext(byte *);

  /*
    endStore()
      Finalize storing, calculate checkSum and total slots and store
      If beginStore() was never called, will be ignored

      output: slots stored
  */
  uint8_t endStore();

  /* Reading Section: */

  /*
    beginRead()
      Checks CheckSum and wether any slots are available
      Returns number of slots available to read
      If checksum fails or no slots available, a '0' will be returned
      resets index for reading in a sequence

      output: number of slots available to read
  */
  uint8_t beginRead();

  /*
    readNext()
      will read next slot available, if any. If end is reached, it will loop back
      Data can be returned as a byte array[]
      Will return NaN '0' in byte array[] if no slots available for reading

      input: byte array[]
  */
  void readNext(byte *);

  /*
    read()
      Input a specific index and read from it. It will set global reading index too
      If index is invalid, default values will be returned '0'. Global read index will not be altered

      input: slot index, data byte array[]
  */
  void read(uint8_t, byte *);

private:
  /* __slotSize & __slotLimit & __EEPROM_Start are not to be changed after initialization */
  uint8_t __slotSize;
  uint8_t __slotLimit;
  uint16_t __EEPROM_Start;

  bool _storeMode = false;
  uint8_t _storeIndex;

  bool _readMode = false;
  uint8_t _readSlotCount;
  uint8_t _readIndex;
};

//.cpp

FixedList_EEPROM::FixedList_EEPROM(uint16_t EEPROM_Start, uint16_t EEPROM_End, uint8_t SlotSize)
{
  uint16_t totalBytesAvailable = 0;
  totalBytesAvailable = EEPROM_End - EEPROM_Start; // no extra check implemented, just make sure end is greater than start!

  totalBytesAvailable -= 3; // first three bytes reserved for checksum and slot count
  if (!SlotSize)            // last thing we want is div by 0 crash
    SlotSize = 1;

  // set vars:
  __slotSize = SlotSize;
  __slotLimit = totalBytesAvailable / SlotSize; // calculate number of slots we can fit! Round down!
  __EEPROM_Start = EEPROM_Start;
}

/* Storing Section: */

uint8_t FixedList_EEPROM::beginStore(void)
{
  _storeMode = true;
  _readMode = false;
  _storeIndex = 0;

  return __slotLimit; // let user know total number of slots allowed
}

uint8_t FixedList_EEPROM::storeNext(byte *data)
{
  if (_storeMode)
  {
    if (_storeIndex < __slotLimit)
    {
      for (uint8_t x = 0; x < __slotSize; x++)
        EEPROM.write(__EEPROM_Start + 3 + (_storeIndex * __slotSize) + x, data[x]);

      _storeIndex++;
      return __slotLimit - _storeIndex; // return available slots left
    }
  }
  return 0;
}

uint8_t FixedList_EEPROM::endStore()
{
  if (_storeMode)
  {
    EEPROM.write(__EEPROM_Start + 2, _storeIndex); // store number of slots stored
    uint16_t checkSum = 85 | (85 << 8);            // default control value
    checkSum += EEPROM.read(__EEPROM_Start + 2);

    for (uint16_t x = 0; x < ((uint16_t)_storeIndex * (uint16_t)__slotSize); x++)
      checkSum += EEPROM.read(__EEPROM_Start + 3 + x); // add each value to checksum

    EEPROM.write(__EEPROM_Start + 0, checkSum);      // store lower bound Checksum
    EEPROM.write(__EEPROM_Start + 1, checkSum >> 8); // store upper bound Checksum

    _storeMode = false;
    return _storeIndex;
  }
  return 0;
}

/* Reading Section: */

uint8_t FixedList_EEPROM::beginRead()
{
  if (!_storeMode) // we cant read if we are currently storing
  {
    // verify checksum:
    uint16_t checkSum = 85 | (85 << 8); // default control value
    uint8_t readSlotCount = EEPROM.read(__EEPROM_Start + 2);
    checkSum += readSlotCount;

    for (uint16_t x = 0; x < ((uint16_t)readSlotCount * (uint16_t)__slotSize); x++)
      checkSum += EEPROM.read(__EEPROM_Start + 3 + x); // add each value to checksum

    uint16_t storedCheckSum = (uint16_t)EEPROM.read(__EEPROM_Start + 0) | ((uint16_t)EEPROM.read(__EEPROM_Start + 1) << 8);

    if (checkSum == storedCheckSum && readSlotCount) // verified and slots available
    {
      _readMode = true;
      _readSlotCount = readSlotCount;
      _readIndex = 0; // reset

      return _readSlotCount;
    }
    else
    {
      _readMode = false;
    }
  }
  return 0;
}

void FixedList_EEPROM::readNext(byte *dataReturn)
{
  if (_readMode)
  {
    for (uint8_t x = 0; x < __slotSize; x++)
      dataReturn[x] = EEPROM.read(__EEPROM_Start + 3 + (_readIndex * __slotSize) + x);

    _readIndex++;
    if (_readIndex >= _readSlotCount)
      _readIndex = 0;
  }
  else
  {
    for (uint8_t x = 0; x < __slotSize; x++)
      dataReturn[x] = 0;
  }
}

void FixedList_EEPROM::read(uint8_t index, byte *dataReturn)
{
  if (_readMode)
  {
    if (index < _readSlotCount)
    {
      _readIndex = index;
      readNext(dataReturn); // reuse, simple
    }
    else
    {
      for (uint8_t x = 0; x < __slotSize; x++)
        dataReturn[x] = 0;
    }
  }
  else
  {
    for (uint8_t x = 0; x < __slotSize; x++)
      dataReturn[x] = 0;
  }
}

#undef FixedList_EEPROM_EEPROM_Size
#endif
