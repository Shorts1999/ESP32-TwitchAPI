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
   TwitchApi.fetchUserData();
   int FollowCount = TwitchApi.getFollowerCount(TwitchApi.userId);
   int SubCount = TwitchApi.getSubscriberCount(TwitchApi.userId);

   Serial.printf("\n\n User %s has %i followers and %i subscribers!", TwitchApi.username.c_str(), FollowCount, SubCount);
    
}
void loop() {
    // TwitchApi.loopserver();

}

#undef DEBUG_PRINT