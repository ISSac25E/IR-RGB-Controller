/* 
  LinkedList_EEPROM_class:
    void reset(). resets entire EEPROM to 0.

    bool createLists(uint8_t). Set new lists or delete old lists. Lists cannot be swapped, yet. Existing lists will not be deleted if within index
    uint8_t listCount(). checks number of lists available
    uint16_t listSize(uint8_t). choose a specific list(index) and return size of list(bytes)
    uint16_t getList(uint8_t, byte[]). choose a list and return all contents into array. returns byte count
    bool setList(uint8_t, byte[], uint16_t). choose a list and set all bytes in it. Will overwrite all previous content. returns success
    bool appendList(uint8_t byte[], uint16_t). choose list and append bytes to it. returns success
    uint16_t getAvailable(). return minimume possible available bytes
    uint16_t getAvailable(uint8_t). return total available bytes for given list index

    bool/uint8_t check(). this should clean up pool if needed
        locate all lists and traverse. find empty memory spots or invalid memory locations
        this one should be run once in the beginning and could consume a lot of time
        steps:
          - validate list table. if invalid: break and flag
          - validate all lists and delete correctable issue. if uncorrectable: break and flag
          - locate NEXT x(maybe 8) addressed/sectors
          - run through ALL Lists(AND table)
            locate if any of the x addresses are invalid.
            if so, delete
          - repeat 2nd step until entire eeprom is checked
          - re-compact all sectors
          - return flag


  EEPROM Map:
    First Sector:
      [x] Number Of Bytes] [0] = invalid/empty Data Pool. odd number = invalid/unknown. Must be even for memory address byte pairs
      [x,x], [x,x]... byte pairs. list head addresses
      [x,x] final byte pair, Tail Address for continued link. [0,0] if end

    Data sector:
      [x] number of bytes [0] max = 255. 0 = empty/available byte, no sector
      [x,x,x...] data bytes. must be same amount of byte count in start
      [x,x] final byte pair. [0,0] if end
    
    // definitions?
*/
class LinkedList_EEPROM_class
{
  /*
    reset():
      reset entire EEPROM to '0'
      this should be done before first use
  */
  void reset();

  // void 
};
extern LinkedList_EEPROM_class LinkedList_EEPROM;