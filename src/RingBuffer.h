#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#ifdef ARDUINO
#   include <stdlib.h>
#   include <stdint.h>
#   include <string.h>
    namespace std
    {
        typedef ::size_t size_t;
    }
#else
#   include <cstdint>
#   include <cstring>
#endif

template<std::size_t S>
class RingBuffer
{
private:
    uint8_t _buffer[S];
    std::size_t _start = 0;
    std::size_t _end = 0;

    // _start == 0 && _end == 0 means empty buffer
    // (_start == _end || (_start == 0 && _end == S)) means full buffer

    void advanceStart(std::size_t n) {
        _start += n;
        if (_start == _end) {
            // if _start == _end then we've emptied the buffer, so reset
            _start = 0;
            _end = 0;
        } else if (_start >= S) {
            _start %= S;
        }
    }

    template<typename U>
    U _min(U a, U b) { return a < b ? a : b; }

    template<typename U>
    U _max(U a, U b) { return a > b ? a : b; }

    std::size_t availableTogether() {
        if (_start == _end && _start == 0) {
            return 0; // Empty buffer
        } else if (_start >= _end) {
            return S - _start; // Read until end of buffer
        } else {
            return _end - _start;
        }
    }

public:
    std::size_t capacity() const { return S; }
    std::size_t available() const {
        if (_end == _start) {
            if (_end == 0) {
                return 0;
            } else {
                return S;
            }
        } else {
            return _end > _start ? _end - _start : S - (_start - _end);
        }
    }

    template<typename T>
    size_t readFrom(T& instance) {
        std::size_t n;
        std::size_t p = _end;
        if (_end == _start) {
            if (_end == 0) {
                n = S; // Empty
            } else {
                return 0; // No room
            }
        } else if (S == _end) {
            // Wrap around
            p = 0;
            n = _start;
        } else if (_end > _start) {
            n = S - _end;
        } else {
            n = _start - _end;
        }

#       if 0
        std::size_t count = instance.read(static_cast<void*>(&_buffer[p]),
                                            static_cast<uint16_t>(n));

#       else
        std::size_t count = 0;
        int c;

        while (count < n && instance.connected() && instance.available()) {
            c = instance.read();
            if (0 > c) {
                break;
            }
            _buffer[count+p] = c;
            count++;
        }
#       endif

        _end += count;
        if (_end > S) {
            _end %= S;
        }
        return count;
    }

    int read() {
        if (0 == available()) {
            return -1;
        }

        uint8_t retval = _buffer[_start];
        advanceStart(1);
        return retval;
    }

    std::size_t read(void* dest, std::size_t n) {
        std::size_t len = availableTogether();
        n = _min(n, len);

        // Perform the copy
        memcpy(dest, &_buffer[_start], n);

        advanceStart(n);

        return n;
    }

    std::size_t readUntil(void* dest, uint8_t c, std::size_t n) {
        // Calculate number of linear available bytes
        std::size_t len = availableTogether();

        // Take the min of the number of available bytes, and the size of dest
        n = _min(n, len);

        // Perform the copy
        uint8_t* pos = static_cast<uint8_t*>(memccpy(dest, &_buffer[_start], c,
                                                        n));

        // Figure out how many bytes were copied
        std::size_t count;
        if (NULL == pos) {
            count = n;
        } else {
            count = pos - static_cast<uint8_t*>(dest);
        }

        advanceStart(count);

        return count;
    }

    int peek() {
        if (0 == available()) {
            return -1;
        }

        return _buffer[_start];
    }

    void putBack(uint8_t c) {
        if (0 == _start) {
            _start = S;
        }

        _buffer[--_start] = c;
    }

    void clear() {
        _start = 0;
        _end = 0;
    }
};

#endif
