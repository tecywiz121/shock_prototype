#include "HTTP_Server.h"

#ifndef HTTP_SILENT
#include <avr/pgmspace.h>
static const char gError[] PROGMEM = "ERROR";
static const char gDebug[] PROGMEM = "DEBUG";

#define error(x) http_log(gError, F(x))
#define debug(x) http_log(gDebug, F(x))

static void http_log(const char *level, const __FlashStringHelper* msg)
{
    Serial.print(reinterpret_cast<const __FlashStringHelper*>(level));
    Serial.print(F(" - "));
    Serial.println(msg);
}
#else
#define error(x) do { ; } while (0)
#define debug(x) do { ; } while (0)
#endif

const __FlashStringHelper* HTTPClientStateToString(http_client_state state)
{
    switch (state) {
    case http_client_state::METHOD:
        return F("METHOD");
    case http_client_state::PATH:
        return F("PATH");
    case http_client_state::VERSION:
        return F("VERSION");
    case http_client_state::BEFORE_HEADER:
        return F("BEFORE_HEADER");
    case http_client_state::HEADER_NAME:
        return F("HEADER_NAME");
    case http_client_state::HEADER_VALUE:
        return F("HEADER_VALUE");
    case http_client_state::BODY:
        return F("BODY");
    case http_client_state::DONE:
        return F("DONE");
    }
}

const __FlashStringHelper* HTTPStatusToString(http_status status)
{
    switch (status) {
        case http_status::OKAY:
            return F("OKAY");
        case http_status::INCOMPLETE:
            return F("INCOMPLETE");
        case http_status::FAIL_NULL_ARG:
            return F("FAIL_NULL_ARG");
        case http_status::FAIL_INVALID_ARG:
            return F("FAIL_INVALID_ARG");
        case http_status::FAIL_DHCP:
            return F("FAIL_DHCP");
        case http_status::FAIL_HARDWARE:
            return F("FAIL_HARDWARE");
        case http_status::FAIL_TIMEOUT:
            return F("FAIL_TIMEOUT");
        case http_status::FAIL_INVALID_STATE:
            return F("FAIL_INVALID_STATE");
        case http_status::FAIL_WIFI:
            return F("FAIL_WIFI");
        case http_status::FAIL_UNSUPPORTED:
            return F("FAIL_UNSUPPORTED");
        case http_status::FAIL_BAD_REQUEST:
            return F("FAIL_BAD_REQUEST");
    }
}


/*****************************************************************************
 * HTTP_Client Implementation                                                *
 *****************************************************************************/
const static char HTTP_V_1_0[] PROGMEM = "HTTP/1.0";
const static char HTTP_V_1_1[] PROGMEM = "HTTP/1.1";
const static char* HTTP_VERSIONS[] = {HTTP_V_1_0, HTTP_V_1_1};

StringComparator HTTP_Client::_versionComparator(HTTP_VERSIONS, 2);

const static char HTTP_TRANSFER_ENCODING[] PROGMEM = "Transfer-Encoding";
const static char HTTP_CONTENT_LENGTH[] PROGMEM    = "Content-Length";
const static char* HTTP_HEADERS[] = {HTTP_TRANSFER_ENCODING, HTTP_CONTENT_LENGTH};

StringComparator HTTP_Client::_headerComparator(HTTP_HEADERS, 2);

static const char TRANSFER_ENCODING_CHUNKED[] PROGMEM = "chunked";
static const char* TRANSFER_ENCODINGS[] = {TRANSFER_ENCODING_CHUNKED};
StringComparator HTTP_Client::_transferEncodingComparator(TRANSFER_ENCODINGS, 1);

void HTTP_Client::connect()
{
    _connected = true;
    _buffer.clear();
    _header = http_header::UNKNOWN;
    _contentLength = 0;
    _chunked = false;
    state(http_client_state::METHOD);
}

void HTTP_Client::disconnect()
{
    _connected = false;
}

