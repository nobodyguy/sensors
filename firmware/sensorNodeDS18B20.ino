//*********************************************************************************************
// Sample RFM69 sender/node sketch, with ACK and optional encryption, and Automatic Transmission Control
// Sends periodic static messages to gateway (id=1)
//*********************************************************************************************
#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/rfm69
#include <SPI.h>           //included with Arduino IDE install (www.arduino.cc)
#include "LowPower.h"      //get it here: https://github.com/rocketscream/Low-Power
#include <OneWire.h>
#include <DallasTemperature.h>

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
#define NODEID        2    //must be unique for each node on same network (range up to 254, 255 is used for broadcast)
#define NETWORKID     100  //the same on all nodes that talk to each other (range up to 255)
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
//#define FREQUENCY   RF69_915MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
//#define IS_RFM69HW  //uncomment only for RFM69HW! Leave out if you have RFM69W!
//*********************************************************************************************
//Auto Transmission Control - dials down transmit power to save battery
//Usually you do not need to always transmit at max output power
//By reducing TX power even a little you save a significant amount of battery power
//This setting enables this gateway to work with remote nodes that have ATC enabled to
//dial their power down to only the required level (ATC_RSSI)
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80
//*********************************************************************************************

// Temperature data wire is plugged into port 4 on the Arduino
#define TEMP_DATA 4
#define TEMP_POWER 5

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMP_DATA);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

char payload[50];

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

void setup() {
	radio.initialize(FREQUENCY, NODEID, NETWORKID);
	radio.encrypt(ENCRYPTKEY);
	//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
	//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
	//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
	//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
	radio.enableAutoPower(ATC_RSSI);
#endif
	radio.sleep();

	pinMode(TEMP_POWER, OUTPUT);
	digitalWrite(TEMP_POWER, HIGH);
	delay(500);
	sensors.begin();
	delay(500);
}

void loop() {
	pinMode(TEMP_POWER, OUTPUT);
	digitalWrite(TEMP_POWER, HIGH);
	delay(50);
	sensors.requestTemperatures(); // Send the command to get temperatures
	delay(50);
	double temp = sensors.getTempCByIndex(0);
	digitalWrite(TEMP_POWER, LOW);
	pinMode(TEMP_POWER, INPUT);
	char tempBuffer[5];
	int vcc = readVcc();
	dtostrf(temp, 4, 2, tempBuffer);
	sprintf(payload, "{\"temperature\":%s,\"vcc\":%d}", tempBuffer, vcc);

	radio.sendWithRetry(GATEWAYID, payload, strlen(payload));
	radio.sleep();
	sleep(30); // sleep Arduino for 30 minutes
}

void sleep(int minutes)
{
	int seconds = minutes * 60;
	int iterations = seconds / 8;

	if ((seconds % 8) == 4)
	{
		// Enter power down state for 4s with ADC and BOD module disabled
		LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
	}

	for (int i = 0; i < iterations; i++)
	{
		LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
	}
}

//--------------------------------------------------------------------------------------------------
// Read current supply voltage
//--------------------------------------------------------------------------------------------------
long readVcc() {
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA, ADSC)); // measuring

	uint8_t low = ADCL; // must read ADCL first - it then locks ADCH  
	uint8_t high = ADCH; // unlocks both

	long result = (high << 8) | low;

	result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
}