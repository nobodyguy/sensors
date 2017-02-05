#include <SPI.h>
#include <Ethernet.h>      //get it here: https://github.com/Wiznet/WIZ_Ethernet_Library
#include <RFM69.h>         //get it here: https://github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://github.com/lowpowerlab/rfm69

#define NODEID        1    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define RFM_SS 8      // Slave Select RFM69 is connected to pin 8

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

// assign a MAC address for the ethernet controller.
// fill in your address here:
byte mac[] = {
	0xB8, 0xCA, 0x3A, 0x8A, 0x03, 0x3B
};

// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 8);

// initialize the library instance:
EthernetClient client;

//char server[] = "www.arduino.cc";
IPAddress server(89, 221, 208, 114);

char payload[100];

// change to your server's port
int serverPort = 80;

// change to the page on that server
char pageName[] = "/gateway-api/";

void setup() {
	radio.setCS(RFM_SS);          // change default Slave Select pin for RFM
	radio.initialize(FREQUENCY, NODEID, NETWORKID);
	radio.encrypt(ENCRYPTKEY);
	// start serial port:
	Serial.begin(9600);
	Serial.println("Starting ethernet module...");

	// give the ethernet module time to boot up:
	delay(1000);

	// start the Ethernet connection:
	if (Ethernet.begin(mac) == 0) {
		Serial.println("Failed to configure Ethernet using DHCP");
		// try to congifure using IP address instead of DHCP:
		Ethernet.begin(mac, ip);
	}

	// give the Ethernet shield a second to initialize:
	delay(1000);

	// print the Ethernet board/shield's IP address:
	Serial.print("My IP address: ");
	Serial.println(Ethernet.localIP());
}

void loop() {
	// if there's incoming data from the net connection.
	// send it out the serial port.  This is for debugging
	// purposes only:

	//if (client.available()) {
	//  char c = client.read();
	//  Serial.write(c);
	//}

	if (radio.receiveDone()) {
		Serial.println("Received radio packet!");
		int senderID = radio.SENDERID;
		sprintf(payload, "{\"sensorID\":%d,\"data\":%s,\"rssi\":%d}", senderID, radio.DATA, radio.RSSI);
		Serial.println(payload);
		if (radio.ACK_REQUESTED) {
			radio.sendACK();
		}

		int tries = 0;
		byte res = httpPostRequest(server, serverPort, pageName, payload);
		while (!res && tries < 3) {
			Serial.println(F("Request failed "));
			tries++;
			if (tries == 3)
			{
				// start the Ethernet connection:
				if (Ethernet.begin(mac) == 0) {
					Serial.println("Failed to configure Ethernet using DHCP");
					// try to congifure using IP address instead of DHCP:
					Ethernet.begin(mac, ip);
				}

				// give the Ethernet shield a second to initialize:
				delay(1000);
			}

			res = httpPostRequest(server, serverPort, pageName, payload);
		}
		if (res)
		{
			Serial.println(F("Request passed "));
		}

	}

}

byte httpPostRequest(IPAddress ip, int thisPort, char* page, char* thisData)
{
	Ethernet.maintain();
	// close any connection before send a new request.
	// This will free the socket on the WiFi shield
	client.stop();

	int inChar;
	char outBuf[50];

	Serial.print(F("connecting..."));

	if (client.connect(server, thisPort) == 1)
	{
		Serial.println(F("connected"));
		Serial.print("Sending payload: ");
		Serial.println(thisData);

		// send the header
		sprintf(outBuf, "POST %s HTTP/1.1", page);
		client.println(outBuf);
		Serial.println(outBuf);
		client.print("Host: ");
		client.println(ip);
		client.println("User-Agent: arduino-gateway");
		client.println("Content-Type: application/json");
		client.println("Connection: close");
		client.println("Accept: */*");
		sprintf(outBuf, "Content-Length: %d", strlen(thisData));
		client.println(outBuf);
		Serial.println(outBuf);
		client.println();
		Serial.println();

		// send the body (variables)
		client.print(thisData);
		Serial.print(thisData);
	}
	else
	{
		Serial.println(F("failed"));
		return 0;
	}

	int connectLoop = 0;

	while (client.connected())
	{
		while (client.available())
		{
			inChar = client.read();
			Serial.write(inChar);
			connectLoop = 0;
		}

		delay(1);
		connectLoop++;
		if (connectLoop > 10000)
		{
			Serial.println();
			Serial.println(F("Timeout"));
			client.stop();
		}
	}

	Serial.println();
	Serial.println(F("disconnecting."));
	client.stop();
	return 1;
}