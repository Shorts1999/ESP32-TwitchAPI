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

// Includes for making HTTP request
#include <HTTPClient.h>

// Non-volatile storage
#include <Preferences.h>

class TwitchAPI {
public:
    //Destructor:
    ~TwitchAPI();

    typedef struct userData{
        String userId;
        String userName;
    }userData;

    //Public members:
    static bool authenticatorRunning;
    static String userId;
    static String username;

    //Public Methods:
    static TwitchAPI &initialise();
    void begin(const char *dnsName, const char *clientID);
    void loopserver();
    bool checkAuthentication();
    String getAuthToken();
    void runAuthenticator();
    httpsserver::HTTPSServer *server;   //HTTPS server
    userData fetchUserData(String username="");

    int getFollowerCount(String toUserId);
    int getSubscriberCount(String toBroadcasterId);

private:
    //constructor:
    TwitchAPI();
    //Private members:
    static String mClientId;
    char *mOauth;

    //NVS keys:
    const char* certificateKey = "apiCertificate";
    const char* privateKeyKey = "apiPk";
    const char* oauthKey = "apiToken";

    httpsserver::SSLCert *certificate;  //self-signed certificate

    //Server nodes:
    httpsserver::ResourceNode *nodeRoot;
    httpsserver::ResourceNode *nodeNotFound;
    httpsserver::ResourceNode *receiveOauth;

    //Private methods:
};

static bool isInitialised = false;
static Preferences apiNvs;          //Non-volatile storage section
static TwitchAPI TwitchApi = TwitchAPI::initialise();

#endif //#ifndef __TWITCHAPI_H