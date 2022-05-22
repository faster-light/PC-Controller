// ------------------------------------- General Header --------------------------------------- //
// Wi-Fi, MQTT, OTA Libraries
#include <WiFi.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
// File with personal information
#include <Personal.h>

#include <Adafruit_NeoPixel.h>
#define LED_PIN_L    16
#define LED_PIN_R    17

#define LED_COUNT    48
Adafruit_NeoPixel strip_L(LED_COUNT, LED_PIN_L, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip_R(LED_COUNT, LED_PIN_R, NEO_GRB + NEO_KHZ800);

// Main variable and other settings
// Set hostname
const char* hostname_OTA = "PC-Controller";
const char* wifi_host = "PC-Controller";
bool connection_error = true;
WiFiClient ESP_32_PowMod;
PubSubClient client(ESP_32_PowMod);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
// -------------------------------------------------------------------------------------------- //

int pc_on_pin = 22;
int dac_left_pin = 25;
int dac_right_pin = 26;

int en_left_pin = 18;
int en_right_pin = 19;

int ws_left_pin = 16;
int ws_right_pin = 17;
int ws_third_pin = 37;

int voltage_left_pin = 34;
int voltage_right_pin = 35;

int voltage_5v_pin = 33;

bool indication_main = false;

// Second core object (parallel main loop worked)
TaskHandle_t SecondCore;


// --------------------------- Char* comparison (usually no change) --------------------------- //
int strcheck(char *str1, const char *str2) {
  int i = 0;
  while(str1[i] != '\0' && str2[i] != '\0'){
    if(str1[i] != str2[i])
      return 0;
    i++;
  }
  if(str1[i] == '\0' && str2[i] == '\0')
    return 1;
  return 0;
}
// --------------------------------- Main loop (second core) ---------------------------------- // 
void SecondCore_Code( void * parameter) {
  for(;;) { 

    delay(50);

  }
}
// ----------------------------- OTA function (usually no change) ----------------------------- //
void initial_OTA() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname(hostname_OTA);
  ArduinoOTA.setPassword(password_OTA);

  ArduinoOTA.onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";
      Serial.println("Start updating " + type);
    });

  ArduinoOTA.begin();
}
// ---------------------------- Event of receiving MQTT messages ------------------------------ //
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  int value = 0;

  if (length == 1) {
    value = payload[0] - '0';
  }

  if (length == 2) {
    int a = payload[0] - '0';
    int b = payload[1] - '0';
    value = a * 10 + b;
  }

  if (length == 3) {
    int a = payload[0] - '0';
    int b = payload[1] - '0';
    int c = payload[2] - '0';
    value = a * 100 + b * 10 + c;
  }

  if (length == 4) {
    int a = payload[0] - '0';
    int b = payload[1] - '0';
    int c = payload[2] - '0';
    int d = payload[3] - '0';
    value = a * 1000 + b * 100 + c * 10 + d;
  }

  if (length == 5) {
    int a = payload[0] - '0';
    int b = payload[1] - '0';
    int c = payload[2] - '0';
    int d = payload[3] - '0';
    int e = payload[4] - '0';
    value = a * 10000 + b * 1000 + c * 100 + d * 10 + e;
  }  

  if (length == 6) {
    int a = payload[0] - '0';
    int b = payload[1] - '0';
    int c = payload[2] - '0';
    int d = payload[3] - '0';
    int e = payload[4] - '0';
    int f = payload[5] - '0';
    value = a * 100000 + b * 10000 + c * 1000 + d * 100 + e * 10 + f;
  }  

  Serial.println(value);

  if (strcheck(topic, topic_light_15)) {
    indication_main = value;
  }

}
// ------------------------ WiFi connect function (usually no change) ------------------------- //
void wifi_connect() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.setHostname(wifi_host);
  WiFi.begin(wifi_ssid, wifi_pass);

  int tries = 0;

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    
    if (tries > 10) {
      Serial.println(" Erorr. WiFi connect failed");
      break;
    }
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
 
}
// ------------------------ MQTT connect function (usually no change) ------------------------- //
void mqtt_connect() {
  // Loop until we're reconnected
  int tries = 0;
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID

    String clientId = wifi_host;
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" connected");
      // Once connected, publish an announcement...
      client.subscribe(topic_light_15);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }

    if (tries > 2) {
      break;      
      Serial.println("Erorr. MQTT connect failed");
    }
    tries++;
  }
}
// ------------ WiFi & MQTT connect/reconnect connect function (usually no change) ------------ //
void wifi_mqtt_servises() {
  if (WiFi.status() != WL_CONNECTED) 
    wifi_connect();
  
  if ((!client.connected()) and (WiFi.status() == WL_CONNECTED)) 
    mqtt_connect();

  client.loop();
}
// ---------------------------------- Main setup function ------------------------------------- //
void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  //Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //Serial2.println("check in serial2 on speed 9600");

  wifi_connect();
  initial_OTA();

  client.setServer(mqtt_host, 1883);
  client.setCallback(callback);

  pinMode(pc_on_pin, OUTPUT);
  digitalWrite(pc_on_pin, LOW);
  
  pinMode(en_left_pin, OUTPUT);
  digitalWrite(en_left_pin, LOW);  

  pinMode(en_right_pin, OUTPUT);
  digitalWrite(en_right_pin, LOW);
  
  strip_L.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip_L.show();            // Turn OFF all pixels ASAP
  strip_L.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  strip_R.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip_R.show();            // Turn OFF all pixels ASAP
  strip_R.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  xTaskCreatePinnedToCore(
      SecondCore_Code, /* Function to implement the task */
      "SecondCore", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &SecondCore,  /* Task handle. */
      0); /* Core where the task should run */
}
// --------------------------------- Main loop (first core) ----------------------------------- // 
void loop() {
  ArduinoOTA.handle();
  wifi_mqtt_servises();

  if (indication_main) {
    for(int i = 0; i < 83; i++) {
      for(int j = 0; j < strip_R.numPixels(); j++) {
        strip_L.setPixelColor(j, strip_R.ColorHSV((234 + i)  * 182, 100 * 2.55, 100 * 2.55));
        strip_R.setPixelColor(j, strip_R.ColorHSV((317 - i) * 182, 100 * 2.55, 100 * 2.55));
      }
      strip_L.show();
      strip_R.show();
      delay(15);
    }

    for(int i = 0; i < 83; i++) {
      for(int j = 0; j < strip_R.numPixels(); j++) {
        strip_L.setPixelColor(j, strip_R.ColorHSV((317 - i) * 182, 100 * 2.55, 100 * 2.55));
        strip_R.setPixelColor(j, strip_R.ColorHSV((234 + i) * 182, 100 * 2.55, 100 * 2.55));
      }
      strip_L.show();
      strip_R.show();
      delay(15);
    }
  }

}
