#include <TwitchAPI.h>


const char *TAG = "TwitchAPI";

TwitchAPI::TwitchAPI(const char *dnsName) {
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

        // The response implements the Print interface, so you can use it just like
        // you would write to Serial etc.
        response->println("<!DOCTYPE html>");
        response->println("<html>");
        response->println("<head><title>Hello World!</title></head>");
        response->println("<body>");
        response->println("<h1>Hello World!</h1>");
        response->print("<p>Your server is running for ");
        // A bit of dynamic data: Show the uptime
        response->print((int)(millis() / 1000), DEC);
        response->println(" seconds.</p>");
        response->println("</body>");
        response->println("</html>");
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

    log_d("Creating server");
    server = new httpsserver::HTTPSServer(certificate);

    log_d("Registering node:");
    server->registerNode(nodeRoot);
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