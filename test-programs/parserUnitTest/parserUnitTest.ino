//Run on 328p for ease
#include <Arduino.h>

uint8_t note_off[5] = {0x80, 0x80, 0x80, 0x40, 0x40};
uint8_t note_on[5] = {0x80, 0x80, 0x90, 0x40, 0x40};
uint8_t test_1[13] = {0xBD, 0xE9, 0xB0, 0x62, 0x01, 0xE9, 0xB0, 0x06, 0x07, 0xE9, 0xB0, 0x26, 0x11};
uint8_t note_on_rs[9] = {0x80, 0x80, 0x90, 0x3C, 0x40, 0x3D, 0x40, 0x3E, 0x40 };
uint8_t prog_change[8] = {0x80, 0x80, 0xC0, 0x3C, 0x80, 0x90, 0x40, 0x40 };
uint8_t channel_Pressure[7] = {0x80, 0x80, 0xD0, 0x3C, 0x80, 0xD0, 0x3C };
uint8_t channel_Pressure_rs[7] = {0x80, 0x80, 0xD0, 0x3C, 0x3D, 0x3E, 0x3F };
uint8_t Pitch_bend[9] = {0x80, 0x80, 0xE0, 0x3C, 0x40, 0xFD, 0x80, 0x3E, 0x40 };

class FakeCharacteristic
{
public:
	FakeCharacteristic(void){dataLength = 0;};
	uint8_t * value(void){return dataBuffer;};
	uint8_t valueLength(void){return dataLength;};
	uint8_t * dataBuffer;
	uint8_t dataLength;
};

FakeCharacteristic characteristic;

void setup() {
	Serial.begin(115200);
	Serial.println("Unit test started");
	
	characteristic.dataBuffer = test_1;//Set test data
	characteristic.dataLength = sizeof(test_1);
	showData();//Show data
	processPacket();//Run test
	
	characteristic.dataBuffer = note_on_rs;//Set test data
	characteristic.dataLength = sizeof(note_on_rs);
	showData();//Show data
	processPacket();//Run test
	
	
	characteristic.dataBuffer = prog_change;//Set test data
	characteristic.dataLength = sizeof(prog_change);
	showData();//Show data
	processPacket();//Run test
	
	characteristic.dataBuffer = channel_Pressure;//Set test data
	characteristic.dataLength = sizeof(channel_Pressure);
	showData();//Show data
	processPacket();//Run test

	characteristic.dataBuffer = channel_Pressure_rs;//Set test data
	characteristic.dataLength = sizeof(channel_Pressure);
	showData();//Show data
	processPacket();//Run test
	
	characteristic.dataBuffer = Pitch_bend;//Set test data
	characteristic.dataLength = sizeof(Pitch_bend);
	showData();//Show data
	processPacket();//Run test


}

void loop() {
}

void showData( void )
{
	Serial.println();
	uint8_t * buffer = (uint8_t*)characteristic.value();
	Serial.print("Test MIDI BLE packet: 0x");
	for( int i = 0; i < characteristic.valueLength(); i++ ){
		if( buffer[i] < 0x10 ) Serial.print("0");
		Serial.print( buffer[i], HEX );
		Serial.print(", ");
	}
	Serial.println();

}

//This normally connects to the midi system
void transmitMIDIonDIN( uint8_t status, uint8_t data1, uint8_t data2 ){
	Serial.print("Sending: 0x");
	Serial.print(status, HEX);
	Serial.print(", 0x");
	Serial.print(data1, HEX);
	Serial.print(", 0x");
	Serial.print(data2, HEX);
	Serial.println();
}

//******This is the UUT******

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
