#include "StringComparator.h"

#include <avr/pgmspace.h>

StringComparison::StringComparison(const StringComparator* parent)
    : _parent(parent), _invalid(new bool[parent->_n_strings])
{
    // TODO: Get rid of the new, because malloc is bad, m'kay?
    reset();
}

StringComparison::StringComparison(StringComparison&& other)
    : _parent(other._parent), _invalid(other._invalid), _count(other._count)
{
    other._invalid = NULL;
}

StringComparison& StringComparison::operator=(StringComparison&& other)
{
    if (this != &other) {
        // Free existing resources
        delete[] _invalid;

        // Move the pointer
        _invalid = other._invalid;
        _parent = other._parent;
        _count = other._count;

        // Clear the original
        other._invalid = NULL;
        other._parent = NULL;
        other._count = 0;
    }
    return *this;
}

void StringComparison::reset()
{
    if (NULL == _invalid || NULL == _parent) {
        return;
    }

    for (size_t ii = 0; ii < _parent->_n_strings; ii++) {
        _invalid[ii] = false;
    }
    _count = 0;
}

void StringComparison::next(uint8_t c)
{
    if (NULL == _invalid || NULL == _parent) {
        return;
    }

    for (size_t ii = 0; ii < _parent->_n_strings; ii++) {
        if (_invalid[ii]) {
            continue;
        }
        _invalid[ii] = _parent->charAt(ii, _count) != c;
    }
    _count++;
}

bool StringComparison::hasMatch(size_t& idx) const
{
    if (NULL == _invalid || NULL == _parent) {
        return false;
    }

    for (size_t ii = 0; ii < _parent->_n_strings; ii++) {
        if (!_invalid[ii]) {
            idx = ii;
            return strlen_P(_parent->_strings[ii]) == _count;
        }
    }
    return false;
}

StringComparison::~StringComparison()
{
    delete[] _invalid;
}

StringComparator::StringComparator(const char** strings, size_t n)
    : _strings(strings), _n_strings(n)
{

}

uint8_t StringComparator::charAt(size_t idx, size_t pos) const
{
    return pgm_read_byte(_strings[idx] + pos);
}

StringComparison StringComparator::create()
{
    return StringComparison(this);
}
