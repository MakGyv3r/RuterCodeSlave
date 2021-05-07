  #include "config_wifi.h"

//new wifi configuration

#define EEPROM_SIZE 300
#define EEPROM_SSID 0
#define EEPROM_PASS 100
#define EEPROM_ID 200
#define MAX_EEPROM_LEN 50 // Max length to read ssid/passwd

  void Config_wifi::wifiSetupNew() {
    // Serial.begin(115200);
     delay(50);
    Serial.println();
    Serial.println("Reading settings from EEPROM");
  delay(50);

    EEPROM.begin(EEPROM_SIZE);
    
//  WiFi.disconnect(); // forget the persistent connection to test the Configuration AP

  // waiting for connection to remembered  Wifi network
  Serial.println("Waiting for connection to WiFi");

  savedSSID = readStringEEPROM(EEPROM_SSID);
  savedPASS = readStringEEPROM(EEPROM_PASS);
  Serial.println(savedSSID);
  Serial.println(savedPASS); 
  Serial.println("press 1 to disconect from wifi");
  // Try to guess if we got saved data or random unitialized
  if (savedSSID.length() == MAX_EEPROM_LEN || savedPASS.length() == MAX_EEPROM_LEN)
  {
      Serial.println("Unitialized data from EEPROM");
      Serial.println("Setting up AP");
      connected = false;
  }
  else
  {
      Serial.println("Trying to connect to saved WiFi");
      connected = connectWifi(savedSSID, savedPASS);
  }
  WiFi.waitForConnectResult();
  
  if (WiFi.status() != WL_CONNECTED || !connected) {
    Serial.println();
    Serial.println("Could not connect to WiFi. Starting configuration AP...");
    configAP();
  } else {
    Serial.println("WiFi connected");
  }
  
}
void Config_wifi::configAP() {

  WiFiServer configWebServer(80);
  
  WiFi.mode(WIFI_AP_STA); // starts the default AP (factory default or setup as persistent)

  Serial.print("Connect your computer to the WiFi network ");
 // Serial.print(WiFi.softAP());
  Serial.println();
  IPAddress ip = WiFi.softAPIP();
  Serial.print("and enter http://");
  Serial.print(ip);
  Serial.println(" in a Web browser");

  configWebServer.begin();

    
  while (true) {

    WiFiClient client = configWebServer.available();
    if (client) {
      char line[64];
      int l = client.readBytesUntil('\n', line, sizeof(line));
      line[l] = 0;
      client.find((char*) "\r\n\r\n");
      if (strncmp_P(line, PSTR("POST"), strlen("POST")) == 0) {
        l = client.readBytes(line, sizeof(line));
        line[l] = 0;

        // parse the parameters sent by the html form
        const char* delims = "=&";
        strtok(line, delims);
        const char* ssid = strtok(NULL, delims);
        strtok(NULL, delims);
        const char* pass = strtok(NULL, delims);
      
        
        // decoding the ssid and the password for ASCII characters
        String ssidS = String(ssid);
        ssidSdecode= urldecode(ssidS);
        ssid=const_cast<char*>(ssidSdecode.c_str());
        String passS = String(pass);
        passSdecode= urldecode(passS);
        pass=const_cast<char*>(passSdecode.c_str());
        Serial.println("this is decoded:");
        Serial.println(passSdecode);
        Serial.println(ssidSdecode);
        // send a response before attemting to connect to the WiFi network
        // because it will reset the SoftAP and disconnect the client station
        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Connection: close"));
        client.println(F("Refresh: 10")); // send a request after 10 seconds
        client.println();
        client.println(F("<html><body><h3>Configuration AP</h3><br>connecting...</body></html>"));
        client.stop();

        Serial.println();
        Serial.print("Attempting to connect to WPA SSID: ");
        Serial.println(ssid);
        //connectWifi(passSdecode, ssidSdecode);
        WiFi.begin(ssid, pass);
        WiFi.waitForConnectResult();

        // configuration continues with the next request

      } else {

        client.println(F("HTTP/1.1 200 OK"));
        client.println(F("Connection: close"));
        client.println();
        client.println(F("<html><body><h3>Configuration AP</h3><br>"));

        int status = WiFi.status();
        if (status == WL_CONNECTED) {
            saveSsidPass(ssidSdecode ,passSdecode);
          client.println(F("Connection successful. Ending AP."));
        } else {
          client.println(F("<form action='/' method='POST'>WiFi connection failed. Enter valid parameters, please.<br><br>"));
          client.println(F("SSID:<br><input type='text' name='i'><br>"));
          client.println(F("Password:<br><input type='password' name='p'><br><br>"));
          client.println(F("<input type='submit' value='Submit'></form>"));
        }
        client.println(F("</body></html>"));
        client.stop();

        if (status == WL_CONNECTED) {
          delay(1000); // to let the SDK finish the communication
          Serial.println("Connection successful. Ending AP.");
          configWebServer.stop();
          saveSsidPass(ssidSdecode ,passSdecode);
          WiFi.mode(WIFI_STA);
          ESP.restart() ;
        }
      }
    }
  }
}
  void Config_wifi::saveSsidPass(String ssid,String pass){
    char ssidA[100];
    char passA[100];
    
    ssid.toCharArray(ssidA, 99);
    pass.toCharArray(passA, 99);

    if (ssid != savedSSID || pass != savedPASS)
    {
        // Wifi config has changed, write working to EEPROM
        Serial.println("Writing Wifi config to EEPROM");
        writeStringEEPROM(EEPROM_SSID, ssidA);
        writeStringEEPROM(EEPROM_PASS, passA);
    }
  
}
  bool Config_wifi::connectWifi(String ssid, String pass){
    char ssidA[100];
    char passA[100];
    
    int i = 5;

    WiFi.disconnect();

    Serial.println("Trying to connect to " + ssid + " with passwd:" + pass);
    // WiFi.begin needs char*
    ssid.toCharArray(ssidA, 99);
    pass.toCharArray(passA, 99);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssidA, passA);
   
   
    while (i-- > 0 && WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Connection failed");
        return false;
    }
    else if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi connected");
      }
    ipaddress = WiFi.localIP().toString();
    Serial.println("WiFi connected, IP address: " + ipaddress);

    if (ssid != savedSSID || pass != savedPASS)
    {
        // Wifi config has changed, write working to EEPROM
        Serial.println("Writing Wifi config to EEPROM");
        writeStringEEPROM(EEPROM_SSID, ssidA);
        writeStringEEPROM(EEPROM_PASS, passA);
    }

    return true;
}
  void Config_wifi::writeStringEEPROM(char add, String data){
    int _size = data.length();
    if (_size > MAX_EEPROM_LEN)
        return;
    int i;
    for (i = 0; i < _size; i++)
    {
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
    EEPROM.commit();
}

  String Config_wifi::readStringEEPROM(char add){
    int i;
    char data[MAX_EEPROM_LEN + 1];
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < MAX_EEPROM_LEN) //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

  String Config_wifi::urldecode(String str){
   
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}
  unsigned char Config_wifi::h2int(char c){
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
