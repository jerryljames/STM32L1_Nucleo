
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "math.h"
/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "Redmi"
#define WLAN_PASS       "qwertyuiop"


/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER         "io.adafruit.com"
#define AIO_SERVERPORT      1883                   
#define AIO_USERNAME       "env_sensor_data"
#define AIO_KEY            "5fb17df79ea14b6883b17753539e611c"

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
//Adafruit_MQTT_Publish Device_1 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Device_1");
Adafruit_MQTT_Publish Device_2 = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Device_2");

// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe onoffbutton = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/onoff");

/*************************** Global Functions ************************************/
void MQTT_connect();
/*************************** Global Variables ************************************/
uint16_t sensor_val = 0;
const float e = 2.71828;
float exponent, ppm = 0;
uint16_t ppm_final;
/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).


void setup() {
  Serial.begin(9600);
  delay(10);

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&onoffbutton);

  Serial.println("Preheating MQ7 Sensor for 2 minutes");
  for(int i=0; i<120; i++)
  delay(1000);
}



void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &onoffbutton) {
      Serial.print(F("Got: "));
      Serial.println((char *)onoffbutton.lastread);
    }
  }

  // Now we can publish stuff!
  sensor_val = analogRead(A0);
  Serial.print(F("\nAnalog Value: "));
  Serial.print(sensor_val);
  exponent = float((1.068*sensor_val*3.3)/1023);
  ppm = pow(e, exponent);
  ppm_final = 3.027 * ppm;
  Serial.print(F("\nSending Device 2 val "));
  Serial.print(ppm_final);
  Serial.print("...");
  if (! Device_2.publish(ppm_final)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}
