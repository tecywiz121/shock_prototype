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

enum class http_request_state
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

enum class http_response_state
{
    VERSION,
    STATUS_CODE,
    STATUS_REASON,
    HEADER_NAME,
    HEADER_VALUE,
    BODY,
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

const __FlashStringHelper* HTTPClientStateToString(http_request_state state);
const __FlashStringHelper* HTTPStatusToString(http_status status);

class HTTP_Client
{
    friend class HTTP_Server;
private:
    bool _connected = false;
    Adafruit_CC3000_ClientRef _client = Adafruit_CC3000_ClientRef(NULL);
    RingBuffer<HTTP_BUFFER_SIZE> _buffer;

    // Tracks the state of the http request
    http_request_state _requestState = http_request_state::METHOD;
    http_response_state _responseState = http_response_state::VERSION;

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
    void client(Adafruit_CC3000_ClientRef c) { _client = c; }

    void getTransition(uint8_t& terminator, http_request_state& next);
    void processState(uint8_t* buf, size_t n_buf);
    http_status checkState();

    void requestState(http_request_state s);
    void responseState(http_response_state s);

    http_status readTerminated(uint8_t* buf, size_t* n_buf);
    http_status readBody(uint8_t* buf, size_t* n_buf);

    bool isValidResponseTransition(http_response_state s);

protected:
    HTTP_Client() = default;

    http_version version() const { return _version; }
    http_response_state responseState() const { return _responseState; }
    http_request_state requestState() const { return _requestState; }

    http_status read(uint8_t* buf, size_t* n_buf, http_request_state* current);

    size_t write(uint8_t* buf, size_t n);
    size_t write(const char* str);
    http_status write(uint8_t c);
    size_t write(const __FlashStringHelper* str);

    http_status advanceTo(http_response_state state);

    http_status close();

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
