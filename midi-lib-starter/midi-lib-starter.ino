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
	//Send midi data by the press of the button to test while running.
	if(digitalRead(BTN_PIN) == 0){
		digitalWrite(GREEN_STAT_PIN, 0);
		MIDI.sendNoteOn(0x45, 80, 1);
		delay(100);
		MIDI.sendNoteOff(0x45, 80, 1);
		digitalWrite(GREEN_STAT_PIN, 1);
	}
	if (central) {
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

void processPacket()
{
	//Receive the written packet and parse it out here.
	uint8_t * buffer = (uint8_t*)characteristic.value();
	uint8_t bufferSize = characteristic.valueLength();
	//hang to give the LED time to show (not necessary if routines are here)
	delay(10);
}

void parseMIDIonDIN()
{
	if (  MIDI.read())
	{
		digitalWrite(RED_STAT_PIN, 0);
		//hang to give the LED time to show (not necessary if routines are here)
		delay(10);
		digitalWrite(RED_STAT_PIN, 1);
	}

}

void setupBLE()
{
	blePeripheral.setLocalName("MIDI BLE Starter"); //local name sometimes used by central
	blePeripheral.setDeviceName("MIDI BLE Starter"); //device name sometimes used by central
	//blePeripheral.setApperance(0x0000); //default is 0x0000, what should this be?
	blePeripheral.setAdvertisedServiceUuid(service.uuid()); //Advertise MIDI UUID

	// add attributes (services, characteristics, descriptors) to peripheral
	blePeripheral.addAttribute(service);
	blePeripheral.addAttribute(characteristic);
	blePeripheral.addAttribute(descriptor);

	// set initial value
	characteristic.setValue(0);

	// set event handlers - Alternate ways of checking for BLE activity
	//characteristic.setEventHandler(BLEWritten, BLEWrittenCallback);
	//characteristic.setEventHandler(BLESubscribed, BLESubscribedCallback);
	//characteristic.setEventHandler(BLEUnsubscribed, BLEUnsubscribedCallback);

	blePeripheral.begin();
}
