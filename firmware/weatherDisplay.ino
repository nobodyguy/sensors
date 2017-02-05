#include <SPI.h>
#include <RFM69.h>         //get it here: https://www.github.com/lowpowerlab/rfm69
#include <RFM69_ATC.h>     //get it here: https://www.github.com/lowpowerlab/rfm69
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>

#define NODEID        3    //unique for each node on same network
#define NETWORKID     100  //the same on all nodes that talk to each other
#define FREQUENCY     RF69_433MHZ
#define ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define RFM_SS 10        // Slave Select RFM69 is connected to pin 10

#define sclk 4
#define mosi 5
#define dc   7
#define cs   8
#define rst  6

// Color definitions
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF

#ifdef ENABLE_ATC
RFM69_ATC radio;
#else
RFM69 radio;
#endif

char payload[100];
Adafruit_SSD1351 tft = Adafruit_SSD1351(cs, dc, mosi, sclk, rst);


void setup() {
	radio.setCS(RFM_SS);          // change default Slave Select pin for RFM
	radio.initialize(FREQUENCY, NODEID, NETWORKID);
	radio.promiscuous(true);
	radio.encrypt(ENCRYPTKEY);

	tft.begin();
	tft.fillScreen(BLACK);
	tft.setCursor(0, 5);
	tft.setTextColor(WHITE);
	tft.setTextSize(1);
	tft.println("Waiting for first radio message...");
}

void loop() {
	if (radio.receiveDone()) {
		int senderID = radio.SENDERID;
		sprintf(payload, "{\"sensorID\":%d,\"data\":%s}", senderID, radio.DATA);
		tft.println(payload);
	}

}