void HTTP_Client::state(http_client_state s)
{
    if (s != _state) {
        _state = s;
        _intParser.reset();

        switch (_state) {
            case http_client_state::VERSION:
                _comparison = _versionComparator.create();
                break;
            case http_client_state::HEADER_NAME:
                _comparison = _headerComparator.create();
                break;
            case http_client_state::HEADER_VALUE:
                if (http_header::TRANSFER_ENCODING == _header) {
                    _comparison = _transferEncodingComparator.create();
                }
                // TODO: Handle Content-Length when applicable
                break;
            default:
                _comparison = StringComparison();
        }
    }
}

void HTTP_Client::getTransition(uint8_t& terminator, http_client_state& next)
{
    switch (_state) {
        case http_client_state::METHOD:
            terminator = ' ';
            next = http_client_state::PATH;
            break;
        case http_client_state::PATH:
            terminator = ' ';
            next = http_client_state::VERSION;
            break;
        case http_client_state::VERSION:
            terminator = '\n';
            next = http_client_state::BEFORE_HEADER;
            break;
        case http_client_state::HEADER_NAME:
            terminator = ':';
            next = http_client_state::HEADER_VALUE;
            break;
        case http_client_state::HEADER_VALUE:
            terminator = '\n';
            next = http_client_state::BEFORE_HEADER;
            break;
        case http_client_state::BODY:
            // TODO
            break;
        case http_client_state::DONE:
            // TODO
            break;
        default:
            // TODO: This is an error state.
            break;
    }
}

http_status HTTP_Client::read(uint8_t* buf, size_t* n_buf,
        http_client_state* current)
{
    if (NULL == n_buf) {
        error("n_buf is null");
        return http_status::FAIL_NULL_ARG;
    }

    if (NULL == current) {
        error("current is null");
        return http_status::FAIL_NULL_ARG;
    }

    if (0 == *n_buf) {
        error("*n_buf must be greater than or equal to 1");
        return http_status::FAIL_INVALID_ARG;
    }

    int peeked;

    // Set the current state
    *current = _state;

    if (http_client_state::BEFORE_HEADER == _state) {
        peeked = _buffer.peek();
        if (0 > peeked) {
            *n_buf = 0;
            return http_status::INCOMPLETE;
        } else if ('\r' == peeked) {
            _buffer.read();
            peeked = _buffer.peek();
            if (0 > peeked) {
                // Can't decide yet, so put the '\r' back and wait for more
                _buffer.putBack('\r');
                *n_buf = 0;
                return http_status::INCOMPLETE;
            } else if ('\n' == peeked) {
                _buffer.read();
                state(http_client_state::BODY);
            } else {
                // Malformed request... Application can deal with extra '\r'
                state(http_client_state::HEADER_NAME);
            }
        } else if ('\n' == peeked) {
            // Missing '\r', but got a new line, so... go to BODY
            _buffer.read();
            state(http_client_state::BODY);
        } else {
            state(http_client_state::HEADER_NAME);
        }
    }

    // The state may have changed by here, so update it
    *current = _state;

    if (http_client_state::BODY == _state) {
        return readBody(buf, n_buf);
    } else {
        return readTerminated(buf, n_buf);
    }
}

http_status HTTP_Client::readBody(uint8_t* buf, size_t* n_buf)
{
    if (_chunked) {
        return http_status::FAIL_UNSUPPORTED; // TODO: support this
    }

    *n_buf = min(*n_buf, _contentLength);
    *n_buf = _buffer.read(buf, *n_buf);

    _contentLength -= *n_buf;

    if (0 == _contentLength) {
        state(http_client_state::DONE);
        return http_status::OKAY;
    } else {
        return http_status::INCOMPLETE;
    }
}

