 #include <ArduinoJson.h>
#include <esp_now.h>
#include <WiFi.h>
#include "config_wifi.h"
#include "EOTAUpdate.h"
#include <HardwareSerial.h>

  #define EEPROM_SSID 0
  #define EEPROM_PASS 100
  #define EEPROM_ID 200

//Config_OTA
//const char * const   VERSION_STRING = "0.1";
  const unsigned short VERSION_NUMBER = 1;
  const char * const   UPDATE_URL     = "http://morning-falls-78321.herokuapp.com/update.txt";
//  const char * const   UPDATE_URL     = "http://0db0f6a29cd7.ngrok.io/update.txt";
  EOTAUpdate updater(UPDATE_URL, VERSION_NUMBER);

  //Config_wifi
  Config_wifi wifi;
  
// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[6];

int checkSendcount=0;
int checkSendTime=15000;
int checkSendTimeNow;
int checkSuccess=0;

int withTimeForSuccess=200;
int withTimeForSuccessNow=0;

int timeSinceRecivedMassageFromPlant=0;
int timeSinceRecivedMassageFromPlantChect=4000;

bool reciveMassageFromMaster=false;

//struct of veribales that are sent to plant
typedef struct sentDataStruct{ 
  int task;//the tesk to do
  String plantIdNumber;
  int motorCurrentSub;
  bool motorState=false;
  bool autoIrrigateState=false;
  int irrigatePlantOption;
  String UPDATE_URL;
  unsigned short versuionNumber;
  String ssid;
  String pass;
 } sentDataStruct; 
sentDataStruct sentData;
 
//struct of veribales that are received from plant
typedef struct receiveDataStruct{ 
  int task;
  String plantIdNumber;
  int batteryStatus;
  int moistureStatus;
  int lightStatus;
  bool motorState;
  bool waterState;
  bool autoIrrigateState;
  bool irrigatePlantWorking;
  unsigned short VERSION_NUMBER;
  bool massgeSuccess;
  bool wifiWorked;
} receiveDataStruct;
receiveDataStruct receiveData;

//functions
//void batteryStatus();
//void checkUpdateProgrem();

void EspNowRegusterPeer();// register peer
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void onReceiveData(const uint8_t * mac, const uint8_t *dataIncom, int len);
void sendTask();
void swithSendTaskPlant(const JsonDocument& local_doc);
void updateSendMACAddress(String MacAddressSring);
void swithTaskReturnMaster( int taskReceive);


void setup() {
    Serial.begin(115200);
   Serial2.begin(4500000);   
    
   delay(1000);
      WiFi.mode(WIFI_STA);
     Serial.println("noooooooo");

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
   esp_now_register_send_cb(OnDataSent);
   esp_now_register_recv_cb(onReceiveData);

}

void loop() {
  boolean messageReady = false;
  String message = "";
    if(Serial2.available()) {
      message = Serial2.readString();
      messageReady = true;
   //  Serial.println(" i have a massage" ); 
    }
  if(messageReady){
    DynamicJsonDocument doc(1024);  
    // Attempt to deserialize the JSON-formatted message
    DeserializationError error = deserializeJson(doc,message);
    if(error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.c_str());
      return;
    }
     reciveMassageFromMaster=true;     
     swithSendTaskPlant(doc);
  }
  
    if((millis()-timeSinceRecivedMassageFromPlant>timeSinceRecivedMassageFromPlantChect)&&(checkSuccess==0)&&(reciveMassageFromMaster==true)){
      //Serial.println("time massage sent:" +(String)(timeSinceRecivedMassageFromPlant));//delete before prduction  
      checkSuccess=1;
    }
    

    if ((checkSuccess==1)&&(millis()-checkSendTimeNow<checkSendTime)&&(millis()-withTimeForSuccessNow>withTimeForSuccess)){
       // Serial.println("time left:" +(String)(millis()-checkSendTimeNow));//delete before prduction  
        Serial.println("Error sending the data, trying agin");//delete before prduction
        withTimeForSuccessNow=millis();
        sendTask();
  }

}

