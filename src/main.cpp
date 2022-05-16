#define CORE_DEBUG_LEVEL 4
#include <Arduino.h>
#include <Preferences.h>

#include <TwitchAPI.h>

TwitchAPI *twitchapi;

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

    twitchapi = new TwitchAPI("ESPTwitchBot");
}
void loop() {
    twitchapi->loopserver();

}

#undef DEBUG_PRINT