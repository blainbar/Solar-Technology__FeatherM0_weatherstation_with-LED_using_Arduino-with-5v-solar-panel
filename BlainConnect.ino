/*
This example connects to an unencrypted Wifi network.
Then it prints the  MAC address of the Wifi shield,
the IP address obtained, and other network details.
It then connects the device to the IoT hub and send data to
table if SQL Server and then to PowerBI.  The reading of
temperature, pressure and humidity and showe this on LED display.
Using C Code and Arduino board and Adafruit sensors. 

Blain Barton
blainbar@microsoft.com
Feb 12th, 2017
*/


#include <RTCZero.h>
//Allows an Arduino Feather MO or MKR1000 board to control and use the internal RTC (Real Time Clock
#include <SPI.h>
//Allows you to communicate with one or more SPI (Serial Peripheral Interface) devices
#include <Wire.h>
//Allows you to communicate with I 2 C devices, often also called "2 wire" or "TWI" (Two Wire Interface
#include <Adafruit_Sensor.h>
//There are two main typedefs and one enum defined in Adafruit_Sensor.h that are used to 'abstract' away the sensor details and values:
#include <Adafruit_BME280.h>
//BME280 sensor, an environmental sensor with temperature, barometric pressure and humidity
#include <WiFi101.h>
//Wifi Library
#include <ArduinoJson.h>
//aJson is an Arduino library to enable JSON processing with Arduino

//LED display
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//char ssid[] = "BBGUEST";         // the name of your SSID wifi network
//char pass[] = "hendrick5";       // the name of your Password wifi network
char ssid[] = "Verizon-MiFi5510L-65F0-Barton";       // the name of your network
char pass[] = "xxxxxxxxx";     // the name of your network

int status = WL_IDLE_STATUS;      // the Wifi radio's status

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2

#define LOGO16_GLCD_HEIGHT 16 
#define LOGO16_GLCD_WIDTH  16 

static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
B00000001, B11000000,
B00000001, B11000000,
B00000011, B11100000,
B11110011, B11100000,
B11111110, B11111000,
B01111110, B11111111,
B00110011, B10011111,
B00011111, B11111100,
B00001101, B01110000,
B00011011, B10100000,
B00111111, B11100000,
B00111111, B11110000,
B01111100, B11110000,
B01110000, B01110000,
B00000000, B00110000 };
// end for display

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

Adafruit_BME280 bme; // I2C
					 //Adafruit_BME280 bme(BME_CS); // hardware SPI
					 //Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

WiFiSSLClient client;
RTCZero rtcTime;

String uri = "/devices/Con1/messages/events?api-version=2016-02-03";  
char hostname[] = "IoT Hub Primary 1 string from Azure IoTOwner";                   
char authSAS[] = "SharedAccessSignature string located in Device explorer";

// Standard Endpoint uri below to use to send message /devices/{device name}/messages/events?api-version=2016-02-03
// String uri = "/devices/Feather1/messages/events?api-version=2016-02-03";
// host name address for your Azure IoT Hub
// char hostname[] = "ArtTempIOT.azure-devices.net";
// Use Device Explorer to generate a sas token on config page.
// char authSAS[] = "SharedAccessSignature sr=ArtTempIOT.azure-devices.net&sig=vmUF6p3IANfHmNWrvk4Zf%2BlpngD365hUX9f%2FB2zNaUM%3D&se=1515799811&skn=iothubowner";*/

// Declare json functions
String createJsonData(String devId, float tmp, float pres, float hum);

