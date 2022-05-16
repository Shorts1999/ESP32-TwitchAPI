#ifndef __TWITCHAPI_H
#define __TWITCHAPI_H

#define CORE_DEBUG_LEVEL 4
#include <Arduino.h>
#include <WiFi.h>
// Includes for the server
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>

#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <Preferences.h>


class TwitchAPI {
public:
    //constructor:
    TwitchAPI(const char *dnsName);
    //Destructor:
    ~TwitchAPI();


    //Public members:

    //Public Methods:
    void loopserver();


private:
    //Private members:
    Preferences apiNvs;                 //Non-volatile storage section
    httpsserver::SSLCert *certificate;  //self-signed certificate

    httpsserver::HTTPSServer *server;   //HTTPS server
    //Server nodes:
    httpsserver::ResourceNode *nodeRoot;
    httpsserver::ResourceNode *nodeNotFound;

    //Private methods:
};

#endif //#ifndef __TWITCHAPI_H