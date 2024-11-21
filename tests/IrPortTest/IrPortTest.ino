#include "IR_config.h"
#include "C:\Users\AVG\Documents\Electrical Main (Amir)\Arduino\Projects\Other\IR RGB Controller\Core\IRremote\src\IRremote.hpp"

void setup()
{
  Serial.begin(115200);
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

  // Start the receiver and if not 3. parameter specified, take LED_BUILTIN pin from the internal boards definition as default feedback LED
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signals of protocols: "));
  printActiveIRProtocols(&Serial);
  Serial.println(F("at pin " STR(IR_RECEIVE_PIN)));
}

uint32_t prevMsgTimer = 0;

void loop()
{
  if (IrReceiver.decode())
  {
    Serial.print("Command: ");
    if (IrReceiver.decodedIRData.decodedRawData == 0)
    {
      Serial.print(IrReceiver.decodedIRData.decodedRawData);
      Serial.print("\tt: ");
      Serial.println(millis() - prevMsgTimer);
    }
    else
      Serial.println(IrReceiver.decodedIRData.decodedRawData);
    prevMsgTimer = millis();
    IrReceiver.resume();
  }
}