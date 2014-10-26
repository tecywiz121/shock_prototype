#define CC3000_TINY_DRIVER

#include <SPI.h>
#include "HTTP_Server.h"
#include "StringComparator.h"
#include "utility/debug.h"

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
    http_request_state old_state = http_request_state::DONE;

    void reportError(http_status e) {
        if (responseState() == http_response_state::VERSION) {
            write(F("HTTP/1.0"));
            advanceTo(http_response_state::STATUS_CODE);
        }

        if (responseState() == http_response_state::STATUS_CODE) {
            switch (e) {
                case http_status::FAIL_BAD_REQUEST:
                    write(F("400"));
                    break;
                default:
                    write(F("500"));
                    break;
            }
            advanceTo(http_response_state::STATUS_REASON);
        }

        if (responseState() == http_response_state::STATUS_REASON) {
            write(HTTPStatusToString(e));
            advanceTo(http_response_state::HEADER_NAME);
        }

        if (responseState() == http_response_state::HEADER_NAME) {
            write(F("Content-Length"));
            advanceTo(http_response_state::HEADER_VALUE);
        }

        if (responseState() == http_response_state::HEADER_VALUE) {
            write('0');
            advanceTo(http_response_state::BODY);
        }
    }

protected:
    virtual void process() override
    {
        uint8_t buf[65];
        std::size_t n_buf = 64;

        http_request_state state;
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
            reportError(status);
            break;
        }

        if (http_status::OKAY == status) {
            switch (state) {
                case http_request_state::VERSION:
                    switch (version()) {
                        case http_version::HTTP_1_1:
                            write(F("HTTP/1.1"));
                            break;
                        case http_version::HTTP_1_0:
                        default:
                            write(F("HTTP/1.0"));
                            break;
                    }
                    advanceTo(http_response_state::STATUS_CODE);
                    break;
                case http_request_state::BODY:
                    write(F("200"));
                    advanceTo(http_response_state::STATUS_REASON);
                    write(F("OK"));
                    advanceTo(http_response_state::HEADER_NAME);
                    write(F("Content-Length"));
                    advanceTo(http_response_state::HEADER_VALUE);
                    write(F("11"));
                    advanceTo(http_response_state::HEADER_NAME);
                    write(F("Connection"));
                    advanceTo(http_response_state::HEADER_VALUE);
                    write(F("close"));
                    advanceTo(http_response_state::BODY);
                    write(F("Hello World"));
                    close();
                    break;
            }
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