void setup() {

	Serial.println("--------------Setup Begin-----------------");

	//Initialize serial and wait for port to open:
	WiFi.setPins(8, 7, 4, 2);
	Serial.begin(9600);

	// by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
	display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x64)
												// Show image buffer on the display hardware.
												// Since the buffer is intialized with an Adafruit splashscreen
												// internally, this will display the splashscreen.
	display.display();
	delay(2000);

	// Clear the buffer.
	display.clearDisplay();

	// draw a single pixel
	display.drawPixel(10, 10, WHITE);
	// Show the display buffer on the hardware.
	// NOTE: You _must_ call display after making any drawing commands
	// to make them visible on the display hardware!

	display.display();
	delay(2000);
	display.clearDisplay();

	// text display tests
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0, 0);
	display.println("Display Initialized");
	display.display();

	// not needed. without this when you upload the sketch it will boot when it has power
	// this is waiting for you to start the serial port monitor.
	//while (!Serial) {
	//  ; // wait for serial port to connect. Needed for native USB port only
	//}

	// check for the presence of the shield:
	if (WiFi.status() == WL_NO_SHIELD) {
		Serial.println("WiFi shield not present");
		// don't continue:
		while (true);
	}

	// attempt to connect to Wifi network:
	while (status != WL_CONNECTED) {
		display.clearDisplay();
		display.setCursor(0, 0);
		display.println("Connecting to ");
		display.print(ssid);
		display.display();

		Serial.print("Attempting to connect to open SSID: ");
		Serial.println(ssid);
		status = WiFi.begin(ssid, pass);

		// wait 10 seconds for connection:
		delay(10000);
		Serial.print("Status: ");
		Serial.println(status);
	}

	// you're connected now, so print out the data:
	Serial.print("You're connected to the network");
	printCurrentNet();
	printWifiData();

	display.clearDisplay();
	display.setCursor(0, 0);
	display.println("Connected");
	display.print(ssid);
	display.display();

	initIOT();

	Serial.println("--------------Setup Complete-----------------");
}

void loop() {

	while (true)
	{
		// show wifi connection status
		printCurrentNet();

		// read sensor and format data to json string
		String myData = readSensor();

		// send json to Azure
		httpRequest("POST", uri, "application/atom+xml;type=entry;charset=utf-8", myData);

		Serial.println("");
		Serial.println("--- post complete ---");
		delay(30000);	// wait 30 seconds

		display.clearDisplay();

	}
}

String getDate()
{
	String  myDate;
	myDate.concat(rtcTime.getMonth());
	myDate.concat('/');
	myDate.concat(rtcTime.getDay());
	myDate.concat('/');
	myDate.concat(rtcTime.getYear());

	return myDate;

}
String getTime()
{
	String  myTime;
	long hrs = ((rtcTime.getEpoch() % 86400L) / 3600);		// getEpoch() returns Greenwich Mean Time 86400 = seconds in 1 day
	int min = ((rtcTime.getEpoch() % 3600) / 60);
	int sec = (rtcTime.getEpoch() % 60);

	myTime.concat(hrs);
	myTime.concat(':');
	myTime.concat(min);
	myTime.concat(':');
	myTime.concat(sec);

	return myTime;
}

long getHours()
{
	long hrs = ((rtcTime.getEpoch() % 86400L) / 3600);
	return hrs;
}

void initIOT() {

	Serial.println("--------------Begin IOT setup-----------------");

	rtcTime.begin();

	unsigned long epochTime = WiFi.getTime();
	rtcTime.setEpoch(epochTime);

	Serial.println("Date:" + getDate());
	Serial.println("Time:" + getTime());

	Serial.println("--------------End IOT setup-----------------");
}

String readSensor()
{

	// Pressure is returned in the SI units of Pascals. 100 Pascals = 1 hPa = 1 millibar.
	// Often times barometric pressure is reported in millibar or inches - mercury.
	// For future reference 1 pascal = 0.000295333727 inches of mercury, or 1 inch Hg = 3386.39 Pascal.
	// So if you take the pascal value of say 100734 and divide by 3386.39 you'll get 29.72 inches-Hg.

	Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);
	if (!bme.begin()) {
		Serial.println("Could not find a valid BME280 sensor, check wiring!");
		while (1);
	}

	float tmp = ((9.0 / 5.0) * bme.readTemperature()) + 32;
	float hum = bme.readHumidity();
	float pre = bme.readPressure() / 3386.39; // see above converting to inch Hg

											  // text display tests
	display.clearDisplay();
	display.setTextSize(1);
	display.setTextColor(WHITE);

	display.setCursor(0, 0);
	display.print("Time :");
	display.println(getTime());

	display.print("Temperature:");
	display.println(tmp);
	display.display();

	Serial.print("Temperature :");
	Serial.println(tmp);

	display.print("Humidity :");
	display.println(hum);
	display.display();

	Serial.print("Humidity :");
	Serial.println(hum);

	Serial.print("Pressure :");
	Serial.println(pre);

	display.print("Pressure :");
	display.println(pre);
	display.display();

	// json to string 
	String myData = createJsonData("Con1", tmp, pre, hum);

	return myData;
}

