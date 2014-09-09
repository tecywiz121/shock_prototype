#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <Adafruit_CC3000.h>
#include "RingBuffer.h"
#include "StringComparator.h"
#include "IntParser.h"

#ifndef HTTP_BUFFER_SIZE
#   define HTTP_BUFFER_SIZE RXBUFFERSIZE
#elif HTTP_BUFFER_SIZE < RXBUFFERSIZE
    /*
     * Don't really know why it glitches with < RXBUFFERSIZE, should look into
     * it eventually.
     */
#   error "HTTP_BUFFER_SIZE must be greater than or equal to RXBUFFERSIZE"
#endif /* HTTP_BUFFER_SIZE */

enum class http_status
{
	OKAY,
    INCOMPLETE,
    FAIL_HARDWARE,
    FAIL_WIFI,
    FAIL_DHCP,
    FAIL_INVALID_STATE,
    FAIL_TIMEOUT,
    FAIL_NULL_ARG,
    FAIL_INVALID_ARG,
    FAIL_BAD_REQUEST,
    FAIL_UNSUPPORTED,
};

enum class http_client_state
{
    METHOD,
    PATH,
    VERSION,
    BEFORE_HEADER,
    HEADER_NAME,
    HEADER_VALUE,
    BODY,
    DONE,
};

enum class http_version
{
    UNKNOWN,
    HTTP_1_0,
    HTTP_1_1,
};

enum class http_header
{
    UNKNOWN,
    TRANSFER_ENCODING,
    CONTENT_LENGTH,
};

const __FlashStringHelper* HTTPClientStateToString(http_client_state state);
const __FlashStringHelper* HTTPStatusToString(http_status status);

class HTTP_Client
{
    friend class HTTP_Server;
private:
    bool _connected = false;
    RingBuffer<HTTP_BUFFER_SIZE> _buffer;

    // Tracks the state of the http request
    http_client_state _state = http_client_state::METHOD;

    // Version information for the client
    static StringComparator _versionComparator;
    http_version _version;

    // Important headers
    static StringComparator _headerComparator;
    http_header _header;

    // Transfer-Encoding
    static StringComparator _transferEncodingComparator;
    bool _chunked = false;

    // Content-Length
    IntParser _intParser;
    uintmax_t _contentLength = 0;

    // Reusable comparison
    StringComparison _comparison;

    void disconnect();
    void connect();

    void getTransition(uint8_t& terminator, http_client_state& next);
    void processState(uint8_t* buf, size_t n_buf);
    http_status checkState();

    void state(http_client_state s);

    http_status readTerminated(uint8_t* buf, size_t* n_buf);
    http_status readBody(uint8_t* buf, size_t* n_buf);

protected:
    HTTP_Client() = default;

    http_status read(uint8_t* buf, size_t* n_buf, http_client_state* current);

    virtual void process() =0;

public:
    bool connected() const { return _connected; }
    virtual ~HTTP_Client() = default;
};

class HTTP_Server
{
private:
    Adafruit_CC3000 _cc3000;
    Adafruit_CC3000_Server _server;

protected:
    virtual HTTP_Client& client(size_t idx) =0;

public:
    HTTP_Server(uint8_t cs, uint8_t irq, uint8_t vbat, uint8_t spi_div,
                uint16_t port = 80);

    http_status begin();

    http_status connect(const char* ssid, const char* key, uint8_t secmode,
            uint8_t attempts = 0);

    http_status tick();

    virtual ~HTTP_Server() = default;
};

#endif
