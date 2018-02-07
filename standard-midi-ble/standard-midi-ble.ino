// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheral depends on SPI)
#include <BLEPeripheral.h>

// create peripheral instance, see pinouts above
const char * localName = "nRF52832 MIDI";
BLEPeripheral blePeripheral;

// create one or more services
BLEService service("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");

// create one or more characteristics
BLECharacteristic characteristic("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLERead | BLEWriteWithoutResponse | BLENotify, 5 );

// create one or more descriptors (optional)
//BLEDescriptor descriptor = BLEDescriptor("2901", "value");

void setup() {
  // begin initialization
  setupBLE();
}

void loop() {
  BLECentral central = blePeripheral.central();

  if (central) {
    // central connected to peripheral
    while (central.connected()) {
      // central still connected to peripheral
      if (characteristic.written()) {
      }
    }

  }
}

void setupBLE()
{
  blePeripheral.setLocalName(localName); // optional
  blePeripheral.setAdvertisedServiceUuid(service.uuid()); // optional

  // add attributes (services, characteristics, descriptors) to peripheral
  blePeripheral.addAttribute(service);
  blePeripheral.addAttribute(characteristic);
  blePeripheral.addAttribute(characteristic7);
  blePeripheral.addAttribute(characteristic9);
  blePeripheral.addAttribute(descriptor);

  // set initial value
  characteristic.setValue(0);

  // set event handlers
  characteristic.setEventHandler(BLEWritten, BLEWrittenCallback);
  characteristic.setEventHandler(BLESubscribed, BLESubscribedCallback);
  characteristic.setEventHandler(BLEUnsubscribed, BLEUnsubscribedCallback);
  
  blePeripheral.begin();
}

// callback signature
void BLEWrittenCallback(BLECentral& central, BLECharacteristic& characteristic) {
  // ....
}

// callback signature
void BLESubscribedCallback(BLECentral& central, BLECharacteristic& characteristic) {
  // ....
}

// callback signature
void BLEUnsubscribedCallback(BLECentral& central, BLECharacteristic& characteristic) {
  // ....
}
//    event - BLEWritten, BLESubscribed, or BLEUnsubscribed