void swithSendTaskPlant(const JsonDocument& local_doc){//task racive from Ruter send to the plant
  sentData.task=local_doc["task"];  
  sentData.plantIdNumber=local_doc["productCatNumber"].as<String>();
  updateSendMACAddress(local_doc["macAddress"]) ; 
  Serial.println("task number "+(String)sentData.task);
  Serial.println("product numbeer"+sentData.plantIdNumber);
  switch(sentData.task) {
    case 1:  
       sentData.motorCurrentSub=local_doc["motorCurrentSub"];
      break;
    case 2:
        sentData.irrigatePlantOption=local_doc["irrigatePlantOption"];
        sentData.motorCurrentSub=local_doc["motorCurrentSub"];
      break;
    case 3:
      break;
    case 4:
        sentData.autoIrrigateState = local_doc["autoIrrigateState"];
        sentData.motorCurrentSub=local_doc["motorCurrentSub"];
      break;
    case 5:
//      batteryStatus();
     break;
    case 6://motor controle product
      sentData.motorState= local_doc["motorState"];
      sentData.motorCurrentSub=local_doc["motorCurrentSub"];
    break;
    case 8:
//      checkUpdateProgrem();
      sentData.UPDATE_URL=local_doc["UPDATE_URL"].as<String>();
      sentData.versuionNumber=local_doc["VERSION_NUMBER"].as<unsigned short>();
      sentData.ssid=local_doc["ssid"].as<String>();
      sentData.pass=local_doc["pass"].as<String>();
    break;
    case 11://checkUpdatePlantProgrem
        if (esp_now_deinit() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
      }
      
       wifi.writeStringEEPROM(EEPROM_SSID,wifi.urldecode(local_doc["ssid"].as<String>()));
       wifi.writeStringEEPROM(EEPROM_PASS,wifi.urldecode(local_doc["pass"].as<String>()));
        WiFi.mode(WIFI_STA);//WIFI_MODE_STA
        String ssidD=wifi.urldecode(wifi.readStringEEPROM(EEPROM_SSID));
        String passD=wifi.urldecode(wifi.readStringEEPROM(EEPROM_PASS));
        char ssidA[100];
        char passA[100];
        ssidD.toCharArray(ssidA, 99);
        passD.toCharArray(passA, 99);
        Serial.println("Trying to connect to " + ssidD + " with passwd:" + passD);  
        WiFi.begin(ssidA, passA); 
        delay(1000);   
        int i = 100;
        while (i-- > 0 && WiFi.status() != WL_CONNECTED)
        {
            delay(50);
            Serial.print(".");//delete before prduction
        }
        if (WiFi.status() != WL_CONNECTED)
        {      
          Serial.println("Connection failed");//delete before prduction 
          receiveData.task=11;
          receiveData.massgeSuccess=false;
          receiveData.VERSION_NUMBER=VERSION_NUMBER;
        }
        else if (WiFi.status() == WL_CONNECTED)
        {
          Serial.println("WiFi connected");//delete before prduction
          Serial.println("Checking for updates. Remember to set the URL!");//delete before prduction
    //      updater.CheckAndUpdate();
          delay(30);
          } 
            if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
      }
         swithTaskReturnMaster(11);
      break;
  
      }     
  if((sentData.task !=10)&&(sentData.task !=11)){
  checkSendTimeNow=millis();
  withTimeForSuccessNow=millis();
  checkSuccess=1;
  sendTask();   
  }  
}
void updateSendMACAddress(String MacAddressSring){
  Serial.println("this is my mac address"+MacAddressSring);
   char MacAddressChar[18];
   MacAddressSring.toCharArray(MacAddressChar, 18);
     char* ptr;
     broadcastAddress[0] = strtol( strtok(MacAddressChar,":"), &ptr, HEX );
  for( uint8_t i = 1; i < 6; i++ )
  {
    broadcastAddress[i] = strtol( strtok( NULL,":"), &ptr, HEX );
  }
}
// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  char macStr[18];
  //Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  //Serial.println(macStr);
 // Serial.println(" send status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  switch (status){
  case ESP_NOW_SEND_SUCCESS:
      checkSuccess=0;
      Serial.println(" send SUCCESS");
      //delay(1000);
      timeSinceRecivedMassageFromPlant=millis();
   break;
  case ESP_NOW_SEND_FAIL:
    if (millis()-checkSendTimeNow>=checkSendTime){
     Serial.println("didn't secced");
     checkSuccess=0;
   } 
   break;
   
   default:
      break;
  }
}
void sendTask(){
  EspNowRegusterPeer();  
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&sentData, sizeof(sentData));
  if (result == ESP_OK)
  {
    Serial.println("Sent with success");//delete before prduction
  }
  else
  {
    Serial.println("Error sending the data");//delete before prduction
  } 
}
void EspNowRegusterPeer(){
  esp_now_peer_info_t peerInfo= {};
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // register first peer  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  } 
}
// callback function that will be executed when data is received
void onReceiveData(const uint8_t * mac, const uint8_t *dataIncom, int len) { 
  checkSuccess=0;
  reciveMassageFromMaster=false;
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  //Serial.println(macStr);
  memcpy(&receiveData, dataIncom, sizeof(receiveData));
 // Serial.print("Packet received size: ");
  //Serial.println(sizeof(receiveData));
  Serial.println(receiveData.task);
  swithTaskReturnMaster(receiveData.task);
}

