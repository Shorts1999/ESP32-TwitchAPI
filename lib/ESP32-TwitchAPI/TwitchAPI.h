#ifndef __TWITCHAPI_H
#define __TWITCHAPI_H

#define CORE_DEBUG_LEVEL 4
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>


class TwitchAPI {
public:
    //constructor:
    TwitchAPI(const char* dnsName);

    //Public members:

    //Public Methods:
    

private:
    //Private members:
    AsyncWebServer _authenticator;
    // AsyncWebSocket _Pubsub;
    // AsyncWebSocket _Irc;

    //Private methods:
};

#endif //#ifndef __TWITCHAPI_H