http_status HTTP_Client::readTerminated(uint8_t* buf, size_t* n_buf)
{
    int peeked;

    // Get the terminator and the next state
    uint8_t terminator;
    bool transition = false;
    http_client_state next;
    getTransition(terminator, next);

    // Read until we hit the terminator, or run out of buffer space
    *n_buf = _buffer.readUntil(buf, terminator, *n_buf);

    http_status retval = http_status::INCOMPLETE;

    if (*n_buf == 0) {
        // No new bytes available to be read
        retval = http_status::INCOMPLETE;
    } else if (terminator == buf[*n_buf-1]) {
        // Got the terminator
        *n_buf -= 1;
        transition = true;
        retval = http_status::OKAY;

        if ('\n' == terminator) {
            if (*n_buf >= 1 && buf[*n_buf-1] == '\r') {
                *n_buf -= 1; // Strip the '\r' from the returned string
            }
        } else if (':' == terminator) {
            // Be somewhat tolerant of spaces after header names
            peeked = _buffer.peek();
            if (0 > peeked) {
                _buffer.putBack(terminator);          // Put the ':' back, and
                retval = http_status::INCOMPLETE;   // wait for the next byte.
                transition = false;
            } else if (' ' == peeked) {
                _buffer.read();                   // Consume the space
            } else {
                // Jump to the next state
            }
        }
    } else if ('\n' == terminator && '\r' == buf[*n_buf-1]) {
        // Put the '\r' back so that it always comes with its matching '\n'
        _buffer.putBack('\r');
        *n_buf -= 1;
    }

    // Advance the comparator
    processState(buf, *n_buf);

    if (transition) {
        // Check to see if the comparator matches anything
        http_status event_status = checkState();
        if (http_status::OKAY != event_status) {
            return event_status;
        }

        // Adavance the state
        state(next);
    }

    return retval;
}

void HTTP_Client::processState(uint8_t* buf, size_t n_buf)
{
    if (0 >= n_buf) {
        return;
    }

    bool str = true;

    switch (_state) {
        case http_client_state::VERSION:
        case http_client_state::HEADER_NAME:
            break;
        case http_client_state::HEADER_VALUE:
            str = _header != http_header::CONTENT_LENGTH;
            break;
        default:
            return;
    }

    if (str) {
        for (size_t ii = 0; ii < n_buf; ii++) {
            _comparison.next(buf[ii]);
        }
    } else {
        for (size_t ii = 0; ii < n_buf; ii++) {
            _intParser.next(buf[ii]);
        }
    }
}


http_status HTTP_Client::checkState()
{
    size_t idx;
    if (!_comparison.hasMatch(idx)) {
        // No matching header/version/whatever...
        switch (_state) {
            case http_client_state::VERSION:
                _version = http_version::UNKNOWN;
                break;
            case http_client_state::HEADER_NAME:
                _header = http_header::UNKNOWN;
                break;
            case http_client_state::HEADER_VALUE:
                if (http_header::TRANSFER_ENCODING == _header) {
                    error("got unknown transfer encoding");
                    return http_status::FAIL_UNSUPPORTED;
                }
                break;
            default:
                break;
        }
    } else {
        switch (_state) {
            case http_client_state::VERSION:
                switch (idx) {
                    case 0:
                        _version = http_version::HTTP_1_0;
                        debug("Got HTTP/1.0");
                        break;
                    case 1:
                        _version = http_version::HTTP_1_1;
                        debug("Got HTTP/1.1");
                        break;
                    default:
                        error("version comparator returned bad version");
                        return http_status::FAIL_INVALID_STATE;
                }
                break;
            case http_client_state::HEADER_NAME:
                switch (idx) {
                    case 0:
                        _header = http_header::TRANSFER_ENCODING;
                        debug("Got TRANSFER_ENCODING");
                        break;
                    case 1:
                        _header = http_header::CONTENT_LENGTH;
                        debug("Got CONTENT_LENGTH");
                        break;
                    default:
                        error("header name comparator returned bad header");
                        return http_status::FAIL_INVALID_STATE;
                }
                break;
            case http_client_state::HEADER_VALUE:
                if (http_header::TRANSFER_ENCODING == _header) {
                    switch (idx) {
                        case 0:
                            _chunked = true;
                            debug("Got chunked transfer encoding");
                            break;
                        default:
                            error("transfer encoding comparator return bad value");
                            return http_status::FAIL_INVALID_STATE;
                    }
                }
                break;
            default:
                break;
        }
    }

    if (http_client_state::HEADER_VALUE == _state
            && http_header::CONTENT_LENGTH == _header) {
        uintmax_t len;
        if (_intParser.value(&len)) {
            _contentLength = len;
        } else if (_intParser.overflowed()) {
            error("content length too long");
            return http_status::FAIL_UNSUPPORTED;
        } else if (_intParser.invalid()) {
            error("non-numeric characters in content length");
            return http_status::FAIL_BAD_REQUEST;
        } else {
            error("problem with content length");
            return http_status::FAIL_INVALID_STATE;
        }
    }
    return http_status::OKAY;
}

