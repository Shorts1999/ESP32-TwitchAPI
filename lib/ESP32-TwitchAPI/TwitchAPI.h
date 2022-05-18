#ifndef __TWITCHAPI_H
#define __TWITCHAPI_H

#define CORE_DEBUG_LEVEL 4
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
// Includes for the server
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <WebsocketHandler.hpp>

#include <Preferences.h>


class TwitchAPI {
public:
    //constructor:
    TwitchAPI(const char *dnsName, const char *clientID);
    //Destructor:
    ~TwitchAPI();


    //Public members:

    //Public Methods:
    void loopserver();


private:
    //Private members:
    char* mClientId;
    Preferences apiNvs;                 //Non-volatile storage section
    httpsserver::SSLCert *certificate;  //self-signed certificate

    httpsserver::HTTPSServer *server;   //HTTPS server
    //Server nodes:
    httpsserver::ResourceNode *nodeRoot;
    httpsserver::ResourceNode *nodeNotFound;
    httpsserver::ResourceNode *receiveOauth;

    //Private methods:
};

#endif //#ifndef __TWITCHAPI_H