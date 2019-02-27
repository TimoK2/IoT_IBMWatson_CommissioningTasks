/*
MKR1000 connecting to IBM Watson IoT Platform

Based on documentation and "recipes" on IBM Bluemix
https://www.ibm.com/cloud-computing/bluemix/watson
Timo Karppinen 19.2.2017

Modified for testing SPI Ambient light sensor board Digilent PmodALS
Please connect
MKR1000 - PmodALS
GND - 5 GND
Vcc - 6 Vcc
9 SCK - 4 SCK
10 MISO - 3 MISO
2 - 1 SS

ALS data on the SPI
D15 ... D13 - three zeros
D12 ... D04 - 8 bits of ambient light data
D03 ... D00 - four zeros

Details on the connection of ADC chip on ALS board are given
http://www.ti.com/lit/ds/symlink/adc081s021.pdf
Timo Karppinen 25.1.2018
 */

#include <SPI.h>
#include <WiFi101.h>
#include <WiFiSSLClient.h>
#include <MQTTClient.h>  // The Gaehwiler mqtt client

// WLAN 
//char ssid[] = "Moto_Z2_TK"; //  your network SSID (name)
//char pass[] = "xxxxxxxxxx";    // your network password (use for WPA)

char ssid[] = "HAMKvisitor"; //  your network SSID (name)
char pass[] = "xxxxxxxxx";    // your network password (use for WPA)

//char ssid[] = "Nelli";
//char pass[] = "xxxxxxxxxxx";



// IBM Watson
// Your organization and device needs to be registered in IBM Watson IoT Platform.
// Instruction for registering on page
// https://internetofthings.ibmcloud.com/#

//char *client_id = "d:<your Organization ID>:<your Device Type>:<your Device ID>"; 
char *client_id = "d:8yyyy:A_MKR1000:sim2_1234";
char *user_id = "use-token-auth";   // telling that authentication will be done with token
char *authToken = "mExxxxxxxa0"; // Your IBM Watson Authentication Token

//char *ibm_hostname = “your-org-id.messaging.internetofthings.ibmcloud.com”;
char *ibm_hostname = "8yyyyyy.messaging.internetofthings.ibmcloud.com";

// sensors and LEDS
const int LEDPin = LED_BUILTIN;     // must be a pin that supports PWM. 0...8 on MKR1000
// PModALS
const int alsCS = 0;        // chip select for sensor SPI communication
byte alsByte0 = 0;           // 8 bit data from sensor board
byte alsByte1 = 0;           // 8 bit data from sensor board
byte alsByteSh0 = 0;
byte alsByteSh1 = 0;
int als8bit = 0;
int alsRaw = 0;
float alsScaledF = 0;


int blinkState = 0;

/*use this class if you connect using SSL
 * WiFiSSLClient net;
*/
WiFiClient net;
MQTTClient MQTTc;

unsigned long lastSampleMillis = 0;
unsigned long previousWiFiBeginMillis = 0;
unsigned long lastWatsonMillis = 0;
unsigned long lastPrintMillis = 0;


void setup() 
{
  Serial.begin(9600);
  delay(2000); // Wait for wifi unit to power up
  WiFi.begin(ssid, pass);
  delay(5000); // Wait for WiFi to connect
  Serial.println("Connected to WLAN");
  printWiFiStatus();
  
  /*
    client.begin("<Address Watson IOT>", 1883, net);
    Address Watson IOT: <WatsonIOTOrganizationID>.messaging.internetofthings.ibmcloud.com
    Example:
    client.begin("iqwckl.messaging.internetofthings.ibmcloud.com", 1883, net);
  */
  MQTTc.begin(ibm_hostname, 1883, net);  // Cut for testing without Watson

  connect();

  SPI.begin();
  // Set up the I/O pins
  pinMode(alsCS, OUTPUT);
  digitalWrite(alsCS, HIGH);   // for not communicating with ALS at the moment
  
  pinMode(LEDPin, OUTPUT);


}

void loop() {
   MQTTc.loop();  // Cut for testing without Watson
 

  // opening and closing SPI communication for reading sensor
  if(millis() - lastSampleMillis > 500)
  { 
    lastSampleMillis = millis();
    SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));
    digitalWrite(alsCS, LOW);
  
    alsByte0 = SPI.transfer(0x00);
    alsByte1 = SPI.transfer(0x00);
  
    digitalWrite(alsCS, HIGH);
    SPI.endTransaction();


    alsByteSh0 = alsByte0 << 4;
    alsByteSh1 = alsByte1 >> 4;
    
    
    als8bit =( alsByteSh0 | alsByteSh1 );
    
    alsRaw = als8bit; // 
    alsScaledF = float(alsRaw)*6.68;

  }

  // Print on serial monitor once in 1000 millisecond
  if(millis() - lastPrintMillis > 1000)
  {
    Serial.print("als bytes from sensor without leading zeros  ");
    Serial.print(alsByteSh0, BIN);
    Serial.print("  ");
    Serial.print(alsByteSh1, BIN);
    Serial.print("  als8bit   ");
    Serial.println(als8bit, BIN);
    Serial.print("  alsRaw  ");
    Serial.println(alsRaw);
    Serial.print("  alsScaledF  ");
    Serial.println(alsScaledF);
 
    lastPrintMillis = millis();
  }
  
     // publish a message every  60 second.
     if(millis() - lastWatsonMillis > 60000) 
     {
      Serial.println("Publishing to Watson...");
        if(!MQTTc.connected()) {    // Cut for testing without Watson
         connect();                 // Cut for testing without Watson
        }                           // Cut for testing without Watson
        lastWatsonMillis = millis();
         //Cut for testing without Watson
    
    //ok   String wpayload = "{\"d\":{\"AmbientLightSensor\":\"ALS1 \",\"ALSScaled\":123.4, \"ALSStreight\":5678}}";
          String wpayload = "{\"d\":{\"AmbientLightSensor\":\"ALS1 \",\"ALSScaled\":" + String(alsScaledF)+ ", \"ALSStreight\":" + String(alsRaw)+"}}";
           
          MQTTc.publish("iot-2/evt/AmbientLight/fmt/json", wpayload);
     
     }
   
    delay(1);
    
// end of loop
}

void connect() 
{
  Serial.print("checking WLAN...");
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.print(".");       // printing a dot every half second
    if ( millis() - previousWiFiBeginMillis > 5000) // reconnecting
    {
      previousWiFiBeginMillis = millis();
      WiFi.begin(ssid, pass);
      delay(5000); // Wait for WiFi to connect
      Serial.println("Connected to WLAN");
      printWiFiStatus();
    }
    delay(500);
    
  }
  /*
    Example:
    MQTTc.connect("d:iqwckl:arduino:oxigenarbpm","use-token-auth","90wT2?a*1WAMVJStb1")
    
    Documentation: 
    https://console.ng.bluemix.net/docs/services/IoT/iotplatform_task.html#iotplatform_task
  */
  
  Serial.print("\nconnecting Watson with MQTT....");
  // Cut for testing without Watson
while (!MQTTc.connect(client_id,user_id,authToken)) 
  {
    Serial.print(".");
    delay(30000); // try again in thirty seconds
  }
  Serial.println("\nconnected!");
}

// messageReceived subroutine needs to be here. MQTT client is calling it.
void messageReceived(String topic, String payload, char * bytes, unsigned int length) {
  Serial.print("incoming: ");
  Serial.print(topic);
  Serial.print(" - ");
  Serial.print(payload);
  Serial.println();
}

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