String createJsonData(String devId, float tmp, float pres, float hum)
{

	// create json object
	StaticJsonBuffer<200> jsonBuffer;
	char* buff;
	String outdata;

	JsonObject& root = jsonBuffer.createObject();
	root["DeviceId"] = devId;
	root["date"] = getDate();
	root["time"] = getTime();
	root["hours"] = getHours();
	root["temperature"] = tmp;
	root["humidity"] = hum;
	root["pressure"] = pres;

	// convert to string
	root.printTo(outdata);
	return outdata;

}

void printWifiData() {
	// print your WiFi shield's IP address:
	IPAddress ip = WiFi.localIP();
	Serial.print("IP Address: ");
	Serial.println(ip);
	Serial.println(ip);

	// print your MAC address:
	byte mac[6];
	WiFi.macAddress(mac);
	Serial.print("MAC address: ");
	Serial.print(mac[5], HEX);
	Serial.print(":");
	Serial.print(mac[4], HEX);
	Serial.print(":");
	Serial.print(mac[3], HEX);
	Serial.print(":");
	Serial.print(mac[2], HEX);
	Serial.print(":");
	Serial.print(mac[1], HEX);
	Serial.print(":");
	Serial.println(mac[0], HEX);

	// print your subnet mask:
	IPAddress subnet = WiFi.subnetMask();
	Serial.print("NetMask: ");
	Serial.println(subnet);

	// print your gateway address:
	IPAddress gateway = WiFi.gatewayIP();
	Serial.print("Gateway: ");
	Serial.println(gateway);
}

void printCurrentNet() {
	// print the SSID of the network you're attached to:
	Serial.print("SSID: ");
	Serial.println(WiFi.SSID());

	// print the MAC address of the router you're attached to:
	byte bssid[6];
	WiFi.BSSID(bssid);
	Serial.print("BSSID: ");
	Serial.print(bssid[5], HEX);
	Serial.print(":");
	Serial.print(bssid[4], HEX);
	Serial.print(":");
	Serial.print(bssid[3], HEX);
	Serial.print(":");
	Serial.print(bssid[2], HEX);
	Serial.print(":");
	Serial.print(bssid[1], HEX);
	Serial.print(":");
	Serial.println(bssid[0], HEX);

	// print the received signal strength:
	long rssi = WiFi.RSSI();
	Serial.print("signal strength (RSSI):");
	Serial.println(rssi);

	// print the encryption type:
	byte encryption = WiFi.encryptionType();
	Serial.print("Encryption Type:");
	Serial.println(encryption, HEX);
}

void httpRequest(String verb, String uri, String contentType, String content)
{
	Serial.println("--- Start Process --- ");
	if (verb.equals("")) return;
	if (uri.equals("")) return;

	// close any connection before send a new request.
	// This will free the socket on the WiFi shield
	client.stop();

	// if there's a successful connection:
	if (client.connect(hostname, 443)) {
		Serial.println("--- We are Connected --- ");
		Serial.print("*** Sending Data To:  ");
		Serial.println(hostname + uri);

		Serial.print("*** Data To Send:  ");
		Serial.println(content);

		client.print(verb); //send POST, GET or DELETE
		client.print(" ");
		client.print(uri);  // any of the URI
		client.println(" HTTP/1.1");
		client.print("Host: ");
		client.println(hostname);  //with hostname header
		client.print("Authorization: ");
		client.println(authSAS);  //Authorization SAS token obtained from Azure IoT device explorer
								  //client.println("Connection: close");

		if (verb.equals("POST"))
		{
			Serial.println("--- Sending POST ----");
			client.print("Content-Type: ");
			client.println(contentType);
			client.print("Content-Length: ");
			client.println(content.length());
			client.println();
			client.println(content);
		}
		else
		{
			Serial.println("--- client status --- ");
			Serial.println(client.status());

			client.println();

		}
	}

	while (client.available()) {
		char c = client.read();
		Serial.write(c);
	}

	Serial.println("--- Send complete ----");
}
