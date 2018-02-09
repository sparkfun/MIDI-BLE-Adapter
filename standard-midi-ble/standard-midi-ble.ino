#include <MIDI.h>
#include "nrf52.h"
#include <BLEPeripheral.h>

#define LED_PIN    7 // LED on pin 7
#define RED_STAT_PIN    11 // LED on pin 7
#define GREEN_STAT_PIN    12 // LED on pin 7
#define BTN_PIN	   6

uint8_t msgBuf[5] = {0x80, 0x80, 0x90, 0x40, 0x40};

// create peripheral instance, see pinouts above
const char * localName = "nRF52832 MIDI2";
BLEPeripheral blePeripheral;
BLEService service("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");
BLECharacteristic characteristic("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLERead | BLEWriteWithoutResponse | BLENotify, 20 );
BLEDescriptor descriptor = BLEDescriptor("2901", "value");

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiA);

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

void setup() {
	delay(1000);
	pinMode(LED_PIN, OUTPUT);
	pinMode(RED_STAT_PIN, OUTPUT);
	pinMode(GREEN_STAT_PIN, OUTPUT);
	pinMode(BTN_PIN, INPUT_PULLUP);
	digitalWrite(LED_PIN, 1);
	digitalWrite(RED_STAT_PIN, 1);
	digitalWrite(GREEN_STAT_PIN, 1);

	setupBLE();
	
	// begin initialization
	midiA.setHandleNoteOn(handleNoteOn);
	midiA.setHandleNoteOff(handleNoteOff);

	// Initiate MIDI communications, listen to all channels
	midiA.begin(MIDI_CHANNEL_OMNI);
	
	NRF_UARTE_Type * myUart;
	myUart = (NRF_UARTE_Type *)NRF_UART0_BASE;
	myUart->BAUDRATE = 0x7FFC80;
	
	digitalWrite(RED_STAT_PIN, 0);
	midiA.sendNoteOn(42, 66, 1);
	delay(500);
	midiA.sendNoteOff(42, 66, 1); 
	digitalWrite(RED_STAT_PIN, 1);
	//midiA.turnThruOn();
	//midiA.turnThruOff();

}


void loop()
{
	BLECentral central = blePeripheral.central();
	if(digitalRead(BTN_PIN) == 0){
		digitalWrite(GREEN_STAT_PIN, 0);
		midiA.sendNoteOff(0x45, 80, 1);
		delay(100);
		digitalWrite(GREEN_STAT_PIN, 1);
	}
	if (central) {
		digitalWrite(LED_PIN, 0);
		// central connected to peripheral

		while (central.connected()) {
			digitalWrite(GREEN_STAT_PIN, 0);
			midiA.read();
			if(digitalRead(BTN_PIN) == 0){
				digitalWrite(GREEN_STAT_PIN, 1);
				midiA.sendNoteOff(0x44, 80, 1);
				delay(100);
			}
			// central still connected to peripheral
			if (characteristic.written()) {
				digitalWrite(RED_STAT_PIN, 0);
				processPacket();
				digitalWrite(RED_STAT_PIN, 1); 
			}
		}

	}
	digitalWrite(LED_PIN, 1);
	digitalWrite(GREEN_STAT_PIN, 1);
	delay(500);
}

void processPacket()
{
	uint8_t * buffer = (uint8_t*)characteristic.value();
	uint8_t bufferSize = characteristic.valueLength();
	uint8_t lPtr = 0;
	uint8_t rPtr = 0;
	uint8_t lastStatus;
	//Decode first packet -- SHALL be "Full MIDI message"
	lPtr = 2; //Start at first MIDI status -- SHALL be "MIDI status"
	while(1){
		lastStatus = buffer[lPtr];
		if( (buffer[lPtr] < 0x80) ){
			//Status message not present, bail
			return;
		}
		//Point to next non-data byte
		rPtr = lPtr;
		while( (buffer[rPtr + 1] < 0x80)&&(rPtr < (bufferSize - 1)) ){
			rPtr++;
		}
		//Serial.print("lPtr: ");
		//Serial.print(lPtr);
		//Serial.print(", rPtr: ");
		//Serial.println(rPtr);
		if( rPtr - lPtr < 1 ){
			//Time code or system
		} else if( rPtr - lPtr < 2 ) {
			transmitMIDIonDIN( lastStatus, buffer[lPtr + 1], 0 );
		} else if( rPtr - lPtr < 3 ) {
			transmitMIDIonDIN( lastStatus, buffer[lPtr + 1], buffer[lPtr + 2] );
		} else {
			//Too much data
			//If not System Common or System Real-Time, send it as running status
			switch( buffer[lPtr] & 0xF0 )
			{
				case 0x80:
				case 0x90:
				case 0xA0:
				case 0xB0:
				case 0xE0:
					for(int i = lPtr; i < rPtr; i = i + 2){
						transmitMIDIonDIN( lastStatus, buffer[i + 1], buffer[i + 2] );
					}
				break;
				case 0xC0:
				case 0xD0:
					for(int i = lPtr; i < rPtr; i = i + 1){
						transmitMIDIonDIN( lastStatus, buffer[i + 1], 0 );
					}
				break;
				default:
				break;
			}
		}
		//Point to next status
		lPtr = rPtr + 2;
		if(lPtr >= bufferSize){
			//end of packet
			return;
		}
	}
}

void transmitMIDIonDIN( uint8_t status, uint8_t data1, uint8_t data2 )
{
	uint8_t channel = status & 0x0F;
	channel++;
	uint8_t command = (status & 0xF0) >> 4;
	switch(command)
	{
	case 0x08: //Note off
		midiA.sendNoteOff(data1, data2, channel);
		break;
	case 0x09: //Note on
		midiA.sendNoteOn(data1, data2, channel);
		break;
	case 0x0A: //Polyphonic Pressure
		midiA.sendAfterTouch(data1, data2, channel);
		break;
	case 0x0B: //Control Change
		midiA.sendControlChange(data1, data2, channel);
		break;
	case 0x0C: //Program Change
		midiA.sendProgramChange(data1, channel);
		break;
	case 0x0D: //Channel Pressure
		midiA.sendAfterTouch(data2, channel);
		break;
	case 0x0E: //Pitch Bend
		midiA.send(midi::PitchBend, data1, data2, channel);
		break;
	case 0x0F: //System
		break;
	default:
		break;
	}	
}

void setupBLE()
{
	blePeripheral.setLocalName(localName); // optional
	blePeripheral.setAdvertisedServiceUuid(service.uuid()); // optional

	// add attributes (services, characteristics, descriptors) to peripheral
	blePeripheral.addAttribute(service);
	blePeripheral.addAttribute(characteristic);
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