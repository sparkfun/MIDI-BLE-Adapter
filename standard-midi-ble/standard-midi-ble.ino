#include <MIDI.h>
#include "nrf52.h"
#include <BLEPeripheral.h>

#define LED_PIN    7 // LED on pin 7
#define RED_STAT_PIN    11 // LED on pin 7
#define GREEN_STAT_PIN    12 // LED on pin 7
#define BTN_PIN	   6

uint8_t msgBuf[5] = {0x80, 0x80, 0x90, 0x40, 0x40};

unsigned long msOffset = 0;
#define MAX_MS 0x01FFF //13 bits, 8192 dec

// create peripheral instance, see pinouts above
const char * localName = "nRF52832 MIDI";
BLEPeripheral blePeripheral;
BLEService service("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");
BLECharacteristic characteristic("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLERead | BLEWriteWithoutResponse | BLENotify, 20 );
BLEDescriptor descriptor = BLEDescriptor("2902", 0);

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiA);

//void handleNoteOn(byte channel, byte pitch, byte velocity)
//{
//	msgBuf[2] = 0x09 & channel;
//	msgBuf[3] = pitch;
//	msgBuf[4] = velocity;
//	characteristic.setValue(msgBuf, 5);
//}
//
//void handleNoteOff(byte channel, byte pitch, byte velocity)
//{
//	msgBuf[2] = 0x08 & channel;
//	msgBuf[3] = pitch;
//	msgBuf[4] = velocity;
//	characteristic.setValue(msgBuf, 5);
//}

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
	//midiA.setHandleNoteOn(handleNoteOn);
	//midiA.setHandleNoteOff(handleNoteOff);

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
	midiA.turnThruOff();

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
		//Prep the timestamp
		msOffset = millis();
		
		digitalWrite(LED_PIN, 0);
		// central connected to peripheral

		while (central.connected()) {
			digitalWrite(GREEN_STAT_PIN, 0);
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
			parseMIDIonDIN();
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
//	msgBuf[2] = 0x09 & channel;
//	msgBuf[3] = pitch;
//	msgBuf[4] = velocity;
//	characteristic.setValue(msgBuf, 5);
void parseMIDIonDIN()
{
	unsigned long currentMillis = millis();
	if(currentMillis < 5000){
		if(msOffset > 5000){
			//it's been 49 days, millis rolled.
			while(msOffset > 5000){
				//roll msOffset - this should preserve current ~8 second count.
				msOffset += MAX_MS;
			}
		}
	}
	while(currentMillis >= (unsigned long)(msOffset + MAX_MS)){
		msOffset += MAX_MS;
	}
	unsigned long currentTimeStamp = currentMillis - msOffset;
	msgBuf[0] = ((currentTimeStamp >> 7) & 0x3F) | 0x80; //6 bits plus MSB
	msgBuf[1] = (currentTimeStamp & 0x7F) | 0x80; //7 bits plus MSB
	if (  midiA.read())
	{
		digitalWrite(RED_STAT_PIN, 0);
		uint8_t statusByte = ((uint8_t)midiA.getType() | ((midiA.getChannel() - 1) & 0x0f));
		switch (midiA.getType())
		{
		case midi::NoteOff :
		case midi::NoteOn :
			msgBuf[2] = statusByte;
            msgBuf[3] = midiA.getData1();
			msgBuf[4] = midiA.getData2();
			characteristic.setValue(msgBuf, 5);
			break;
		case midi::AfterTouchPoly :
//			{
//				Serial.print("PolyAT, chan: ");
//				Serial.print(midiA.getChannel());
//				Serial.print(" Note#: ");
//				Serial.print(midiA.getData1());
//				Serial.print(" AT: ");
//				Serial.println(midiA.getData2());
//			}
			break;
		case midi::ControlChange :
//			{
//				Serial.print("Controller, chan: ");
//				Serial.print(midiA.getChannel());
//				Serial.print(" Controller#: ");
//				Serial.print(midiA.getData1());
//				Serial.print(" Value: ");
//				Serial.println(midiA.getData2());
//			}
			break;
		case midi::ProgramChange :
//			{
//				Serial.print("PropChange, chan: ");
//				Serial.print(midiA.getChannel());
//				Serial.print(" program: ");
//				Serial.println(midiA.getData1());
//			}
			break;
		case midi::AfterTouchChannel :
//			{
//				Serial.print("ChanAT, chan: ");
//				Serial.print(midiA.getChannel());
//				Serial.print(" program: ");
//				Serial.println(midiA.getData1());
//
//			}
			break;
		case midi::PitchBend :
//			{
//				uint16_t val;
//
//				Serial.print("Bend, chan: ");
//				Serial.print(midiA.getChannel());
//
//				// concatenate MSB,LSB
//				// LSB is Data1
//				val = midiA.getData2() << 7 | midiA.getData1();
//
//				Serial.print(" value: 0x");
//				Serial.println(val, HEX);
//
//
//			}
			break;
		case midi::SystemExclusive :
//			{
//				// Sysex is special.
//				// could contain very long data...
//				// the data bytes form the length of the message,
//				// with data contained in array member
//				uint16_t length;
//				const uint8_t  * data_p;
//
//				Serial.print("SysEx, chan: ");
//				Serial.print(midiA.getChannel());
//				length = midiA.getSysExArrayLength();
//
//				Serial.print(" Data: 0x");
//				data_p = midiA.getSysExArray();
//				for (uint16_t idx = 0; idx < length; idx++)
//				{
//					Serial.print(data_p[idx], HEX);
//					Serial.print(" 0x");
//				}
//				Serial.println();
//			}
			break;
		case midi::TimeCodeQuarterFrame :
//			{
//				// MTC is also special...
//				// 1 byte of data carries 3 bits of field info 
//				//      and 4 bits of data (sent as MS and LS nybbles)
//				// It takes 2 messages to send each TC field,
//				
//				Serial.print("TC 1/4Frame, type: ");
//				Serial.print(midiA.getData1() >> 4);
//				Serial.print("Data nybble: ");
//				Serial.println(midiA.getData1() & 0x0f);
//			}
			break;
		case midi::SongPosition :
//			{
//				// Data is the number of elapsed sixteenth notes into the song, set as 
//				// 7 seven-bit values, LSB, then MSB.
//				
//				Serial.print("SongPosition ");
//				Serial.println(midiA.getData2() << 7 | midiA.getData1());
//			}
			break;
		case midi::SongSelect :
//			{
//				Serial.print("SongSelect ");
//				Serial.println(midiA.getData1());
//			}
			break;
		case midi::TuneRequest :
//			{
//				Serial.println("Tune Request");
//			}
			break;
		case midi::Clock :
//			{
//				ticks++;
//
//				Serial.print("Clock ");
//				Serial.println(ticks);
//			}
			break;
		case midi::Start :
//			{
//				ticks = 0;
//				Serial.println("Starting");
//			}
			break;
		case midi::Continue :
//			{
//				ticks = old_ticks;
//				Serial.println("continuing");
//			}
			break;
		case midi::Stop :
//			{
//				old_ticks = ticks;
//				Serial.println("Stopping");
//			}
			break;
		case midi::ActiveSensing :
//			{
//				Serial.println("ActiveSense");
//			}
			break;
		case midi::SystemReset :
//			{
//				Serial.println("Stopping");
//			}
			break;
		case midi::InvalidType :
//			{
//				Serial.println("Invalid Type");
//			}
			break;
		default:
//			{
//				Serial.println();
//			}
			break;
		}
		digitalWrite(RED_STAT_PIN, 1);
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
	//characteristic.setEventHandler(BLEWritten, BLEWrittenCallback);
	//characteristic.setEventHandler(BLESubscribed, BLESubscribedCallback);
	//characteristic.setEventHandler(BLEUnsubscribed, BLEUnsubscribedCallback);

	blePeripheral.begin();
}

//// callback signature
//void BLEWrittenCallback(BLECentral& central, BLECharacteristic& characteristic) {
//	// ....
//
//}
//
//// callback signature
//void BLESubscribedCallback(BLECentral& central, BLECharacteristic& characteristic) {
//	// ....
//
//}
//
//// callback signature
//void BLEUnsubscribedCallback(BLECentral& central, BLECharacteristic& characteristic) {
//	// ....
//
//}
////    event - BLEWritten, BLESubscribed, or BLEUnsubscribed