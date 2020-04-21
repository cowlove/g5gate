#include <HardwareSerial.h>
#include "SPI.h"
//#include "Update.h"

#ifdef ESP32
#include "Update.h"
#include "WebServer.h"
#include "DNSServer.h"
#include "ESPmDNS.h"
#else
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#include "FS.h"
#include "ArduinoOTA.h"
#include "WiFiManager.h"
#include "WiFiUdp.h"
#include "mavlink.h"

#include "mavlink.h"
#include "Wire.h"

#define LED_PIN 2
#define BUTTON_PIN 0

#ifdef ESP32
#include "WiFiMulti.h"
WiFiMulti wifi;
#endif

WiFiUDP udp;
//const char *udpHost = "192.168.4.100";
const char *udpHost = "255.255.255.255";

// port schema:  7890 - g5 in 
//               7891 - g5 out
//               7892 - ifly in
//               7893 - ifly out

int udpPortIn = 7890;
int udpPortOut = 7891;
int serBytes = 0;
int hdgBug = 0;


class EggTimer {
	uint64_t last;
	int interval; 
public:
	EggTimer(int ms) : interval(ms), last(0) {}
	bool tick() { 
		uint64_t now = millis();
		if (now - last > interval) { 
			last = now;
			return true;
		} 
		return false;
	}
};

bool wifiTryConnect(const char *ap, const char *pw, int seconds = 15) { 
	if (WiFi.status() != WL_CONNECTED) {
		WiFi.begin(ap, pw);
		for(int d = 0; d < seconds * 10 && WiFi.status() != WL_CONNECTED; d++) {
			digitalWrite(LED_PIN, d % 2);
			delay(100);
		}
	}
	return WiFi.status() == WL_CONNECTED;
}
	

void setup() {
	pinMode(LED_PIN, OUTPUT);
	pinMode(3,INPUT);
#ifdef SCREEN
	u8g2.begin();
	u8g2.clearBuffer();					// clear the internal memory
	u8g2.setFont(u8g2_font_courB08_tr);	// choose a suitable font
	u8g2.setCursor(0,10);				// set write position
	u8g2.println("Searching for WiFi");
	u8g2.sendBuffer();
#endif
	
	WiFi.mode(WIFI_STA);
#ifdef ESP32
	WiFi.setSleep(false);
	
	wifi.addAP("Ping-582B", "");
	wifi.addAP("Team America", "51a52b5354");
	wifi.addAP("ChloeNet", "niftyprairie7");
	while (WiFi.status() != WL_CONNECTED) {
		wifi.run();
	}
#else

	//WiFi.begin("ChloeNet", "niftyprairie7");
	wifiTryConnect("Ping-582B", "");
	wifiTryConnect("ChloeNet", "niftyprairie7");
	wifiTryConnect("Ping-582B", "");
	wifiTryConnect("ChloeNet", "niftyprairie7");
	wifiTryConnect("Ping-582B", "");
	wifiTryConnect("ChloeNet", "niftyprairie7");
//	WiFi.begin("Ping-582B", "");
#endif 

	digitalWrite(LED_PIN, 1);
	while (WiFi.status() != WL_CONNECTED) {
#ifdef WIFI_MULTI
		wifi.run();
#endif
		delay(1);
	}

	ArduinoOTA.begin();
	udp.begin(udpPortIn);
	
#ifdef ESP32	
	pinMode(12, INPUT);
	pinMode(13, OUTPUT);
	Serial.begin(9600, SERIAL_8N1, 12, 13);
#else
	Serial.begin(9600, SERIAL_8N1);
#endif

	Serial.setTimeout(10);
}


// From data format described in web search "SL30 Installation Manual PDF" 
class SL30 {
public:
	std::string twoenc(unsigned char x) {
		char r[3];
		r[0] = (((x & 0xf0) >> 4) + 0x30);
		r[1] = (x & 0xf) + 0x30;
		r[2] = 0;
		return std::string(r);
	}
	int chksum(const std::string& r) {
		int sum = 0;
		const char* s = r.c_str();
		while (*s)
			sum += *s++;
		return sum & 0xff;
	}
	void open() {
	}
	void pmrrv(const std::string& r) {
		std::string s = std::string("$PMRRV") + r + twoenc(chksum(r)) + "\r\n";
		Serial.write(s.c_str());
	}
	void setCDI(double hd, double vd) {
		int flags = 0b11111010;
		hd *= 127 / 3;
		vd *= 127 / 3;
		pmrrv(std::string("21") + twoenc(hd) + twoenc(vd) + twoenc(flags));
	}
};

class LineBuffer {
public:
	char line[1024];
	char len;
	int add(char c) {
		int r = 0; 
		line[len++] = c;
		if (len >= sizeof(line) || c == '\n') {
			r = len;
			len = 0;
		}
		return r;
	}
};

class ButtonDebounce {
	EggTimer timer;
	bool recentlyPressed;
public:
	ButtonDebounce(int ms = 250) : timer(ms), recentlyPressed(false) {}
	bool check(bool button) { 
		if (timer.tick())
			recentlyPressed = false;
		if (button == true && recentlyPressed == false) { 
			recentlyPressed = true; 
			return true;
		}
		return false;
	}
};

ButtonDebounce button;


EggTimer blinkTimer(500), screenTimer(200);

#define BUFFER_LENGTH 2041 // minimum buffer size that can be used with qnx (I don't know why)
static uint8_t buf[BUFFER_LENGTH];
static int count = 0;
static double hd = 0, vd = 0;
static bool debugMoveNeedles = false;
SL30 sl30;

void loop() {
//	wifi.run(1);
	ArduinoOTA.handle();

	bool buttonPress = button.check(digitalRead(BUTTON_PIN) == 0);
	if (buttonPress) 
		debugMoveNeedles = !debugMoveNeedles;
		
#ifdef SCREEN
	if (screenTimer.tick()) { 
		u8g2.clearBuffer();					// clear the internal memory
		u8g2.setFont(u8g2_font_courB08_tr);	// choose a suitable font
		u8g2.setCursor(0,10);				// set write position
		u8g2.println(WiFi.localIP());
		u8g2.setCursor(0,20);
		u8g2.printf("TRK: %03d   CRS: %03d", 52, 180);
		u8g2.setCursor(0,30);				
		u8g2.printf("VD: %.1f  HD: %.1f", vd, hd);
		u8g2.setCursor(0,40);					
		u8g2.printf("SER: %04d  HDG: %04d", serBytes % 1000, hdgBug);
		u8g2.sendBuffer();
	}
#endif

	static LineBuffer line;
	if (Serial.available()) {
		int l = Serial.readBytes(buf, sizeof(buf));
		serBytes += l + random(0,2);
		for (int i = 0; i < l; i++) {
			int ll = line.add(buf[i]);
			if (ll) {
				udp.beginPacket(udpHost,udpPortOut);
				udp.write((uint8_t *)line.line, ll);
				udp.endPacket();
				count++;
			}
		}
	}

	if (blinkTimer.tick()) { 
		count++;
		if (debugMoveNeedles) { 
			hd += .1;
			vd += .15;
			if (hd > 2) hd = -2;
			if (vd > 2) vd = -2;
			sl30.setCDI(hd, vd);
		}
		//Serial.write("$PMRRV2100;0?::7\r\n");
	}
	
	unsigned int avail;
	if ((avail = udp.parsePacket()) > 0) { 
		static uint8_t buf[1024];
		int r = udp.read(buf, min(avail, sizeof(buf)));
		Serial.write(buf, r);	
		count++;
	}

	digitalWrite(LED_PIN, (count & 0x1));
	delay(1);
	yield();
}
