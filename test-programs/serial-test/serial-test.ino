unsigned long msOffset = 0;

#define MAX_MS 0x01FFF //13 bits, 8192 dec


void setup()
{
  delay(3000);  //5 seconds delay for enabling to see the start up comments on the serial board
  Serial.end();
  Serial.begin(115200);
  while(Serial.available()){
	  Serial.read();
  }
  Serial.println("Sketch started");
}

void loop()
{
	//unsigned long currentMillis = millis();
	//if(currentMillis < 5000){
	//	if(msOffset > 5000){
	//		//it's been 49 days, millis rolled.
	//		while(msOffset > 5000){
	//			//roll msOffset - this should preserve current ~8 second count.
	//			msOffset += MAX_MS;
	//		}
	//	}
	//}
	//while(currentMillis >= (unsigned long)(msOffset + MAX_MS)){
	//	msOffset += MAX_MS;
	//	Serial.print("offset inc. New timestamp: ");
	//	Serial.println(currentMillis - msOffset);
	//}
	//unsigned long currentTimeStamp = currentMillis - msOffset;
	uint16_t currentTimeStamp = millis() & 0x01FFF;
    if (Serial.available())                    // If we have received a message
    {
		Serial.print("Timestamp (ms): ");
		Serial.println(currentTimeStamp);
		Serial.print("0x");
		Serial.println(Serial.read(), HEX);
    }
}


//nrfBaudRate = 0x7FFC80; //For 31250 baud
