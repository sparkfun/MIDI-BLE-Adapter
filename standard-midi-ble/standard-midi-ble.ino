#include <MIDI.h>
#include "nrf52.h"

// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiA);

uint8_t msgBuf[5] = {0x80, 0x80, 0x90, 0x40, 0x40};

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

void handleNoteOn(byte channel, byte pitch, byte velocity)
{
	msgBuf[2] = 0x09 & channel;
	msgBuf[3] = pitch;
	msgBuf[4] = velocity;
	characteristic.setValue(msgBuf, 5);
}

void handleNoteOff(byte channel, byte pitch, byte velocity)
{
	msgBuf[2] = 0x08 & channel;
	msgBuf[3] = pitch;
	msgBuf[4] = velocity;
	characteristic.setValue(msgBuf, 5);
}

#define LED_PIN    7 // LED on pin 7
#define BTN_PIN	   6

void setup() {
  pinMode(LED_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, 1);

  // begin initialization
  midiA.setHandleNoteOn(handleNoteOn);
  midiA.setHandleNoteOff(handleNoteOff);

  // Initiate MIDI communications, listen to all channels
  midiA.begin(MIDI_CHANNEL_OMNI);
  NRF_UARTE_Type * myUart;
  myUart = (NRF_UARTE_Type *)NRF_UART0_BASE;
  myUart->BAUDRATE = 0x7FFC80;
	
  setupBLE();
}

void loop() {
  BLECentral central = blePeripheral.central();

  if (central) {
    // central connected to peripheral
    while (central.connected()) {
		midiA.read();
		if(digitalRead(BTN_PIN) == 0){
			midiA.sendNoteOff(0x44, 80, 0);
			delay(100);
		}
      // central still connected to peripheral
      if (characteristic.written()) {
           uint8_t * buffer = (uint8_t*)characteristic.value();
           uint8_t channel = buffer[2] & 0x0F;
		   channel++;
           uint8_t command = (buffer[2] & 0xF0) >> 4;
           switch(command)
           {
             case 0x08:
               midiA.sendNoteOff(buffer[3], buffer[4], channel);
             break;
             case 0x09:
               midiA.sendNoteOn(buffer[3], buffer[4], channel);
             break;
             default:
             break;
           }
           digitalWrite(LED_PIN, 0);
           delay(100);
           digitalWrite(LED_PIN, 1); 
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
  //blePeripheral.addAttribute(descriptor);

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