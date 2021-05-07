
  #ifndef config_IOT_H
  #define config_IOT_H
  
  #include <WiFi.h> 
  #include <WiFiClient.h>
  #include <WiFiAP.h>
  #include <EEPROM.h>
  #include <Arduino.h>

  
  class Config_wifi{
    protected:
    //new wifi configuration
    String passSdecode="";
    String ssidSdecode= "";
    String savedSSID = "";
    String savedPASS = "";
    String ipaddress = "";
    String networks = "";
    int i=0;
    bool connected = false;

    char* ID;
    
    public:
    void wifiSetupNew(); 
    void configAP();
    void saveSsidPass(String ssid,String pass);
    bool connectWifi(String ssid, String pass);
    String readStringEEPROM(char add);
    void writeStringEEPROM(char add, String data);
    String readStringIDEEPROM(char add);
    String urldecode(String str);
    unsigned char h2int(char c);
  };
  #endif
