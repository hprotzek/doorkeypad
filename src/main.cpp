#include "Arduino.h"
#include "Keypad.h"
#include <WiFi.h>
#include <PubSubClient.h>
 
 // keypad
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {18, 23, 13, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {25, 26, 27}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

// door password
#define maxPasswordLenght 10
char password[maxPasswordLenght]; 
byte charCount = 0;

// wifi
const char* ssid = "xxx";
const char* wifiPassword = "xxx";

// mqtt
const char* mqtt_server = "192.168.0.13";

// wifi
WiFiClient espClient;
PubSubClient client(espClient);

// deep sleep
#define BUTTON_PIN_BITMASK 0x400000000 // 4^33 in hex, GPIO 34

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setupWifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, wifiPassword);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    count++;
    Serial.print(".");
    if (count > 4) {
      ESP.restart();
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnectMqtt() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    if (client.connect("DoorKeyPad", "user", "Br73YNM8xW")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      ESP.restart();
    }
  }
}

void setupMqtt() {
  Serial.println();
  Serial.print("Configure MQTT ....");
  Serial.print(mqtt_server);
  Serial.print(":");
  Serial.println(1883);

  client.setServer(mqtt_server, 1883);
  reconnectMqtt();

  Serial.println("");
  Serial.println("successful");
}

void deepSleep() {
  Serial.println("Going to sleep now");
  digitalWrite(2, LOW);
  delay(1000);
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}

void setup(){
  Serial.begin(115200);
  delay(1000);
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  print_wakeup_reason();
  
  setupWifi();
  setupMqtt();
  digitalWrite(2, HIGH);
}

void sendPassword() {
  Serial.println("Send password:" + String(password));
  client.publish("home/door/access", password);
}

void resetPassword() {
  for(int i = 0; i < sizeof(password); ++i) {
    password[i] = (char)0;
  }
  charCount = 0;
}

void checkKeypad() {
  char key = keypad.getKey();
  
  if (key != NO_KEY){
    Serial.println("Key pressed: " + String(key));

    if (key == '#') {
      sendPassword();
      resetPassword();
      deepSleep();
    } else if (charCount < maxPasswordLenght) {
      password[charCount] = key; 
      charCount++;
    } 
  }
}
  
void loop() {
  if (!client.connected()) {
    reconnectMqtt();
  }
  client.loop();

  checkKeypad();

  if(millis() > 30000) {
    deepSleep();
  }
}