void swithTaskReturnMaster( int taskReceive){//task that are send to the Router 
      StaticJsonDocument<200> doc1;
      doc1["task"]=(String)receiveData.task;
     // Serial.println(receiveData.task);
      //Serial.println(receiveData.plantIdNumber);
      doc1["productCatNumber"]=(String)receiveData.plantIdNumber;
  switch(taskReceive) {
    case 1://plantInitialization
      doc1["massgeSuccess"]=(String)receiveData.massgeSuccess;
      Serial.println(receiveData.massgeSuccess); 
      Serial.println(receiveData.plantIdNumber); 
    break;
    case 2://reciving sensor data   
      break;
    case 3://reciving sensor data
      doc1["moistureStatus"]=receiveData.moistureStatus;
      doc1["lightStatus"]=receiveData.lightStatus;
      Serial.println("humidity Status:"+ (String)receiveData.moistureStatus );
      Serial.println("humidity Status:"+ (String)receiveData.lightStatus );  
      break;
    case 4://reciving motor status 
        doc1["motorState"]=receiveData.motorState;
        doc1["irrigatePlantWorking"]=receiveData.irrigatePlantWorking;
        doc1["autoIrrigateState"]=receiveData.autoIrrigateState;
        doc1["waterState"]=receiveData.waterState;
        Serial.println("motor state:"+ (String)receiveData.motorState);        
        //Serial.println("irrigatePlantWorking state:"+ (String)receiveData.irrigatePlantWorking); 
        //Serial.println("autoIrrigateState state:"+ (String)receiveData.autoIrrigateState); 
        //Serial.println("waterState state:"+ (String)receiveData.waterState); 
      break;
    case 8://update recive and updateing or not 
     doc1["massgeSuccess"]=receiveData.massgeSuccess;
     doc1["VERSION_NUMBER"]=receiveData.VERSION_NUMBER; 
     doc1["wifiWorked"]=receiveData.wifiWorked;  
    break;
    case 9://update didn't secseed 
      //cheack version
    break;
     case 10://massage racived
      Serial.println("massage racived");
      doc1["massgeSuccess"]=receiveData.massgeSuccess;
     break;
     case 11://massage racived
      Serial.println("massage racived");
      doc1["task"]=11;
      doc1["type"]="slave";
      doc1["VERSION_NUMBER"]=VERSION_NUMBER;
      doc1["massgeSuccess"]=receiveData.massgeSuccess;
     break;
}
    if(receiveData.task !=10){
    serializeJson(doc1,Serial2);
    Serial.println("send tesk to master" ); 
    }
    
}
