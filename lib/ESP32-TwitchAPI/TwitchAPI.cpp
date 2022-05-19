#include <TwitchAPI.h>


const char *TAG = "TwitchAPI";

static String HTMLPAGE = "<!DOCTYPE html>"
"<html>"
"<script>"
"function checkToken(){"
"let oauth=location.hash.substring(location.hash.indexOf(\"access_token\")).split('&')[0].split('=')[1];"
"if(oauth!=undefined){let xhr=new XMLHttpRequest(); xhr.open('POST','/authtoken'); xhr.send(oauth);}}"
"</script>"
"<body onload=\"checkToken()\">"
"<a href=\"https://id.twitch.tv/oauth2/authorize?"
"response_type=token"
"&client_id=idPlaceholder"
"&redirect_uri=https://dnsPlaceholder.local/"
"&scope=chat:read+chat:edit channel:moderate channel:read:subscriptions whispers:read whispers:edit channel_editor\">"
"CLICK HERE TO LOG IN</a></body>"
"</html>";

TwitchAPI::TwitchAPI() {

}

void TwitchAPI::begin(const char *dnsName, const char *clientId) {
    mClientId = String(clientId);

    //Is there a better way to do this?
    HTMLPAGE.replace("idPlaceholder", clientId);
    HTMLPAGE.replace("dnsPlaceholder", dnsName);

    //Create an MDNS:
    log_d("Created TwitchAPI with DNS %s", dnsName);
    if (MDNS.begin(dnsName)) {
        MDNS.addService("http", "tcp", 80) ? : Serial.println("error adding http 80");
        MDNS.addService("https", "tcp", 443) ? : Serial.println("error adding https tcp 443");
        log_i("MDNS responder started");
        log_i("Can now connect to %s.local/", dnsName);
    }
    certificate = new httpsserver::SSLCert();
    //Check if a certificate exists:
    apiNvs.begin("dnsName");
    size_t certSize = apiNvs.getBytesLength(certificateKey);
    size_t pkSize = apiNvs.getBytesLength(privateKeyKey);
    if (certSize != 0 && pkSize != 0) { //if non-zero, a certificate exists in NVS:
        uint8_t *certBuffer = new uint8_t[certSize + 1];
        uint8_t *keyBuffer = new uint8_t[pkSize + 1];

        size_t certSizeResult = apiNvs.getBytes(certificateKey, certBuffer, apiNvs.getBytesLength(certificateKey) + 1);
        log_d("Certificate length read from NVS: %i", certSizeResult);
        certificate->setCert(certBuffer, certSize);

        size_t pkSizeResult = apiNvs.getBytes(privateKeyKey, keyBuffer, apiNvs.getBytesLength(privateKeyKey) + 1);
        log_d("Key length read from NVS: %i", pkSizeResult);
        certificate->setPK(keyBuffer, pkSize);
    }
    else { //Generate a new self-signed certificate
        log_i("No certificate found, creating a new one!");
        std::string certName;
        certName.append("CN=");
        certName.append(dnsName);
        certName.append(".local,O=shungeryunger,C=NL");
        int createCertResult = httpsserver::createSelfSignedCert(
            *certificate,
            httpsserver::KEYSIZE_2048,
            certName,
            "20200101000000",
            "20300101000000"
        );
        if (createCertResult != 0) {
            log_e("Create certificate threw error 0x%02X !", createCertResult);
            while (1) delay(500);    //Can not continue
        }
        log_i("Created certificate!");
        //Store in NVS:
        size_t certNvsResult = apiNvs.putBytes(certificateKey, certificate->getCertData(), certificate->getCertLength() + 1);
        size_t pkNvsResult = apiNvs.putBytes(privateKeyKey, certificate->getPKData(), certificate->getPKLength() + 1);
        log_d("Wrote Certificate with length %i and PK with length %i to NVS!", certNvsResult, pkNvsResult);
    }
    log_d("Finished init TwitchAPI");

}

TwitchAPI::~TwitchAPI() {
    delete certificate;
}

void TwitchAPI::loopserver() {
    server->loop();
}

TwitchAPI &TwitchAPI::initialise() {
    if (!isInitialised) {
        static TwitchAPI twitchAPIref = *(new TwitchAPI());
        isInitialised = true;
        return twitchAPIref;
    }
    //Return the already existing static reference:
    return TwitchApi;
}

void TwitchAPI::runAuthenticator() {
    TwitchApi.authenticatorRunning = true;
    //Setup the server and attach the nodes:
    nodeRoot = new httpsserver::ResourceNode("/", "GET",
        [](httpsserver::HTTPRequest *request, httpsserver::HTTPResponse *response) {
        // Status code is 200 OK by default.
        // We want to deliver a simple HTML page, so we send a corresponding content type:
        response->setHeader("Content-Type", "text/html");
        response->print(HTMLPAGE);
    });

    nodeNotFound = new httpsserver::ResourceNode("/", "GET",
        [](httpsserver::HTTPRequest *request, httpsserver::HTTPResponse *response) {
        // Discard request body, if we received any
        // We do this, as this is the default node and may also server POST/PUT requests
        request->discardRequestBody();

        // Set the response status
        response->setStatusCode(404);
        response->setStatusText("Not Found");
    });

    receiveOauth = new httpsserver::ResourceNode("/authtoken", "POST", [](httpsserver::HTTPRequest *request, httpsserver::HTTPResponse *response) {
        size_t requestLength = request->getContentLength();
        char requestBuffer[requestLength + 1];//add one for null terminator
        request->readChars(requestBuffer, requestLength);
        requestBuffer[requestLength] = '\0'; //manually add null terminator to end string
        log_d("received token: %s", requestBuffer);
        TwitchApi.mOauth = requestBuffer;
        //Write Oauth key to the NVS:
        log_d("oauth key: %s", TwitchApi.oauthKey);
        apiNvs.putString(TwitchApi.oauthKey, requestBuffer);
        log_d("Oauth: moauth: %s nvsOauth: %s", TwitchApi.mOauth, apiNvs.getString(TwitchApi.oauthKey).c_str());
        //Restart the device with the newly saved credential:
        esp_restart();
    });


    log_d("Creating server");
    server = new httpsserver::HTTPSServer(certificate);
    log_d("Registering node:");
    server->registerNode(nodeRoot);
    server->registerNode(receiveOauth);
    server->setDefaultNode(nodeNotFound);
    log_d("Starting server");
    server->start();
    log_d("Finished starting server");
}

