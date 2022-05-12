#include <TwitchAPI.h>


const char* TAG = "TwitchAPI";



TwitchAPI::TwitchAPI(const char* dnsName) : _authenticator(80) {
    log_d("Created TwitchAPI with DNS %s", dnsName);
}

// bool TwitchPubSub::begin(const char *DnsName) {
//     return MDNS.begin(DnsName);

// }

#undef DEBUG_PRINT