/*****************************************************************************
 * HTTP_Server Implementation                                                *
 *****************************************************************************/
HTTP_Server::HTTP_Server(uint8_t cs, uint8_t irq, uint8_t vbat,
        uint8_t spi_div, uint16_t port)
    : _cc3000(cs, irq, vbat, spi_div), _server(port)
{
}

http_status HTTP_Server::begin()
{
    /* Initialise the module */
    debug("Initializing...");
    if (!_cc3000.begin()) {
        error("Couldn't begin()! Check your wiring?");
        return http_status::FAIL_HARDWARE;
    }

    return http_status::OKAY;
}

http_status HTTP_Server::connect(const char* ssid, const char* key,
        uint8_t secmode, uint8_t attempts)
{
    debug("Connecting to access point...");

    if (!_cc3000.connectToAP(ssid, key, secmode, attempts)) {
        error("Unable to connect to access point");
        return http_status::FAIL_WIFI;
    }

    debug("Connected!");

    debug("Requesting IP address...");

    uint8_t dhcp = 0;
    while (!_cc3000.checkDHCP()) {
        if (attempts && dhcp >= attempts) {
            error("Unable to acquire IP address");
            _cc3000.disconnect();
            return http_status::FAIL_DHCP;
        }
        delay(100);
    }

    debug("Got IP address!");

    _server.begin();

    return http_status::OKAY;
}

http_status HTTP_Server::tick()
{
    /* Find disconnected clients */
    for (size_t ii = 0; ii < MAX_SERVER_CLIENTS; ii++) {
        HTTP_Client& httpClient = client(ii);
        Adafruit_CC3000_ClientRef ccClient = _server.getClientRef(ii);
        if (httpClient.connected() && !ccClient.connected()) {
            Serial.print(F("Disconnected - Client "));
            Serial.println(ii, DEC);
            httpClient.disconnect();
        }
    }

    /* Accept new clients */
    bool newClient = false;
    int8_t idx = _server.availableIndex(&newClient);

    /* Update connected clients */
    if (newClient) {
        for (size_t ii = 0; ii < MAX_SERVER_CLIENTS; ii++) {
            HTTP_Client& httpClient = client(ii);
            Adafruit_CC3000_ClientRef ccClient = _server.getClientRef(ii);
            if (!httpClient.connected() && ccClient.connected()) {
                Serial.print(F("Connected - Client "));
                Serial.println(ii, DEC);
                httpClient.connect();
            }
        }
    }

    /* Handle new data */
    if (idx >= 0) {
        Adafruit_CC3000_ClientRef ccClient = _server.getClientRef(idx);
        HTTP_Client& httpClient = client(idx);

        httpClient._buffer.readFrom(ccClient);
    }

    /* Process any data in client buffers */
    for (size_t ii = 0; ii < MAX_SERVER_CLIENTS; ii++) {
        HTTP_Client& httpClient = client(ii);
        if (httpClient.connected()) {
            httpClient.process();
        }
    }

    return http_status::OKAY;
}
