#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>
#include "time.h"
#include <ESP32Servo.h>

Servo myservo1; 
Servo myservo2;
Servo myservo3;
Servo myservo4;

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>
PN532_I2C pn532_i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532_i2c);
String tagId = "None";
byte nuidPICC[4];

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

String database_path;
String parent_path;

String location_path = "/vending_location";
String tid_path = "/transaction_id";
String item_path = "/items_purchased";
String price_path = "/price";
String time_path = "/transaction_date";

int epoch_time;
FirebaseJson json;
const char* ntpServer = "pool.ntp.org";

#include <Keypad.h>
#include <map>

std::map<String, float> myItems;
std::map<String, String> NFCusers;
std::map<String, Servo> itemServo;

#define ROW_NUM     4 // four rows
#define COLUMN_NUM  4 // four columns

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'2', '5', '6', 'B'},
  {'3', '8', '9', 'C'},
  {'4', '0', '#', 'D'}
};

byte pin_rows[ROW_NUM]      = {19,18,5,17}; // GIOP19, GIOP18, GIOP5, GIOP17 connect to the row pins
byte pin_column[COLUMN_NUM] = {16,4,2,15};   // GIOP16, GIOP4, GIOP0, GIOP2 connect to the column pins

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

#define WIFI_SSID "..............."
#define WIFI_PASSWORD "............"

// Insert Firebase project API Key
#define API_KEY "......................."

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "......................" 

FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;
int intValue;
float floatValue;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

String readNFC() 
{
 if (nfc.tagPresent())
 {
   NfcTag tag = nfc.read();
   tag.print();
   tagId = tag.getUidString();
   return tagId;
 }
 
 delay(1000);
}

unsigned long Get_Epoch_Time() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

void WriteDataFirebase(String item, float newBalance,String firebase_id, Servo myservo){
  static unsigned long sendDataPrevMillisWrite = 0; // separate variable for WriteDataFirebase
  String uid= firebase_id;
  rotate_servo(myservo);
   
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillisWrite > 15000 || sendDataPrevMillisWrite == 0)){
    sendDataPrevMillisWrite = millis();
    
    
    if (Firebase.RTDB.setFloat(&fbdo, ("/users/"+uid+"/balance/").c_str() ,float(newBalance) )){
      Serial.println("PASSED");
      Serial.println("PATH: " + fbdo.dataPath());
      Serial.println("TYPE: " + fbdo.dataType());
    }
    else {
      Serial.println("FAILED");
      Serial.println("REASON: " + fbdo.errorReason());
    }

     epoch_time = Get_Epoch_Time();
    database_path =  "/users/" + String(uid)+ String("/transactions");
    Serial.print ("time: ");
    Serial.println (epoch_time);

    parent_path= database_path + "/" +String(epoch_time);

    json.set(location_path.c_str(), String("KEC Kalimati"));
    json.set(tid_path.c_str(), String(uid));
    json.set(item_path.c_str(), String(item));
    json.set(price_path.c_str(), String(myItems[item]));
    json.set(time_path, String(epoch_time));

  Serial.printf("Set json...%s\n",Firebase.RTDB.setJSON(&fbdo, parent_path.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
}
}

float ReadData(String firebase_id){
  static unsigned long sendDataPrevMillisRead = 0; // separate variable for ReadData
  String uid= firebase_id;
    Serial.println(uid);
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillisRead > 15000 || sendDataPrevMillisRead == 0)) {
    sendDataPrevMillisRead = millis();
    
    if (Firebase.RTDB.getFloat(&fbdo, ("/users/"+uid+"/balance/").c_str())) {
      if (fbdo.dataType() == "float") {
        return floatValue = fbdo.floatData();
      }
    }
    else {
       return floatValue = 1000.0;
      Serial.println(fbdo.errorReason());
    }
  }
  return floatValue; // return the value outside the if block
}

void rotate_servo(Servo myservo){
 int pos=0;
  for (pos = 0; pos <= 180; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);    // tell servo to go to position in variable 'pos'
    delay(15);             // waits 15ms for the servo to reach the position
  }
 
  }

void setup(){
  Serial.begin(115200);
   configTime(0, 0, ntpServer);
  myItems["Dettol"] = 50.0;
  myItems["Sanitizer"] = 60.0;
  myItems["kitkat"] = 100.0;
  myItems["dairymilk"] = 150.0;
  NFCusers["13 37 81 13"]="wMfLuU4BBWMNnnQEfRL5l7rILPH3"; //white tag
  NFCusers["23 24 D0 12"]="e4DIoclJ2HZHx6CopkMt3wWPIBu1"; //blue tag
  itemServo["Dettol"]= myservo1;
  itemServo["Sanitizer"]= myservo2;
  itemServo["Kitkat"]= myservo3;
  itemServo["dairymilk"]= myservo4;
  //database_path =  "/users/" + String("wMfLuU4BBWMNnnQEfRL5l7rILPH3")+ String("/transactions");
  
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo1.setPeriodHertz(50);    // standard 50 hz servo
  myservo1.attach(12, 500, 2400);
  myservo2.setPeriodHertz(50);    // standard 50 hz servo
  myservo2.attach(13, 500, 2400);

  myservo3.setPeriodHertz(50);    // standard 50 hz servo
  myservo3.attach(14, 500, 2400);

  myservo4.setPeriodHertz(50);    // standard 50 hz servo
  myservo4.attach(27, 500, 2400);
  
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  nfc.begin();
}

void loop(){
  char key = keypad.getKey();
  float balance;
  String NFCid="";
  String Item_chosen="";
  Servo mot;
  
  if (key) {
    Serial.println(key);
    switch (key) {
    case '1':  
      Serial.println("Dettol chosen");
      Item_chosen="Dettol";
      mot= myservo1;
      break;
    case '2':  
      Serial.println("Sanitizer chosen");
      Item_chosen="Sanitizer";
      mot= myservo2;
      break;
    case '3':  
      Serial.println("kitkat chosen");
      Item_chosen="kitkat";
      mot= myservo3;
      break;
    case '4':  
      Serial.println("dairy milk");
      Item_chosen="dairymilk";
      mot= myservo4;
      break;
    default:
      Serial.print("invalid input");
}

      NFCid= readNFC();
      Serial.println(NFCid);
      balance = ReadData(NFCusers[NFCid]);
      Serial.println(NFCusers[NFCid]);
      balance= balance- myItems[Item_chosen];
      Serial.println(balance);
      WriteDataFirebase(Item_chosen,balance,NFCusers[NFCid],mot) ;
      delay(1000);
  }
}
