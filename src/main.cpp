#define CORE_DEBUG_LEVEL 4
#include <Arduino.h>
#include <Preferences.h>

#include <TwitchAPI.h>

const char* myClientId = "8460mbnko5p0n4e0fftgvvfaxf3fzt";
const char* espDns = "esptwitchbot";

void setup() {
    Serial.begin(115200);
    Preferences wifiNVS;
    wifiNVS.begin("WiFiCred");
    String SSID = wifiNVS.getString("SSID", String(""));
    String password = wifiNVS.getString("PWD", String(""));
    if (SSID == "" || password == "") {
        log_w("Could not find SSID/PWD, please write new credentials to NVS");
        while (1) {}
    }
    log_i("Found SSID %s", SSID.c_str());

    WiFi.begin(SSID.c_str(), password.c_str());

    Serial.print("[WIFI] Connecting..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(200);
    }
    Serial.print("\n");

   TwitchApi.begin(espDns, myClientId);

   if(!TwitchApi.checkAuthentication()){
       log_d("Could not find an auth key, running authenticator!");
       TwitchApi.runAuthenticator();
       //If no token is found, run the authenticator:
       while(true){
           TwitchApi.loopserver();
       }
   }
   TwitchAPI::userData userdata =TwitchApi.fetchUserData();
   int FollowCount = TwitchApi.getFollowerCount(userdata.userId);
   int SubCount = TwitchApi.getSubscriberCount(userdata.userId); //This should not be allowed if the userId is not the oauth token owner

   Serial.printf("\n\n User %s has %i followers and %i subscribers!", userdata.userName, FollowCount, SubCount);
    
}
void loop() {
}

#undef DEBUG_PRINT