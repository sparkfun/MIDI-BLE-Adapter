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
    if (Serial.available())                    // If we have received a message
    {
		Serial.print("0x");
		Serial.println(Serial.read(), HEX);
    }
}


//nrfBaudRate = 0x7FFC80; //For 31250 baud