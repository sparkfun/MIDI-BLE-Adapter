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
				//hang to give the LED time to show (not necessary if routines are here)
				delay(10);
				digitalWrite(RED_STAT_PIN, 1); 
			}
			//Check if data exists coming in from the serial port
			parseMIDIonDIN();
		}
	}
	//No longer connected.  Turn off the LEDs.
	digitalWrite(BLUE_STAT_PIN, 1);
	digitalWrite(GREEN_STAT_PIN, 1);
	//Delay to show off state for a bit
	delay(100);
}

//This function is called to check if MIDI data has come in through the serial port.  If found, it builds a characteristic buffer and sends it over BLE.
void parseMIDIonDIN()
{
	//Check MIDI object for new data.
	if (  MIDI.read())
	{
		digitalWrite(RED_STAT_PIN, 0);

		uint8_t msgBuf[5]; //Outgoing buffer
	
		//Calculate timestamp.
		uint16_t currentTimeStamp = millis() & 0x01FFF;
		
		msgBuf[0] = ((currentTimeStamp >> 7) & 0x3F) | 0x80; //6 bits plus MSB
		msgBuf[1] = (currentTimeStamp & 0x7F) | 0x80; //7 bits plus MSB
	
		uint8_t statusByte = ((uint8_t)MIDI.getType() | ((MIDI.getChannel() - 1) & 0x0f));
		
		switch (MIDI.getType())
		{
			//2 Byte Channel Messages
			case midi::NoteOff :
			case midi::NoteOn :
			case midi::AfterTouchPoly :
			case midi::ControlChange :
			case midi::PitchBend :
				msgBuf[2] = statusByte;
				msgBuf[3] = MIDI.getData1();
				msgBuf[4] = MIDI.getData2();
				characteristic.setValue(msgBuf, 5);
				break;
			//1 Byte Channel Messages
			case midi::ProgramChange :
			case midi::AfterTouchChannel :
				msgBuf[2] = statusByte;
				msgBuf[3] = MIDI.getData1();
				characteristic.setValue(msgBuf, 4);
				break;
			//System Common Messages
			case midi::TimeCodeQuarterFrame :
				msgBuf[2] = 0xF1;
				msgBuf[3] = MIDI.getData1();
				characteristic.setValue(msgBuf, 4);
				break;
			case midi::SongPosition :
				msgBuf[2] = 0xF2;
				msgBuf[3] = MIDI.getData1();
				msgBuf[4] = MIDI.getData2();
				characteristic.setValue(msgBuf, 5);
				break;
			case midi::SongSelect :
				msgBuf[2] = 0xF3;
				msgBuf[3] = MIDI.getData1();
				characteristic.setValue(msgBuf, 4);
				break;
			case midi::TuneRequest :
				msgBuf[2] = 0xF6;
				characteristic.setValue(msgBuf, 3);
				break;
				//Real-time Messages
			case midi::Clock :
				msgBuf[2] = 0xF8;
				characteristic.setValue(msgBuf, 3);
				break;
			case midi::Start :
				msgBuf[2] = 0xFA;
				characteristic.setValue(msgBuf, 3);
				break;
			case midi::Continue :
				msgBuf[2] = 0xFB;
				characteristic.setValue(msgBuf, 3);
				break;
			case midi::Stop :
				msgBuf[2] = 0xFC;
				characteristic.setValue(msgBuf, 3);
				break;
			case midi::ActiveSensing :
				msgBuf[2] = 0xFE;
				characteristic.setValue(msgBuf, 3);
				break;
			case midi::SystemReset :
				msgBuf[2] = 0xFF;
				characteristic.setValue(msgBuf, 3);
				break;
			//SysEx
			case midi::SystemExclusive :
//				{
//					// Sysex is special.
//					// could contain very long data...
//					// the data bytes form the length of the message,
//					// with data contained in array member
//					uint16_t length;
//					const uint8_t  * data_p;
//	
//					Serial.print("SysEx, chan: ");
//					Serial.print(MIDI.getChannel());
//					length = MIDI.getSysExArrayLength();
//	
//					Serial.print(" Data: 0x");
//					data_p = MIDI.getSysExArray();
//					for (uint16_t idx = 0; idx < length; idx++)
//					{
//						Serial.print(data_p[idx], HEX);
//						Serial.print(" 0x");
//					}
//					Serial.println();
//				}
				break;
			case midi::InvalidType :
			default:
				break;
		}
		digitalWrite(RED_STAT_PIN, 1);
	}

}

void setupBLE()
{
	blePeripheral.setLocalName("DIN to BLE"); //local name sometimes used by central
	blePeripheral.setDeviceName("DIN to BLE"); //device name sometimes used by central
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
