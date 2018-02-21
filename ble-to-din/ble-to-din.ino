#include <MIDI.h>
#include "nrf52.h"
#include <BLEPeripheral.h>

#define BLUE_STAT_PIN     7   // LED on pin 7
#define RED_STAT_PIN     11   // LED on pin 11
#define GREEN_STAT_PIN   12   // LED on pin 12
#define BTN_PIN	          6   // User button 

// create peripheral instance, see pinouts above
//const char * localName = "nRF52832 MIDI";
BLEPeripheral blePeripheral;
BLEService service("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");
BLECharacteristic characteristic("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLERead | BLEWriteWithoutResponse | BLENotify, 20 );
BLEDescriptor descriptor = BLEDescriptor("2902", 0);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, MIDI);

void setup() {
	delay(1000);
	
	//Setup diag leds
	pinMode(BLUE_STAT_PIN, OUTPUT);
	pinMode(RED_STAT_PIN, OUTPUT);
	pinMode(GREEN_STAT_PIN, OUTPUT);
	digitalWrite(BLUE_STAT_PIN, 1);
	digitalWrite(RED_STAT_PIN, 1);
	digitalWrite(GREEN_STAT_PIN, 1);
	
	//Setup nRF52832 user button
	pinMode(BTN_PIN, INPUT_PULLUP);

	setupBLE();

	// Initiate MIDI communications, listen to all channels
	MIDI.begin(MIDI_CHANNEL_OMNI);
	MIDI.turnThruOff();
	
	// The nRF52832 converts baud settings to the discrete standard rates.
	// Use the nrf52.h names to write a custom value, 0x7FFC80 after beginning midi
	NRF_UARTE_Type * myUart;
	myUart = (NRF_UARTE_Type *)NRF_UART0_BASE;
	myUart->BAUDRATE = 0x7FFC80;
	
	//Write data to the serial output pin to make sure the serial output is working.
	//Sometimes serial output only allows 1 byte out then hangs.  Resetting the
	//nRF52832 resolves the issue
	digitalWrite(RED_STAT_PIN, 0);
	MIDI.sendNoteOn(42, 66, 1);
	delay(500);
	MIDI.sendNoteOff(42, 66, 1); 
	digitalWrite(RED_STAT_PIN, 1);

}

void loop()
{
	BLECentral central = blePeripheral.central();
	if(digitalRead(BTN_PIN) == 0){
		digitalWrite(GREEN_STAT_PIN, 0);
		MIDI.sendNoteOff(0x45, 80, 1);
		delay(100);
		digitalWrite(GREEN_STAT_PIN, 1);
	}
	if (central) {
		//Prep the timestamp
		msOffset = millis();
		
		digitalWrite(BLUE_STAT_PIN, 0);
		// central connected to peripheral

		while (central.connected()) {
			digitalWrite(GREEN_STAT_PIN, 0);
			//If connected, send midi data by the button here
			if(digitalRead(BTN_PIN) == 0){
				digitalWrite(GREEN_STAT_PIN, 0);
				MIDI.sendNoteOn(0x45, 80, 1);
				delay(100);
				MIDI.sendNoteOff(0x45, 80, 1);
				digitalWrite(GREEN_STAT_PIN, 1);
			}
			//Check if data exists coming in from BLE
			if (characteristic.written()) {
				digitalWrite(RED_STAT_PIN, 0);
				processPacket();
				digitalWrite(RED_STAT_PIN, 1); 
			}
		}
	}
	//No longer connected.  Turn off the LEDs.
	digitalWrite(BLUE_STAT_PIN, 1);
	digitalWrite(GREEN_STAT_PIN, 1);
	//Delay to show off state for a bit
	delay(100);
}

//This function decodes the BLE characteristics and calls transmitMIDIonDIN
//if the packet contains sendable MIDI data.
void processPacket()
{
	//Receive the written packet and parse it out here.
	uint8_t * buffer = (uint8_t*)characteristic.value();
	uint8_t bufferSize = characteristic.valueLength();

	//Pointers used to search through payload.
	uint8_t lPtr = 0;
	uint8_t rPtr = 0;
	//lastStatus used to capture runningStatus 
	uint8_t lastStatus;
	//Decode first packet -- SHALL be "Full MIDI message"
	lPtr = 2; //Start at first MIDI status -- SHALL be "MIDI status"
	//While statement contains incrementing pointers and breaks when buffer size exceeded.
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
		//look at l and r pointers and decode by size.
		if( rPtr - lPtr < 1 ){
			//Time code or system
			transmitMIDIonDIN( lastStatus, 0, 0 );
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

//This function takes a midi packet as input and calls the appropriate library
//function to transmit the data.  It's a little redundant because the library
//reforms midi data from the calls and sends it out the serial port.
//
//Ideally, the MIDI BLE object would feed a MIDI library object as a serial
//object removing all of this code.
//
//A benefit of this redundant code is that it's easy to filter messages, and
//exposes how the library works.
void transmitMIDIonDIN( uint8_t status, uint8_t data1, uint8_t data2 )
{
	uint8_t channel = status & 0x0F;
	channel++;
	uint8_t command = (status & 0xF0) >> 4;
	switch(command)
	{
	case 0x08: //Note off
		MIDI.sendNoteOff(data1, data2, channel);
		break;
	case 0x09: //Note on
		MIDI.sendNoteOn(data1, data2, channel);
		break;
	case 0x0A: //Polyphonic Pressure
		MIDI.sendAfterTouch(data1, data2, channel);
		break;
	case 0x0B: //Control Change
		MIDI.sendControlChange(data1, data2, channel);
		break;
	case 0x0C: //Program Change
		MIDI.sendProgramChange(data1, channel);
		break;
	case 0x0D: //Channel Pressure
		MIDI.sendAfterTouch(data2, channel);
		break;
	case 0x0E: //Pitch Bend
		MIDI.send(midi::PitchBend, data1, data2, channel);
		break;
	case 0x0F: //System
		switch(status)
		{
			case 0xF1: //MTC Q frame
				MIDI.sendTimeCodeQuarterFrame( data1 );
				break;
			case 0xF2: //Song position
				MIDI.sendSongPosition(( (uint16_t)(data1 & 0x7F) << 7) | (data2 & 0x7F));
				break;
			case 0xF3: //Song select
				MIDI.sendSongSelect( data1 );
				break;
			case 0xF6: //Tune request
				MIDI.sendTuneRequest();
				break;
			case 0xF8: //Timing Clock
			case 0xFA: //Start
			case 0xFB: //Continue
			case 0xFC: //Stop
			case 0xFE: //Active Sensing
			case 0xFF: //Reset
				MIDI.sendRealTime( (midi::MidiType)status );
				break;
			default:
				break;
		}
		break;
	default:
		break;
	}	
}

void setupBLE()
{
	blePeripheral.setLocalName("BLE to DIN"); //local name sometimes used by central
	blePeripheral.setDeviceName("BLE to DIN"); //device name sometimes used by central
	//blePeripheral.setApperance(0x0000); //default is 0x0000, what should this be?
	blePeripheral.setAdvertisedServiceUuid(service.uuid()); //Advertise MIDI UUID

	// add attributes (services, characteristics, descriptors) to peripheral
	blePeripheral.addAttribute(service);
	blePeripheral.addAttribute(characteristic);
	blePeripheral.addAttribute(descriptor);

	// set initial value
	characteristic.setValue(0);

	blePeripheral.begin();
}
