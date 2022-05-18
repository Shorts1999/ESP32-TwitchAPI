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
"&scope=chat:read+chat:edit channel:moderate whispers:read whispers:edit channel_editor\">"
"CLICK HERE TO LOG IN</a></body>"
"</html>";

TwitchAPI::TwitchAPI(const char *dnsName, const char *clientId) {
    mClientId = new char[strlen(clientId) + 1]; //Add +1 for null terminator
    log_d("Allocated %d for clientId", strlen(clientId));
    strcpy(mClientId, clientId);
    log_d("Saved clientID %s", mClientId);

    //Is there a better way to do this?
    HTMLPAGE.replace("idPlaceholder", clientId);
    HTMLPAGE.replace("dnsPlaceholder", dnsName);
    log_d("%s", HTMLPAGE.c_str());

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
    size_t certSize = apiNvs.getBytesLength("apiCertificate");
    size_t pkSize = apiNvs.getBytesLength("apiPk");
    if (certSize != 0 && pkSize != 0) { //if non-zero, a certificate exists in NVS:
        uint8_t *certBuffer = new uint8_t[certSize + 1];
        uint8_t *keyBuffer = new uint8_t[pkSize + 1];

        size_t certSizeResult = apiNvs.getBytes("apiCertificate", certBuffer, apiNvs.getBytesLength("apiCertificate") + 1);
        log_d("Certificate length read from NVS: %i", certSizeResult);
        certificate->setCert(certBuffer, certSize);

        size_t pkSizeResult = apiNvs.getBytes("apiPk", keyBuffer, apiNvs.getBytesLength("apiPk") + 1);
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
        size_t certNvsResult = apiNvs.putBytes("apiCertificate", certificate->getCertData(), certificate->getCertLength() + 1);
        size_t pkNvsResult = apiNvs.putBytes("apiPk", certificate->getPKData(), certificate->getPKLength() + 1);
        log_d("Wrote Certificate with length %i and PK with length %i to NVS!", certNvsResult, pkNvsResult);
    }

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

        // Set content type of the response
        response->setHeader("Content-Type", "text/html");

        // Write a tiny HTTP page
        response->println("<!DOCTYPE html>");
        response->println("<html>");
        response->println("<head><title>Not Found</title></head>");
        response->println("<body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body>");
        response->println("</html");
    });
    receiveOauth = new httpsserver::ResourceNode("/authtoken", "POST",
        [](httpsserver::HTTPRequest *request, httpsserver::HTTPResponse *response) {
        size_t requestLength = request->getContentLength();
        char requestBuffer[requestLength+1];//add one for null terminator
        request->readChars(requestBuffer, requestLength);
        requestBuffer[requestLength] = '\0'; //manually add null terminator to end string
        log_d("%s", requestBuffer);
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


TwitchAPI::~TwitchAPI() {
    delete certificate;
    delete server;
}

void TwitchAPI::loopserver() {
    server->loop();
}

#undef DEBUG_PRINT