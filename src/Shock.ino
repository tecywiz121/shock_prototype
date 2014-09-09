#define CC3000_TINY_DRIVER

#include <SPI.h>
#include "HTTP_Server.h"
#include "StringComparator.h"
#include "utility/debug.h"
#include "IntParser.h"

// These are the interrupt and control pins
#define CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define CC3000_VBAT  5
#define CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11

#define WLAN_SSID       "Anubis"           // cannot be longer than 32 characters!
#define WLAN_PASS       "hackmenow321"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

class My_HTTP_Client : public HTTP_Client
{
    friend class My_HTTP_Server;
    using HTTP_Client::HTTP_Client;
private:
    http_client_state old_state = http_client_state::DONE;

protected:
    virtual void process() override
    {
        uint8_t buf[101];
        std::size_t n_buf = 100;

        http_client_state state;
        http_status status = read(buf, &n_buf, &state);

        if (old_state != state) {
            old_state = state;
            Serial.println();
            Serial.print(HTTPClientStateToString(state));
            Serial.print(F(" "));
            Serial.print((uintptr_t)this, HEX);
            Serial.print(F(": "));
        }

        buf[n_buf] = '\0';

        switch (status) {
        case http_status::INCOMPLETE:
        case http_status::OKAY:
            if (n_buf > 1) {
                Serial.print(reinterpret_cast<char*>(buf));
            } else if (n_buf == 1) {
                Serial.print("<");
                Serial.print(buf[0], HEX);
                Serial.print("> ");
            }
            break;
        default:
            Serial.println(HTTPStatusToString(status));
            break;
        }
    }
};

class My_HTTP_Server : public HTTP_Server
{
    using HTTP_Server::HTTP_Server;
private:
    My_HTTP_Client _clients[MAX_SERVER_CLIENTS];

protected:
    virtual HTTP_Client& client(size_t idx) override
    {
        return _clients[idx];
    }
};

static My_HTTP_Server server(CC3000_CS, CC3000_IRQ, CC3000_VBAT, SPI_CLOCK_DIVIDER);

static void die(void)
{
    Serial.println();
    Serial.println(F("Dead."));
    while (1);
}

void setup()
{
    Serial.begin(115200);
    Serial.println(F("Hello, CC3000!\n"));

    Serial.print(F("Free RAM: ")); Serial.println(getFreeRam(), DEC);

    if (http_status::OKAY != server.begin()) {
        die();
    }

    if (http_status::OKAY != server.connect(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
        die();
    }
}

void loop()
{
    if (http_status::OKAY != server.tick()) {
        die();
    }
}
