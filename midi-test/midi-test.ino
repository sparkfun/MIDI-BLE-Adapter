#include <MIDI.h>
#include "nrf52.h"

// Simple tutorial on how to receive and send MIDI messages.
// Here, when receiving any message on channel 4, the Arduino
// will blink a led and play back a note for 1 second.

MIDI_CREATE_INSTANCE(HardwareSerial, Serial, midiA);

static const unsigned ledPin = 7;      // LED pin on Arduino Uno

void setup()
{
	Serial.begin(9600);
    pinMode(ledPin, OUTPUT);
    midiA.begin(MIDI_CHANNEL_OMNI);                      // Launch MIDI and listen to channel 4
	NRF_UARTE_Type * myUart;
	myUart = (NRF_UARTE_Type *)NRF_UART0_BASE;
	myUart->BAUDRATE = 0x7FFC80;
	//uint16_t * address;
	//address * = (uint16_t*)(NRF_UART0_BASE);
}

void loop()
{
    if (midiA.read())                    // If we have received a message
    {
        digitalWrite(ledPin, 0);
        midiA.sendNoteOn(42, 127, 1);    // Send a Note (pitch 42, velo 127 on channel 1)
        delay(1000);		            // Wait for a second
        midiA.sendNoteOff(42, 0, 1);     // Stop the note
        digitalWrite(ledPin, 1);
    }
}


//nrfBaudRate = 0x7FFC80; //For 31250 baud