bool TwitchAPI::checkAuthentication() {
    String oauthToken = apiNvs.getString(TwitchApi.oauthKey);
    if (oauthToken.length() > 0) {
        return true;
    }
    return false;
}

String TwitchAPI::getAuthToken() {
    return apiNvs.getString(TwitchApi.oauthKey);
}

int TwitchAPI::getFollowerCount(String toUserId) {
    log_d("Getting follower count");
    HTTPClient followerRequestClient;
    String url;
    url = "https://api.twitch.tv/helix/users/follows?to_id=" + toUserId + "&first=1";
    followerRequestClient.begin(url);
    followerRequestClient.addHeader("Authorization", "Bearer " + TwitchApi.getAuthToken());
    followerRequestClient.addHeader("Client-Id", mClientId);

    int response = followerRequestClient.GET();

    if (response != 200) return -1;
    DynamicJsonDocument payloadJson(256);
    StaticJsonDocument<64> JsonFilter;
    JsonFilter["total"] = true;
    deserializeJson(payloadJson, followerRequestClient.getString(), DeserializationOption::Filter(JsonFilter));

    log_d("Follower count: %d", payloadJson["total"].as<int>());

    followerRequestClient.end();
    return payloadJson["total"].as<int>();
}

int TwitchAPI::getSubscriberCount(String toBroadcasterId) {
    log_d("Getting sub count");
    HTTPClient followerRequestClient;
    String url;
    url = "https://api.twitch.tv/helix/subscriptions?broadcaster_id=" + toBroadcasterId + "&first=1";
    followerRequestClient.begin(url);
    followerRequestClient.addHeader("Authorization", "Bearer " + TwitchApi.getAuthToken());
    followerRequestClient.addHeader("Client-Id", mClientId);

    int response = followerRequestClient.GET();

    if (response != 200) return -1; //If connect failed
    DynamicJsonDocument payloadJson(256);
    StaticJsonDocument<64> JsonFilter;
    JsonFilter["total"] = true;
    deserializeJson(payloadJson, followerRequestClient.getString(), DeserializationOption::Filter(JsonFilter));
    log_d("Subscriber count: %i", payloadJson["total"].as<int>());

    followerRequestClient.end();
    return payloadJson["total"].as<int>();
}

TwitchAPI::userData TwitchAPI::fetchUserData(String username) {
    log_d("Fetching user data");
    HTTPClient userRequestClient;
    //make synchronous, we need to wait for this info to display on page
    //request to helix/users without supplied name/ID will fetch user info of the supplied bearer
    String putInUsername = username == "" ? "" : "?login=" +username;
    userRequestClient.begin("https://api.twitch.tv/helix/users" + putInUsername);
    userRequestClient.addHeader("Authorization", "Bearer " + TwitchApi.getAuthToken());
    userRequestClient.addHeader("Client-Id", "8460mbnko5p0n4e0fftgvvfaxf3fzt");

    int response = userRequestClient.GET();

    if (response != 200) return TwitchAPI::userData{.userId="", .userName=username};
    if (userRequestClient.getSize() > 1024) {
        log_e("Payload too large!");
        log_d("Payload size was %i", userRequestClient.getSize());
        userRequestClient.end();
        return TwitchAPI::userData{.userId="", .userName=username};
    }
    DynamicJsonDocument payloadJson(512);
    StaticJsonDocument<128> JsonFilter;
    JsonFilter["data"][0]["id"] = true;
    JsonFilter["data"][0]["display_name"] = true;
    deserializeJson(payloadJson, userRequestClient.getString(), DeserializationOption::Filter(JsonFilter));

    String lUserId = payloadJson["data"][0]["id"];
    String lDisplayName = payloadJson["data"][0]["display_name"];
    log_d("User id: %s, displayname: %s", lUserId.c_str(), lDisplayName.c_str());
    log_d("Stored user Id: %s", TwitchApi.userId.c_str());
    userRequestClient.end();


    if (username == "") {
        TwitchApi.userId = lUserId;
        TwitchApi.username = lDisplayName;
    }
    userData lUserData = { .userId = lUserId, .userName = lDisplayName };
    return lUserData;
}

//Static members:
bool TwitchAPI::authenticatorRunning = false;
String TwitchAPI::userId = "";
String TwitchAPI::username = "";
String TwitchAPI::mClientId = "";

#undef DEBUG_PRINT