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
        uint8_t certBuffer[certSize];
        uint8_t keyBuffer[pkSize];

        size_t certSizeResult = apiNvs.getBytes("apiCertificate", certBuffer, apiNvs.getBytesLength("apiCertificate"));
        log_d("Certificate length read from NVS: %i", certSizeResult);
        certificate->setCert(certBuffer, certSize);

        size_t pkSizeResult = apiNvs.getBytes("apiPk", keyBuffer, apiNvs.getBytesLength("apiPk"));
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
        size_t certNvsResult = apiNvs.putBytes("apiCertificate", certificate->getCertData(), certificate->getCertLength());
        size_t pkNvsResult = apiNvs.putBytes("apiPk", certificate->getPKData(), certificate->getPKLength());
        log_d("Wrote Certificate with length %i and PK with length %i to NVS!", certNvsResult, pkNvsResult);
    }

    //Setup the server and attach the nodes:
    // nodeRoot = new httpsserver::ResourceNode("/", "GET", handleRoot);

}


TwitchAPI::~TwitchAPI() {
    delete certificate;
}


void (TwitchAPI::*handleRoot)(httpsserver::HTTPRequest *request, httpsserver::HTTPResponse *response) {

}

#undef DEBUG_PRINT