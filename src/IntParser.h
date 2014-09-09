#ifndef INTPARSER_H
#define INTPARSER_H

#include <stdlib.h>
#include <stdint.h>

class IntParser
{
private:
    bool _overflowed = false;
    bool _invalid = false;
    uintmax_t _value = 0;

public:
    void reset();
    void next(uint8_t c);
    bool value(uintmax_t* v) const { *v = _value; return !_overflowed && !_invalid; }

    bool overflowed() const { return _overflowed; }
    bool invalid() const { return _invalid; }
};

#endif /* INTPARSER_H */
