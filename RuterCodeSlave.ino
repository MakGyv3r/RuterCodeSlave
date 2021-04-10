#include <ArduinoJson.h>
#include <esp_now.h>
#include <WiFi.h>
#include <HardwareSerial.h>

// REPLACE WITH THE RECEIVER'S MAC Address
String bAddress="24:6F:28:AD:F2:0C";
uint8_t broadcastAddress[] = {0xFC, 0xF5, 0xC4, 0x31, 0xA4, 0x7C};//FC:F5:C4:31:A4:7C
//uint8_t broadcastAddress[] = {0x24, 0x6F, 0x28, 0xB2, 0x0D, 0x34};//24:6F:28:B2:0D:34

//int motorCurrentSub=100;
//int irrigatePlantOption=1;
//bool checkMessageReceive=false;

int checkSendcount=0;
int checkSendTime=15000;
int checkSendTimeNow;
int checkSuccess=0;

int withTimeForSuccess=200;
int withTimeForSuccessNow=0;

bool checkRecive=false;

//struct of veribales that are sent to plant
typedef struct sentDataStruct{ 
  int task;//the tesk to do
  String plantIdNumber;
  int motorCurrentSub;
  bool motorState=false;
  bool autoIrrigateState=false;
  int irrigatePlantOption;
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
  bool massgeSuccess;
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
void swithTaskReturnMaster( int taskReceive);


void setup() {
    Serial.begin(250000);
   Serial2.begin(5000000);   
    
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
     Serial.println(" i have a massage" ); 
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
           swithSendTaskPlant(doc);  
  }

    if ((checkSuccess==1)&&(millis()-checkSendTimeNow<checkSendTime)&&(millis()-withTimeForSuccessNow>withTimeForSuccess)){
    Serial.println("time left:" +(String)(millis()-checkSendTimeNow));//delete before prduction  
    Serial.println("Error sending the data, trying agin");//delete before prduction
    withTimeForSuccessNow=millis();
    sendTask();
  }

}

void swithSendTaskPlant(const JsonDocument& local_doc){//task racive from Ruter send to the plant
  sentData.task=local_doc["task"];  
  Serial.println("task number "+(String)sentData.task);
  switch(sentData.task) {
    case 1:    
       sentData.motorCurrentSub=local_doc["motorCurrentSub"];
       sentData.plantIdNumber=local_doc["plantIdNumber"].as<String>();
       Serial.println("product numbeer"+(String)sentData.plantIdNumber);
      break;
    case 2:
        sentData.irrigatePlantOption=local_doc["irrigatePlantOption"];
        sentData.motorCurrentSub=local_doc["motorCurrentSub"];
        sentData.plantIdNumber=local_doc["plantIdNumber"].as<String>();
        Serial.println("product numbeer"+(String)sentData.plantIdNumber);
      break;
    case 3:
      break;
    case 4:
        sentData.autoIrrigateState = local_doc["autoIrrigateState"];
        sentData.motorCurrentSub=local_doc["motorCurrentSub"];
        Serial.println("product numbeer"+sentData.plantIdNumber);
        Serial.println("auto Irrigate State"+(String)sentData.autoIrrigateState);//delete before prduction
      break;
    case 5:
//      batteryStatus();
     break;
    case 6:
      sentData.motorState= local_doc["motorState"];
      sentData.motorCurrentSub=local_doc["motorCurrentSub"];
    break;
    case 8:
//      checkUpdateProgrem();
    break;
    }
    checkSendTimeNow=millis();
    withTimeForSuccessNow=millis();
    checkSuccess=1;
    sendTask();
  
      
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  char macStr[18];
  //Serial.print("Packet to: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
 // Serial.println(" send status:\t");
  //Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  switch (status){
  case ESP_NOW_SEND_SUCCESS:
  // if(millis()-checkSendTimeNow<checkSendTime){
      Serial.println(" send SUCCESS");
     // int timeSUCCESS=millis()-checkSendTimeNow;
      //Serial.println("time :"+ (String)timeSUCCESS);
    checkSuccess=0;
   //}
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
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);
  memcpy(&receiveData, dataIncom, sizeof(receiveData));
  Serial.print("Packet received size: ");
  Serial.println(sizeof(receiveData));
  Serial.println(receiveData.task);
  swithTaskReturnMaster(receiveData.task);
}

void swithTaskReturnMaster( int taskReceive){//task that are send to the Router 
      StaticJsonDocument<200> doc1;
      doc1["task"]=receiveData.task;
      Serial.println(receiveData.task);
  switch(taskReceive) {
    case 1:
//      plantInitialization
      doc1["massgeSuccess"]=receiveData.massgeSuccess;
      doc1["plantIdNumber"]=receiveData.plantIdNumber;
      Serial.println(receiveData.massgeSuccess); 
      Serial.println(receiveData.plantIdNumber); 
    break;
    case 2://reciving sensor data   
      break;
    case 3://reciving sensor data
      doc1["moistureStatus"]=receiveData.moistureStatus;
      doc1["lightStatus"]=receiveData.lightStatus;
      doc1["plantIdNumber"]=receiveData.plantIdNumber;
      Serial.println("humidity Status:"+ (String)receiveData.moistureStatus );
      Serial.println("humidity Status:"+ (String)receiveData.lightStatus );  
      break;
    case 4://reciving motor status 
        doc1["plantIdNumber"]=receiveData.plantIdNumber;
        doc1["motorState"]=receiveData.motorState;
        doc1["irrigatePlantWorking"]=receiveData.irrigatePlantWorking;
        doc1["autoIrrigateState"]=receiveData.autoIrrigateState;
        doc1["waterState"]=receiveData.waterState;
        Serial.println("motor state:"+ (String)receiveData.motorState);        
        Serial.println("irrigatePlantWorking state:"+ (String)receiveData.irrigatePlantWorking); 
        Serial.println("autoIrrigateState state:"+ (String)receiveData.autoIrrigateState); 
        Serial.println("waterState state:"+ (String)receiveData.waterState); 
      break;
    case 8:
      //checkUpdateProgrem();
    break;
}
    Serial.println("no fuck i am here" );  
    serializeJson(doc1,Serial2);
    checkSuccess=0;
}
